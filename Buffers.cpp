// Bufers for the PortAudio callback
// double buffering is not really necessary here, however,
// seperate bufers for incoming and outgoing data allows
// significant number-crunching on both incoming and outgoing
// data and means I don't need to do in-place algorithms

#include "Buffers.h"


using namespace std;

AudioBuffer::AudioBuffer(unsigned long iSizeHint, fstream *_fout):
    BufferInput(iSizeHint),
    BufferOutput(iSizeHint),
    BufferRef(referenceMaxDelay),
    underrunFlag{ false },
    freshData{ false },
    fout{ _fout }
{
    size_t writeSize;
    float *writeHead;

    writeSize = BufferRef.contigWrite(&writeHead);  //next write position
    cout << "BufferRef has " << writeSize << " samples free" << endl;
    memset(writeHead, 0, referenceDelay);

    BufferRef.push(NULL, referenceDelay); //advance the reference ring by some delay
    cout << "writing " << referenceDelay << " samples to delay" << endl;

    writeSize = BufferOutput.contigWrite(&writeHead);  //next write position
    cout << "BufferOutput has " << writeSize << " samples free" << endl;
    freshEntropy(writeHead, fftSize);   //new data written directly to output buffer
    BufferRef.push(writeHead,fftSize);   //copied into ref buffer
    BufferOutput.push(NULL, fftSize); //advance the output ring by the amount we wrote
    cout << "writing " << fftSize << " samples" << endl;

    cout << "popping " << fftSize << " samples to kernelBuf" << endl;
    memset(kernelBuf, 0, fftSize + stepOver);
    BufferRef.pop(&kernelBuf[stepOver], fftSize);

    writeSize = BufferOutput.contigWrite(&writeHead);  //next write position
    cout << "BufferOutput has " << writeSize << " samples free" << endl;
    freshEntropy(writeHead, fftSize);   //new data written directly to output buffer
    BufferRef.push(writeHead,fftSize);   //copied into ref buffer
    BufferOutput.push(NULL, fftSize); //advance the output ring by the amount we wrote
    cout << "writing " << fftSize << " samples" << endl;


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

    if(BufferInput.size() >= fftSize){    //number of elements in a callback buffer

        //fetch the pointer to the ring buffer read head
        float *readHead = NULL;
        size_t readSize = BufferInput.contigRead(&readHead);

        size_t writeSize = 0;
        float *writeHead = NULL;

        if(readSize >= fftSize){
            
            //delay the samples going from the reference to the kernel
            //kernelBuf has an addional stepover bytes
            //this allows the delay to be off by stepOver without losing correlation energy
            memmove(kernelBuf, &kernelBuf[fftSize], stepOver*sizeof(float));    //copy the back to the front
            BufferRef.pop(&kernelBuf[stepOver], fftSize);    //copy in the new data

            SpoolBuffers(readHead, kernelBuf, fftSize);

            //we're done with the input data
            BufferInput.pop(NULL,fftSize);

            //copy fresh bytes to the output
            writeSize = BufferOutput.contigWrite(&writeHead);
            if(writeSize >= fftSize)
            {
                freshEntropy(writeHead, fftSize);
                BufferRef.push(writeHead,fftSize);  //we know the data was written contiguously
                BufferOutput.push(NULL,fftSize);
            }
            else
            {
                cout << "output ring has only " << writeSize << " bytes in a row" << endl;
            }



        }
        else
        {
            cout << "Misaligned!" << endl;
        }
    }


    if (underrunFlag)
    {
        cout << "AudioBuffer::ProcessBuffers, output buffer was not filled: " << endl;
        underrunFlag = false;
    }
    if (overrunFlag)
    {
        cout << "AudioBuffer::ProcessBuffers, input buffer was not emptied: " << endl;
        overrunFlag = false;
    }

}

