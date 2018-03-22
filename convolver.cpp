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
Mat calibrationSpectrum;
Mat calibrationResponse = Mat(1, 2*(fftSize + stepOver), CV_32F);

int dispHeight = 100;
int dispWidth = 1024;
float dispDecay = 0.9;
float skewRate = 0.1;
Mat image = Mat(dispHeight, dispWidth, CV_8UC3);
Mat tfDoppler = Mat(dispHeight, dispWidth, CV_32F);

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
    fflush(stdout);
  
    fReferenceFile->seekg (0, ios::beg);       //return to the beginning of the file
    fout.open( "results.raw", ios::out|ios::binary);
    finput.open( "input.raw", ios::out|ios::binary);
	fkern.open( "kern.raw", ios::out|ios::binary);

	//fetch the impulse response for the speaker microphone combination.
    fstream fcalib;
    fcalib.open( "deconv.raw", ios::in|ios::binary);
    fcalib.read ( (char*) calibrationResponse.ptr(0) , fftSize * sizeof(float));   
    //if the file is shorter, don't care.
	fcalib.close();
	//calculate the spectrum for the calibration
	int nonzeroRows = 1;
	int flags = DFT_ROWS | DFT_COMPLEX_OUTPUT;
	dft (calibrationResponse, calibrationSpectrum, flags, nonzeroRows);


	//conjugate divided by magnitude squared seems to be the best OpenCV has to offer
	//Mat planes[2];
	//split(calibrationSpectrum, planes);                   // planes[0] = Re, planes[1] = Im
	//Mat specMag;
	//magnitude(planes[0], planes[1], specMag);
	//planes[0] =  planes[0] / specMag;
	//planes[1] = -planes[1];
	//planes[0] = (planes[0]).mul(1/specMag);
	//planes[1] = (-planes[1]).mul(1/specMag);
	//planes[0] = ( planes[0]).mul(1/specMag.mul(specMag));
	//planes[1] = (-planes[1]).mul(1/specMag.mul(specMag));
	//merge(planes, 2, calibrationSpectrum);
}



void freshEntropy(float *refBuffer, size_t len)
{

    Mat noise = Mat(1, len, CV_32F, refBuffer);
	randn(noise, 0, 0.3);

    //int16_t sample = 0;	//import sample-by-sample
	/*
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
	*/

}

//compare the vector to the kernel and report some stats
void SpoolBuffers(float *inputSig, float* kernelBuf, size_t len){

	sigData = Mat(1, len, CV_32F, inputSig);	//create a matrix over the array
	kernel = Mat(1, len + stepOver, CV_32F, kernelBuf);

	//Convolve(sigData,kernel, outData);
	Convolve();

}

//void Convolve(Mat &sigData, Mat &kern, Mat &outData)
void Convolve()
{

	Mat sigSpectrum = Mat();
	Mat kernSpectrum = Mat();
	Mat outSpectrum = Mat();
	Mat planes[2];	//for working with real and complex components

	int flags = 0;
	int nonzeroRows = 1;
	bool conjkern = true;

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
	//apply the speaker correction
	conjkern = true;
	mulSpectrums(outSpectrum, calibrationSpectrum, outSpectrum, flags, conjkern);

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

	//taking only part of the transfer function
	tfMat = outData(Rect(750, 0, dispWidth, 1));

	//apply a shear to the doppler accumulator
    Point2f srcTri[3];
    srcTri[0] = Point2f(0,  dispHeight/2 );
    srcTri[1] = Point2f(1,  dispHeight/2 );
    srcTri[2] = Point2f(0, (dispHeight/2) + 1 );

    Point2f dstTri[3];
    dstTri[0] = Point2f(0,  dispHeight/2 );
    dstTri[1] = Point2f(1,  dispHeight/2 );
    dstTri[2] = Point2f(skewRate, (dispHeight/2) + 1 );

    Mat M = getAffineTransform(srcTri,dstTri);
    warpAffine(tfDoppler, tfDoppler, M, image.size());

	fflush(stdout);

	for(int i=0; i<dispHeight; i++)
	{
		tfDoppler.row(i) =  (dispDecay * tfDoppler.row(i)) + ((1-dispDecay) * tfMat.row(0));
	}
    
	double pulseMax;	//size of the peak
	double pulseMin;	//size of the inverted peak (likely)
	Point pulseMaxT;	//sample number of the peak
	Point pulseMinT;	//sample number of the inverted peak
	Scalar noiseMean(0);	//DC offset, not required
	Scalar noiseStddev(0);	//measurement of the noise floor (RMS)

	minMaxLoc(tfDoppler, &pulseMin, &pulseMax, &pulseMinT, &pulseMaxT);
	meanStdDev(tfDoppler, noiseMean, noiseStddev);
	//cout << "peak at "  << pulseMaxT.x << " height is " << pulseMax << endl;

	double scaleDivisor = 2 * max(pulseMax, -pulseMin);

	Mat tfdopNorm;
    tfDoppler.convertTo(tfdopNorm,CV_8UC1,255.0/scaleDivisor, 127);
    
    applyColorMap(tfdopNorm, image, COLORMAP_JET);

    namedWindow( "Display window", WINDOW_AUTOSIZE );// Create a window for display.
    imshow( "Display window", image );                   // Show our image inside it.
	waitKey(1);

}

