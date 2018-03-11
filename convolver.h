#ifndef _CONVOLVER_H
#define _CONVOLVER_H

#include <opencv2/opencv.hpp>
#include "Buffers.h"

extern cv::Mat kernel;

//void PrepKernel(vector<float> *refSig);
void PrepKernel(fstream *freference);
void SpoolBuffers(float *inputSig, float* kernelBuf, size_t len);

void freshEntropy(float *refBuffer, size_t len);

//void Convolve(cv::Mat &sigData, cv::Mat &kern, cv::Mat &outData);
void Convolve();
void displayTF(cv::Mat &tfMat);
//void displayTF();


#endif