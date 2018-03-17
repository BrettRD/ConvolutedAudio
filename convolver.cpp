/** Convolver
  *	runs a convolution on incoming data streams.
  * Using OpenCV because I feel like it
  * I'd like to run the William Gardner various window size magic,
  * This convolution is designed to extract the position of a known signal from input data
  *	The actual result will be the accumulated energy of the signal conentrated
  *		to a delta func at the end of the reference.
  * Since the result can only be measured at the end of the long reference block,
  * 	there isn't much of a compelling reason to optimise for latency
  *
  *
  *	
*/

#include "convolver.h"

using namespace std;
using namespace cv;


int nRefChannels = 1;	//one reference signal for now
int nDatchannels = 1;	//one channel audio for now

  //prep a kernel with multiple reference signal (Row vectors)
  //the input buffer is a single row vector (could stripe them with N zero vectors between them)
  //Run filter2d on the input buffer and the signals


Mat kernel;		//the reference signal(s)
Mat sigData;	//the input data
Mat outData;	//the output delta functions
Mat image;
fstream *fReferenceFile;
fstream fout;
fstream fkern;
fstream finput;

//void PrepKernel(vector<float> *refSig){
void PrepKernel(fstream *fReference)
{
	//read the reference signal data into the kernel
	fReferenceFile = fReference;
	fReferenceFile->seekg (0, ios::end);       //seek to the end of the file to tell the size

    size_t refsize = fReferenceFile->tellg();  //the pointer to the current byte == the length
    cout << "The sample file is " << refsize << " bytes long" << endl;    
    fReferenceFile->seekg (0, ios::beg);       //return to the beginning of the file
    fout.open( "results.raw", ios::out|ios::binary);
    finput.open( "input.raw", ios::out|ios::binary);
	fkern.open( "kern.raw", ios::out|ios::binary);
    //also prep the display while we're at it
}



void freshEntropy(float *refBuffer, size_t len)
{
    int16_t sample = 0;	//import sample-by-sample

	for(size_t i=0; i<len; i++)
	{
    	fReferenceFile->read ( (char*) &sample, sizeof(int16_t));   //read the whole file
		if(fReferenceFile->eof())
		{
			//when we're out of reference data, start from the beginning
			fReferenceFile->clear();
			fReferenceFile->seekg (0, ios::beg);
			fReferenceFile->read ( (char*) &sample, sizeof(int16_t));   //read the whole file
		}
    	float sampleFloat = sample;
    	refBuffer[i] = sampleFloat / (float) INT16_MAX;
	}

}

//compare the vector to the kernel and report some stats
void SpoolBuffers(float *inputSig, float* kernelBuf, size_t len){

    fflush(stdout);

	sigData = Mat(1, len, CV_32F, inputSig);	//create a matrix over the array
	kernel = Mat(1, len + stepOver, CV_32F, kernelBuf);

	//Convolve(sigData,kernel, outData);
	Convolve();

}

