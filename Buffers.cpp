// Bufers for the PortAudio callback
// double buffering is not really necessary here, however,
// seperate bufers for incoming and outgoing data allows
// significant number-crunching on both incoming and outgoing
// data and means I don't need to do in-place algorithms

#include "Buffers.h"



using namespace std;

AudioBuffer::AudioBuffer(int iSizeHint, fstream *_fout):
	freshData{ false },
	fout{ _fout },
	underrunFlag{ false }
{

	if (iSizeHint > 0)
	{
		BufferInput.reserve(iSizeHint);
		BufferOutput.reserve(iSizeHint);
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

	// Copy the write buffer to the output device
	//std::vector<float>::iterator
	auto it = BufferOutput.begin();
//	auto it = BufferInput.begin();
	for (unsigned long i = 0; i < iFramesPerBuffer; i++)
	{
		if(it == BufferOutput.end()){
			pDataO[0][i] = 0;
			underrunFlag = true;
		}else{
			pDataO[0][i] = *(it++);
		}
	}

//	BufferInput.clear();
	// Copy the samples to the input buffer
	for (unsigned long i = 0; i < iFramesPerBuffer; i++)
	{
		BufferInput.push_back(pDataI[0][i]);
	}

    unique_lock<mutex> lk(callbackMutex);	//lock the mutex

	freshData = true;	//the buffers are ready to be crunched
	flag.notify_one();	//signal to the process thread to begin
	return paContinue;
}


void AudioBuffer::ProcessBuffers()
{
	static int ellipsisCount = 0;	//dodgy print thing
    
	//cout << "AudioBuffer::ProcessBuffers, wait" << endl;
    unique_lock<mutex> lk(callbackMutex);	//lock the mutex

    flag.wait(lk, [&](){return freshData;});		//unlock it and wait for the callback to notify_one().
    if(freshData){

		//write the input buffer to a file
		if(fout){
			fout->write((char*) &(*BufferInput.begin()), BufferInput.size() * sizeof(float));
		}
		
			//probably worth checking BufferInput.capacity() during the callback to verify
			//if it gets reallocated, we can call BufferInput.reserve() here

		//Simple loopback:
		//just swap the buffers.

		//BufferInput.swap(BufferOutput);

	/*
		auto it = BufferOutput.begin();
		//	auto it = BufferInput.begin();
		for (unsigned long i = 0; i < iFramesPerBuffer; i++)
		{
			if(it == BufferOutput.end()){

		}
		BufferOutput.clear();	//reallocation is not guaranteed to occur (we hope it doesn't)		
	*/
		
		//growing ellipsis during recording
		freshData = false;
		if(++ellipsisCount >= 138){	//10Hz
		    cout << "AudioBuffer::ProcessBuffers, input =" << count << endl;

			if (underrunFlag)
			{
				cout << "AudioBuffer::ProcessBuffers, output buffer was not filled: " << BufferInput.size() << endl;
				underrunFlag = false;
			}
			ellipsisCount = 0;
			cout << ".";
		}


		BufferInput.clear();	//reallocation is not guaranteed to occur (we hope it doesn't)
		BufferInput.reserve(256);
	}
}

