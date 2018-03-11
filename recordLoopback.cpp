// Example program that uses portaudio to stream an input to disk continuously
// via a double buffer while simultaneously running loopback
// This file just gets things spinning
//
// Thanks to Keith Vertanen for his expansion of a PortAudio C++ example

#include <iostream>
#include <cmath>
#include <cassert>
#include <cstddef>

#include <thread>

#include "../portaudiocpp/PortAudioCpp.hxx"
#include "Buffers.h"

// ---------------------------------------------------------------------
using namespace std;


// ---------------------------------------------------------------------


void audioProcessThread(portaudio::System *sys, AudioBuffer *myBuffer, bool *spin){
    while(*spin){
        myBuffer->ProcessBuffers();
        sys->sleep(1);    //Replace with semaphore wait
    }
}



// main:
int main(int argc, char* argv[]);
int main(int argc, char* argv[])
{

    try
    {
        char     chWait;
        int     iInputDevice = -1;
        int     iOutputDevice = -1;

        //open the file with the reference data
        //open with the pointer at the end to measure the length
        fstream fReference( "sample.raw", ios::in|ios::binary); 
        if (fReference)
        {
            PrepKernel(&fReference);     //send it to the correlation engine
            //fReference.close();          //we're done with the file
        }




        //prep the output file and reserve the buffers
        //fstream fout( "input.raw", ios::out|ios::binary);
        fstream fout;


        AudioBuffer myBuffer(frames_per_ring, &fout);
        //AudioBuffer myBuffer(frames_per_buffer, NULL);
        cout << "Setting up PortAudio..." << endl;

        // Set up the System:
        portaudio::AutoSystem autoSys;
        portaudio::System &sys = portaudio::System::instance();

        if (argc > 2)
        {    
            iInputDevice     = atoi(argv[1]); 
            iOutputDevice     = atoi(argv[2]); 

            cout << "Using input device index = " << iInputDevice << endl;
            cout << "Using output device index = " << iOutputDevice << endl;
        }
        else
        {
            cout << "Using system default input/output devices..." << endl;      
            iInputDevice    = sys.defaultInputDevice().index();
            iOutputDevice    = sys.defaultOutputDevice().index();
        }




        // List out all the devices we have
        int     iNumDevices     = sys.deviceCount();
        int     iIndex     = 0;    
        string    strDetails    = "";

        std::cout << "Number of devices = " << iNumDevices << std::endl;    
        if ((iInputDevice >= 0) && (iInputDevice >= iNumDevices))
        {
            cout << "Input device index out of range!" << endl;
            return 0;
        }
        if ((iOutputDevice >= 0) && (iOutputDevice >= iNumDevices))
        {
            cout << "Ouput device index out of range!" << endl;
            return 0;
        }
        for (portaudio::System::DeviceIterator i = sys.devicesBegin(); i != sys.devicesEnd(); ++i)
        {
            strDetails = "";
            if ((*i).isSystemDefaultInputDevice())
                strDetails += ", default input";
            if ((*i).isSystemDefaultOutputDevice())
                strDetails += ", default output";

            cout << (*i).index() << ": " << (*i).name() << ", ";
            cout << "in=" << (*i).maxInputChannels() << " ";
            cout << "out=" << (*i).maxOutputChannels() << ", ";
            cout << (*i).hostApi().name();

            cout << strDetails.c_str() << endl;

            iIndex++;
        }




        //prep the I/O devices
        //This needs a total rewrite:

        cout << "Opening stream on " << sys.deviceByIndex(iInputDevice).name() << endl;
        cout << "Opening playback output stream on " << sys.deviceByIndex(iOutputDevice).name() << endl;
        portaudio::DirectionSpecificStreamParameters instreamParams(
        sys.deviceByIndex(iInputDevice),
        1,
        portaudio::FLOAT32,
        false,
        sys.deviceByIndex(iInputDevice).defaultLowInputLatency(),
        NULL);
        portaudio::DirectionSpecificStreamParameters outStreamParams(
        sys.deviceByIndex(iOutputDevice),
        1,
        portaudio::FLOAT32,
        false,
        sys.deviceByIndex(iOutputDevice).defaultLowOutputLatency(),
        NULL);
        portaudio::StreamParameters streamParams(
        instreamParams,
        outStreamParams,
        sample_rate,
        frames_per_buffer,
        paClipOff);    
        portaudio::MemFunCallbackStream<AudioBuffer> myStream(
        streamParams,
        myBuffer,
        &AudioBuffer::PAcallback);




        cout << "Press enter to STOP recording.";
         //create a thread to run the audio processing
        bool spin = true;
        std::thread processThread(audioProcessThread, &sys, &myBuffer, &spin);
        myStream.start();
        cin.get(chWait);
        spin = false;    //quit the thread loop
        processThread.join();
        myStream.stop();

        myStream.close();
        sys.terminate();
        fReference.close();          //we're done with the file

    }
    catch (const portaudio::PaException &e)
    {
        cout << "A PortAudio error occured: " << e.paErrorText() << endl;
    }
    catch (const portaudio::PaCppException &e)
    {
        cout << "A PortAudioCpp error occured: " << e.what() << endl;
    }
    catch (const exception &e)
    {
        cout << "A generic exception occured: " << e.what() << endl;
    }
    catch (...)
    {
        cout << "An unknown exception occured." << endl;
    }

    return 0;
}