//void Convolve(Mat &sigData, Mat &kern, Mat &outData)
void Convolve()
{
    //filter2d runs a correlation which is the same
    // equation as a FIR if you limit to 1xN images
    //	this also allows easy exansion of channel or reference count
	
	//int ddepth = CV_32F;	//default to source bit depth
	//Point anchor(0,stepOver);	//default to kernel centre
	//double delta = 0;		//inject a DC offset
	//int borderType = BORDER_ISOLATED;	//assume the reference loops

	//outData = Mat(sigData.size(), CV_32F, Scalar(0));	//create a matrix over the array
	//cout << "incoming sig size = " << sigData.size() << endl;
	//cout << "kernel size = " << kernel.size() << endl;
	//cout << "outgoing size = " << outData.size() << endl;

	//filter2D uses a straight-up correlation.
		//the regular correlation using a white noise kernel leaves the input PSD represented in the output.
		//any strong single-frequency noise will come through just fine, but with altered phase.
			//in general, if the PSD is very different to the kernel, we know we can't use phase from any peaks.
	//filter2D(sigData, outData, ddepth , kernel, anchor, delta, borderType);

	//Solution is to normalize the input to match the kernel,
		//or even notch-filter strong signals from the input.

	//with a kernel of 2*fftSize, zero-pad it to 4*fftSize
	//zero-pad the input to the same size
	//DFT the kernel and image. K, I
	//normalize the spectrum of the input, (I/|I|)
	//re-scale the input spectrum to the kernel amplitudes (|K|.*(I./|I|))
	//multiply the re-scaled input by the complex conjugate of the kernel ((K*) .* I) .* (|K| ./ |I|)
		//total power of correlation should be constant frame-by-frame, with time concentration indicating confidence
			//optionally divide through by |I|^2 instead to attenuate channels that are likely swamped with noise.

	Mat sigSpectrum = Mat();
	Mat kernSpectrum = Mat();
	Mat outSpectrum = Mat();
	Mat planes[2];	//for working with real and complex components

	int flags = 0;
	int nonzeroRows = 1;
	bool conjkern = true;

   	//cout << "start convolution" << endl;
	//fflush(stdout);
	Mat sigDataPadded;
	Mat kernelPadded;
	copyMakeBorder(sigData,sigDataPadded,0,0,0,fftSize+(2*stepOver), BORDER_CONSTANT, Scalar(0));
	copyMakeBorder(kernel,kernelPadded,0,0,0,fftSize+stepOver, BORDER_CONSTANT, Scalar(0));


	//calculate the dft for the input and the kernel
	flags = DFT_ROWS | DFT_COMPLEX_OUTPUT;
	dft (sigDataPadded, sigSpectrum, flags, nonzeroRows);
	dft (kernelPadded, kernSpectrum, flags, nonzeroRows);
	

	//compute the magnitudes of the spectra of the signals
	Mat kernMag = Mat::zeros(kernSpectrum.size(), CV_32F);
    split(kernSpectrum, planes);                   // planes[0] = Re(DFT(I), planes[1] = Im(DFT(I))
    magnitude(planes[0], planes[1], kernMag);// planes[0] = magnitude

	Mat sigMag = Mat::zeros(sigSpectrum.size(), CV_32F);
    split(sigSpectrum, planes);                   // planes[0] = Re(DFT(I), planes[1] = Im(DFT(I))
    magnitude(planes[0], planes[1], sigMag);// planes[0] = magnitude

    //create a filter to return the signal to the kernel's spectrum
	Mat envelope = kernMag.mul(1/sigMag);

	//square it to attenuate any obviously polluted frequencies.
    envelope.mul(envelope);

	//apply the filter
    planes[0] = planes[0].mul(envelope);
    planes[1] = planes[1].mul(envelope);
	merge(planes, 2, sigSpectrum);

	//calculate the correlation
	flags = DFT_ROWS;
	conjkern = true;
	mulSpectrums(sigSpectrum, kernSpectrum, outSpectrum, flags, conjkern);

	//calculate the inverse dft for the output
	flags = DFT_INVERSE | DFT_ROWS | DFT_COMPLEX_OUTPUT;
	dft (outSpectrum, outData, flags, nonzeroRows);

    split(outData, planes);                   // planes[0] = Re(DFT(I), planes[1] = Im(DFT(I))
    outData = planes[0];	//discard the imagniary component

    //outData = outData(Rect(outData.size().width/2, 0, outData.size().width/2, 1));	//discard the first half of the dft
    outData = outData(Rect(3*outData.size().width/4, 0, outData.size().width/4, 1));	//discard the first three quarters of the dft

	double pulseMax;	//size of the peak
	double pulseMin;	//size of the inverted peak (likely)
	minMaxLoc(outData, &pulseMin, &pulseMax, NULL, NULL);
	outData = outData / max(pulseMax, -pulseMin);
	

    fout.write((char*) outData.ptr(0),  sizeof(float) * outData.size().width);
    finput.write((char*) sigData.ptr(0),  sizeof(float) * fftSize);
    fkern.write((char*) kernel.ptr(0),  sizeof(float) * (fftSize + stepOver));

	//find the maximum, minimum, and variance for each input channel.
	//displayTF(kernel);
	displayTF(outData);

}


void displayTF(Mat &tfMat)
//void displayTF()
{

	double pulseMax;	//size of the peak
	double pulseMin;	//size of the inverted peak (likely)
	Point pulseMaxT;	//sample number of the peak
	Point pulseMinT;	//sample number of the inverted peak
	Scalar noiseMean(0);	//DC offset, not required
	Scalar noiseStddev(0);	//measurement of the noise floor (RMS)

	minMaxLoc(tfMat, &pulseMin, &pulseMax, &pulseMinT, &pulseMaxT);
	cout << "peak at "  << pulseMaxT.x << " height is " << pulseMax << endl;

	meanStdDev(tfMat, noiseMean, noiseStddev);
	//cout << "noise floor = " << noiseStddev[0] << endl;

	int height = 512;
	int width = 1024;

	image = Mat(height, width, CV_8U);

	int offset = pulseMaxT.x-(width/2);
	if(offset+width > tfMat.size().width)
	{
		offset = tfMat.size().width - width;
	}
	else if (offset < 0)
	{
		offset = 0;
	}

	for(int i=0; i<width; i++)
	{
		int j;
		for(j=0; j<height; j++)
		{
			image.at<uint8_t>(j,i) = 0;
		}

		j = (height/2) + ((height/2) * (tfMat.at<float>(0,i+offset) / (pulseMax-pulseMin)));
		//cout << "marker " << i << ", " << j << endl;
		if(j < 0) j = 0;
		if(j >= height) j = height - 1;
		image.at<uint8_t>(j,i) = 255;
	}

    namedWindow( "Display window", WINDOW_AUTOSIZE );// Create a window for display.
    imshow( "Display window", image );                   // Show our image inside it.
	waitKey(1);

}

