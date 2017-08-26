
#include "RingBuffer.h"

using namespace std;
//list off the types that the ringbuffer is can be built for
template class RingBuffer<float>;



template <typename T>
RingBuffer<T>::RingBuffer(size_t length):
isEmpty(true)
{
    ring = (T*) new T[length * sizeof(T)];

    ringEnd = &(ring[length]);

    writeHead = ring;
    readHead = ring;
}

template <typename T>
RingBuffer<T>::~RingBuffer()
{
    delete[] ring;
}

//push one element to the ring
template <typename T>
bool RingBuffer<T>::push(T element)
{
    if(vacant() > 0){    
        *writeHead = element;
        advanceWrite(1);
        return true;
    }
    return false;
}


//arguments:
//pointer to read values from
//number of elements to read
//returns number of elements actually pushed
template <typename T>
size_t RingBuffer<T>::push(T *element, size_t count)
{
    size_t writes = min(contigWrite(), count); //how many elements can we write at once?

    while(writes > 0)
    {
        memcpy(writeHead, element, writes * sizeof(T));
        advanceWrite(writes);   //advance the write pointer
        element += writes;      //advance the read pointer
        count -= writes;        //subtract the written count

        writes = min(contigWrite(), count); //how many elements can we write at once?
    }

    return size();
}


template <typename T>
bool RingBuffer<T>::pop(T *element){
    if(size()>0){
        *element = *readHead;
        advanceRead(1);
        return true;
    }
    return false;
}




//arguments:
//pointer to write values to
//number of elements to write        
//returns number of elements actually popped
template <typename T>
size_t RingBuffer<T>::pop(T* element, size_t count){
    size_t reads = min(contigRead(), count); //how many elements can we write at once?

    while(reads > 0)
    {
        memcpy(element, readHead, reads * sizeof(T));
        advanceRead(reads);     //advance the write pointer
        element += reads;       //advance the read pointer
        count -= reads;         //subtract the written count

        reads = min(contigRead(), count);   //how many elements can we write at once?
    }

    return size();
}




//returns the number of elements in use by the buffer
template <typename T>
size_t RingBuffer<T>::size(){
    //return the size of the populated section
    if(isEmpty){
        return 0;
    }else if(readHead >= writeHead){
        return (ringEnd - readHead) + (writeHead - ring);
    }else{
        return writeHead - readHead;
    }
}



template <typename T>
size_t RingBuffer<T>::vacant(){
    //return the size of the vacant section
    return (ringEnd - ring) - size();
}

//return the contiguously accessible size from the read pointer
template <typename T>
size_t RingBuffer<T>::contigRead(){
    if(isEmpty){
        return 0;    //empty
    }else if(writeHead <= readHead){    //can read up to the write head
        return (ringEnd - readHead);
    }else{                        //can read up to the ring break
        return (writeHead - readHead);
    }
}



//return the contiguously accessible size from the write pointer
template <typename T>
size_t RingBuffer<T>::contigWrite(){
    if((!isEmpty) && (readHead == writeHead)){
        return 0;   //full
    }else if(readHead <= writeHead){    //can write up to the read head
        return (ringEnd - writeHead);
    }else{                        //can write up to the ring break
        return (readHead - writeHead);
    }
}



template <typename T>
void RingBuffer<T>::advanceWrite(size_t count){
    if(vacant() >= count){
        writeHead += count;
        if(writeHead >= ringEnd) writeHead = ring + (writeHead - ringEnd);

        if(count > 0) isEmpty = false;
    }
}



template <typename T>
void RingBuffer<T>::advanceRead(size_t count){
    if(size() >= count){
        readHead += count;
        if(readHead >= ringEnd) readHead = ring + (readHead - ringEnd);

        if((count > 0) && (readHead == writeHead)) isEmpty = true;
    }
}

