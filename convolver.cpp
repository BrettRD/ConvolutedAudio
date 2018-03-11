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

	cout << "fetch more kernel" << endl;
    fflush(stdout);

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
		//if(true)
	}
	//cout << "refBuffer[0] = " << refBuffer[0] << endl;

}

//compare the vector to the kernel and report some stats
void SpoolBuffers(float *inputSig, float* kernelBuf, size_t len){

 	//cout << "create matrix over input buffer" << endl;
    fflush(stdout);

	sigData = Mat(len, 1, CV_32F, inputSig);	//create a matrix over the array
	kernel = Mat(len + stepOver, 1, CV_32F, kernelBuf);


	double pulseMax;	//size of the peak
	double pulseMin;	//size of the inverted peak (likely)
	Point pulseMaxT;	//sample number of the peak
	Point pulseMinT;	//sample number of the inverted peak
	Scalar noiseMean(0);	//DC offset, not required
	Scalar noiseStddev(0);	//measurement of the noise floor (RMS)

	//cout << "matrix size = " << tfMat.size() << endl;

	//minMaxLoc(tfMat, NULL, NULL, NULL, NULL);
	minMaxLoc(kernel, &pulseMin, &pulseMax, &pulseMinT, &pulseMaxT);
	cout << "kernel min, max = " << pulseMin << ", " << pulseMax << endl;
	meanStdDev(kernel, noiseMean, noiseStddev);
	cout << "kernel mean, stddiv = " << noiseMean[0] << ", " << noiseStddev[0] << endl;


 	//cout << "run the convolution" << endl;
    fflush(stdout);


	//Convolve(sigData,kernel, outData);
	Convolve();

}

//void Convolve(Mat &sigData, Mat &kern, Mat &outData)
void Convolve()
{
    //filter2d runs a correlation which is the same
    // equation as a FIR if you limit to 1xN images
    //	this also allows easy exansion of channel or reference count
	
	int ddepth = CV_32F;	//default to source bit depth
	Point anchor(0,stepOver);	//default to kernel centre
	double delta = 0;		//inject a DC offset
	int borderType = BORDER_ISOLATED;	//assume the reference loops

	outData = Mat(sigData.size(), CV_32F, Scalar(0));	//create a matrix over the array
	//cout << "incoming sig size = " << sigData.size() << endl;
	//cout << "kernel size = " << kernel.size() << endl;
	//cout << "outgoing size = " << outData.size() << endl;
	filter2D(sigData, outData, ddepth , kernel, anchor, delta, borderType);
	//matchTemplate(kernel, sigData, outData, TM_CCORR);
	//cout << "outgoing size = " << outData.size() << endl;
	//cout << "outgoing[0] = " << outData.at<float>(0) << endl;
	//cout << "sigData[0] = " << sigData.at<float>(0) << endl;
	//cout << "outgoing[0] = " << outData.at<float>(0) << endl;

	double pulseMax;	//size of the peak
	double pulseMin;	//size of the inverted peak (likely)
	minMaxLoc(outData, &pulseMin, &pulseMax, NULL, NULL);
	outData = outData / max(pulseMax, -pulseMin);


    fout.write((char*) outData.ptr(0),  sizeof(float) * fftSize);
    finput.write((char*) sigData.ptr(0),  sizeof(float) * fftSize);
    fkern.write((char*) kernel.ptr(0),  sizeof(float) * (fftSize + stepOver));
  	cout << "fftSize, stepOver = " << fftSize << stepOver << endl;

	cout << "kernel[0] = " << kernel.at<float>(0) << endl;
	cout << "kernel[fftSize + stepOver -1] = " << kernel.at<float>(fftSize + stepOver -1) << endl;

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

	//cout << "matrix size = " << tfMat.size() << endl;

	cout << "peak detection" << endl;
    fflush(stdout);

	minMaxLoc(tfMat, &pulseMin, &pulseMax, &pulseMinT, &pulseMaxT);
	cout << "pulse = "<< pulseMax << " at sample " << pulseMaxT.y << endl;

	cout << "Noise measurement" << endl;
    fflush(stdout);
	meanStdDev(tfMat, noiseMean, noiseStddev);
	cout << "noise floor = " << noiseStddev[0] << endl;






	int height = 512;
	int width = 1024;

	image = Mat(height, width, CV_8U);

	int offset = pulseMaxT.y-(width/2);
	if(offset+width > tfMat.size().height)
	{
		offset = tfMat.size().height - width;
	}
	else if (offset < 0)
	{
		offset = 0;
	}
	cout << "offset = " << offset << endl;


	for(int i=0; i<width; i++)
	{
		int j;
		for(j=0; j<height; j++)
		{
			image.at<uint8_t>(j,i) = 0;
		}

		j = (height/2) + ((height/2) * (tfMat.at<float>(i+offset,0) / (pulseMax-pulseMin)));
		//cout << "marker " << i << ", " << j << endl;
		if(j < 0) j = 0;
		if(j >= height) j = height - 1;
		image.at<uint8_t>(j,i) = 255;
	}

    namedWindow( "Display window", WINDOW_AUTOSIZE );// Create a window for display.
    imshow( "Display window", image );                   // Show our image inside it.
	waitKey(1);

}

