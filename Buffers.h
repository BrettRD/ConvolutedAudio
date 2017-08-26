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

using namespace std;


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
        
        RingBuffer BufferInput;    //a buffer copying from the input stream.
        RingBuffer BufferOutput; //a buffer writing to the output stream and disk



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