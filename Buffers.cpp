// Bufers for the PortAudio callback
// double buffering is not really necessary here, however,
// seperate bufers for incoming and outgoing data allows
// significant number-crunching on both incoming and outgoing
// data and means I don't need to do in-place algorithms

#include "Buffers.h"



using namespace std;

AudioBuffer::AudioBuffer(unsigned long iSizeHint, fstream *_fout):
	freshData{ false },
	fout{ _fout },
	underrunFlag{ false }
{

	for(int i=0; i<iSizeHint; i++)
	{
		BufferOutput.push_front(0);	//create a data delay from input to output
	}
}


AudioBuffer::~AudioBuffer()
{
	//fout.close(); //fstreams have internal destructors that call close
	BufferInput.clear();
	BufferOutput.clear();
}



int AudioBuffer::PAcallback(const void* pInputBuffer, 
							void* pOutputBuffer, 
							unsigned long iFramesPerBuffer, 
							const PaStreamCallbackTimeInfo* timeInfo, 
							PaStreamCallbackFlags statusFlags)
{

	
	float **pDataO = (float**) pOutputBuffer;
	float **pDataI = (float**) pInputBuffer;
	
	count = iFramesPerBuffer;
	if (pOutputBuffer == NULL)
	{
		cout << "AudioBuffer::PACallback, output buffer was NULL!" << endl;
		return paContinue;
	}

	if (pInputBuffer == NULL)
	{
		cout << "AudioBuffer::PACallback, input buffer was NULL!" << endl;
		return paContinue;
	}


	for (unsigned long i = 0; i < iFramesPerBuffer; i++)
	{
		if(BufferOutput.size() <= 1){	//thread safety is an issue at or below one
			pDataO[0][i] = 0;
			underrunFlag = true;
		}else{
			pDataO[0][i] = BufferOutput.front();
			BufferOutput.pop_front();
		}
	}

//	BufferInput.clear();
	// Copy the samples to the input buffer
	for (unsigned long i = 0; i < iFramesPerBuffer; i++)
	{
		BufferInput.push_back(pDataI[0][i]);
	}

    //unique_lock<mutex> lk(callbackMutex);	//lock the mutex

	//freshData = true;	//the buffers are ready to be crunched
	//flag.notify_one();	//signal to the process thread to begin
	return paContinue;
}


void AudioBuffer::ProcessBuffers()
{
	static int ellipsisCount = 0;	//dodgy print thing
    
	//cout << "AudioBuffer::ProcessBuffers, wait" << endl;
    //unique_lock<mutex> lk(callbackMutex);	//lock the mutex

    //flag.wait(lk, [&](){return freshData;});		//unlock it and wait for the callback to notify_one().
    if(BufferInput.size() >= 64){	//number of elements in a callback buffer

    	int writeCount = 0;
		//write the input buffer to a file
		if(fout){
			while(BufferInput.size() > 1)
			{
				//hideously unoptimized
				float tmp = BufferInput.front();
				BufferInput.pop_front();

				//do something to generate output from input
				BufferOutput.push_back(tmp);

				//write the input to file (hoping fstream consolidates a write buffer)
				fout->write((char*) &tmp,  sizeof(float));
				
				writeCount++;
			}
		}
		
		
		//growing ellipsis during recording
		freshData = false;
		if(++ellipsisCount >= 138){	//10Hz
		    cout << "AudioBuffer::ProcessBuffers, wrote " << writeCount << endl;

			if (underrunFlag)
			{
				cout << "AudioBuffer::ProcessBuffers, output buffer was not filled: " << endl;
				underrunFlag = false;
			}
			ellipsisCount = 0;
			cout << ".";
		}

	}
}

