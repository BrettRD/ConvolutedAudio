// Bufers for the PortAudio callback
// double buffering is not really necessary here, however,
// seperate bufers for incoming and outgoing data allows
// significant number-crunching on both incoming and outgoing
// data and means I don't need to do in-place algorithms

#include "Buffers.h"


using namespace std;

vector<float> dspBuffer;    //a buffer to send to the DSP algorithms

AudioBuffer::AudioBuffer(unsigned long iSizeHint, fstream *_fout):
    BufferInput(iSizeHint),
    BufferOutput(iSizeHint),
    underrunFlag{ false },
    freshData{ false },
    fout{ _fout }
{

    for(unsigned long i=0; i<iSizeHint; i++)
    {
        BufferOutput.push(0);    //create a data delay from input to output
    }
}


AudioBuffer::~AudioBuffer()
{
    //fout.close(); //fstreams have internal destructors that call close
    //ring buffers' destructors get called automagically
}



int AudioBuffer::PAcallback(const void* pInputBuffer, 
                            void* pOutputBuffer, 
                            unsigned long iFramesPerBuffer, 
                            const PaStreamCallbackTimeInfo* timeInfo, 
                            PaStreamCallbackFlags statusFlags)
{

    
    float **pDataO = (float**) pOutputBuffer;
    float **pDataI = (float**) pInputBuffer;

    size_t remains;
    
    count = iFramesPerBuffer;
    if (pOutputBuffer == NULL){
        cout << "AudioBuffer::PACallback, output buffer was NULL!" << endl;
        return paContinue;
    }

    if (pInputBuffer == NULL){
        cout << "AudioBuffer::PACallback, input buffer was NULL!" << endl;
        return paContinue;
    }

    remains = BufferOutput.pop(pDataO[0], iFramesPerBuffer);
    //cout << "remains = " << remains << endl;
    if(0 != remains ) underrunFlag = true;


    // Copy the samples to the input buffer
    remains = BufferInput.push(pDataI[0], iFramesPerBuffer);
    //cout << "remains = " << remains << endl;
    if(0 != remains ) overrunFlag = true;

    return paContinue;
}


void AudioBuffer::ProcessBuffers()
{
    static int ellipsisCount = 0;    //dodgy print thing
    
    //cout << "AudioBuffer::ProcessBuffers, wait" << endl;
    //unique_lock<mutex> lk(callbackMutex);    //lock the mutex

    if(BufferInput.size() >= fftSize){    //number of elements in a callback buffer

        //fetch the pointer to the ring buffer read head
        float *readHead = NULL;
        size_t readSize = BufferInput.contigRead(&readHead);

        if(readSize >= fftSize){
            
            fout->write((char*) readHead,  sizeof(float) * fftSize);

            SpoolBuffers(readHead, fftSize);

            //copy input to output
            if(0 != BufferOutput.push(readHead,fftSize))
            {
                cout << "output ring is full" << endl;
            }


            BufferInput.pop(NULL,fftSize);

        }
        else
        {
            cout << "Misaligned!" << endl;

        }

        
    }
    //growing ellipsis during recording
    if(++ellipsisCount >= 69){    //10Hz
        //cout << "AudioBuffer::ProcessBuffers, write size:  " << writeCount << endl;

        if (underrunFlag)
        {
            cout << "AudioBuffer::ProcessBuffers, output buffer was not filled: " << endl;
            underrunFlag = false;
        }
        ellipsisCount = 0;
        cout << ".";
        fflush(stdout);
    }
}

