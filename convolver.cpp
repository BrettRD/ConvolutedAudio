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

//void PrepKernel(vector<float> *refSig){
void PrepKernel(fstream *fReference){
	//read the reference signal data into the kernel
	fReference->seekg (0, ios::end);       //return to the beginning of the file

    size_t refsize = fReference->tellg();  //the pointer to the current byte == the length
    cout << "The sample file is " << refsize << " bytes long" << endl;    

	

    //create the matrix of appropriate size
    int cols = 1;
    int rows = refsize / sizeof(int16_t);
    if(rows > fftSize)
    {
	    rows = fftSize;
    }
	cout << "using " << rows << " samples" << endl;    

    kernel = Mat(rows, cols, CV_32F, double(0));	//allocate, and initialize

    fReference->seekg (0, ios::beg);       //return to the beginning of the file

    int16_t sample = 0;	//import sample-by-sample
    int nSamples = rows/sizeof(int16_t);	//bytes were written raw and non-portable
	for(int i=0; i<nSamples; i++){
    	fReference->read ( (char*) &sample, sizeof(int16_t));   //read the whole file
    	float sampleFloat = sample;
    	kernel.at<Vec<float, 1>>(i,0) = sampleFloat;
	}


	//refSig.copyTo(kernel.row(0));
	//refSig.copyTo(kernel);
	//kernel = Mat(*refSig);
	//cout << "refSig size = " << kernel.rows << ", " << kernel.cols << endl;
	cout << "refSig size = " << kernel.size() << endl;

	Scalar noiseMean, noiseStddev;
	meanStdDev(kernel, noiseMean, noiseStddev);
	cout << "noise floor = " << noiseStddev << endl;	
}


//compare the vector to the kernel and report some stats
void SpoolBuffers(float *inputSig, size_t len){

 	cout << "create matrix over input buffer" << endl;
    fflush(stdout);

	sigData = Mat(len, 1, CV_32F, inputSig);	//create a matrix over the array
	//sigData = sigData.t();
	cout << "incoming sig size = " << sigData.size() << endl;

	//run filter2 on the matrix
	int ddepth = -1;	//default to source bit depth
	Point anchor(-1,-1);	//default to kernel centre
	double delta = 0;		//inject a DC offset
	int borderType = BORDER_WRAP;	//assume the reference loops

 	cout << "run the convolution" << endl;
    fflush(stdout);


    //filter2d runs a correlation which is the same
    // equation as a FIR if you limit to 1xN images
    //	this also allows easy exansion of channel or reference count
	filter2D(sigData, outData, ddepth , kernel, anchor, delta, borderType );

 	cout << "matrix size = " << outData.size() << endl;


 	
	//find the maximum, minimum, and variance for each input channel.

	double pulseMax;	//size of the peak
	double pulseMin;	//size of the inverted peak (likely)
	Point pulseMaxT;	//sample number of the peak
	Point pulseMinT;	//sample number of the inverted peak
	Scalar noiseMean(0);	//DC offset, not required
	Scalar noiseStddev(0);	//measurement of the noise floor (RMS)

	cout << "peak detection" << endl;
    fflush(stdout);
	minMaxLoc(outData, &pulseMin, &pulseMax, &pulseMinT, &pulseMaxT, Mat());
	cout << "pulse = "<< pulseMax << " at sample " << pulseMaxT << endl;

	cout << "Noise measurement" << endl;
    fflush(stdout);
	meanStdDev(outData, noiseMean, noiseStddev);
	cout << "noise floor = " << noiseStddev[0] << endl;

	

}
