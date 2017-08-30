#ifndef _BUFFERS_H_
#define _BUFFERS_H_


//#include <vector>
#include <deque>
#include <iostream>
#include <fstream>
#include <string.h>
#include <cmath>
#include <cassert>
#include <cstddef>
#include <mutex>
#include <thread>
#include <condition_variable>

#include "../portaudiocpp/PortAudioCpp.hxx"

#include "RingBuffer.h"
#include "convolver.h"

using namespace std;

// Some constants:
const int           beep_seconds      = 1;
const double        sample_rate       = 44100.0;
const int           frames_per_buffer = 64;
const unsigned long frames_per_ring   = sample_rate;

extern vector<float> dspBuffer;    //a buffer to send to the DSP algorithms




class AudioBuffer
{
    public:
        AudioBuffer(unsigned long iSizeHint, fstream *_fout);
        ~AudioBuffer();

        int PAcallback(const void* pInputBuffer, 
                            void* pOutputBuffer, 
                            unsigned long iFramesPerBuffer, 
                            const PaStreamCallbackTimeInfo* timeInfo, 
                            PaStreamCallbackFlags statusFlags);

        void ProcessBuffers(void);

    private:
        
        RingBuffer<float> BufferInput;    //a buffer copying from the input stream.
        RingBuffer<float> BufferOutput; //a buffer writing to the output stream and disk


        unsigned long count = 0;

        bool underrunFlag;
        
        //condition_variable flag;
        //mutex callbackMutex;
        bool freshData;// flag to the process function that new data is ready
            //replace this with a semaphore or whatever the posix thing is.

        //std::string strFilename;    //raw file to write
        std::fstream *fout;    //file pointer thing    

};




#endif