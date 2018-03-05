#ifndef _RING_BUFFER_H_
#define _RING_BUFFER_H_

#include <cmath>
//#include <cassert>
//#include <cstddef>
//#include <cstdlib>
#include <string.h>
#include <algorithm>

using namespace std;

template <typename T>
class RingBuffer
{
    public:
        RingBuffer(unsigned long length);
        ~RingBuffer();

        //adds a value to the ring buffer
        bool push(T element);
        size_t push(T *element, size_t count);
        
        //returns the last value on the buffer and advances the pointer
        bool pop(T* element);
        size_t pop(T* element, size_t count);


        void advanceWrite(size_t count);
        void advanceRead(size_t count);
        size_t size();
        size_t vacant();
        
        size_t contigRead(T**);
        size_t contigWrite(T**);


    private:
        T *ring;
        T *ringEnd;
        T *writeHead;
        T *readHead;
        bool isEmpty;
        bool isFull();

};

#endif