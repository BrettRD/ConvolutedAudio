#ifndef _CONVOLVER_H
#define _CONVOLVER_H

#include <opencv2/opencv.hpp>
#include "Buffers.h"

extern cv::Mat kernel;

//void PrepKernel(vector<float> *refSig);
void PrepKernel(fstream *freference);
void SpoolBuffers(float *inputSig, size_t len);





#endif