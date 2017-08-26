#ifndef _RING_BUFFER_H_
#define _RING_BUFFER_H_

#include <cmath>
//#include <cassert>
//#include <cstddef>
//#include <cstdlib>
#include <string.h>


class RingBuffer
{
    public:
        RingBuffer(unsigned long length);
        ~RingBuffer();

        //adds a value to the ring buffer
        bool push(float element);
        size_t push(float *element, size_t count);
        
        //returns  the last value on the buffer and advances the pointer
        bool pop(float* element);
        size_t pop(float* element, size_t count);
        //advances a specific head
        //float pop(int readHead);
        
        //arguments: read head number, the number of bytes requested
        //returns a pointer to the read head,
        //writes the number of contiguously accessible bytes into the count
        //advances the read head by count
        //call this function twice to get all of the data
        //float* pop(int readHead, int *count); 



        //returns the number of elements in use by the buffer
        void advanceWrite(size_t count);
        void advanceRead(size_t count);
        size_t size();
        size_t vacant();
        //returns the number of elements between the selected head and the write head
        size_t contigRead();
        size_t contigWrite();


    private:
        float *ring;
        float *ringEnd;
        float *writeHead;
        float *readHead;
        bool isEmpty;
        bool isFull();

};

#endif