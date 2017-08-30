#ifndef _CONVOLVER_H
#define _CONVOLVER_H

#include <opencv2/opencv.hpp>
#include "Buffers.h"


//void PrepKernel(vector<float> *refSig);
void PrepKernel(fstream *freference);
void SpoolBuffers(vector<float> *inputSig);





#endif