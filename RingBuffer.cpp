
#include "RingBuffer.h"


RingBuffer::RingBuffer(size_t length):
isEmpty(true)
{
    ring = (float*) new float[length * sizeof(float)];

    ringEnd = &(ring[length]);

    writeHead = ring;
    readHead = ring;
}

RingBuffer::~RingBuffer()
{
    delete[] ring;
}

//push one element to the ring
bool RingBuffer::push(float element)
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
size_t RingBuffer::push(float *element, size_t count)
{
    size_t writes = contigWrite(); //how many elements can we write at once?
    if(writes > count){
        writes = count;
    }

    while(writes > 0)
    {
        memcpy(writeHead, element, writes * sizeof(float));
        advanceWrite(writes);   //advance the write pointer
        element += writes;      //advance the read pointer
        count -= writes;        //subtract the written count

        writes = contigWrite(); //how many elements can we write at once?
        if(writes > count){
            writes = count;
        }
    }

    return size();
}


bool RingBuffer::pop(float *element){
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
size_t RingBuffer::pop(float* element, size_t count){
    size_t reads = contigRead(); //how many elements can we write at once?
    if(reads > count){
        reads = count;
    }

    while(reads > 0)
    {
        memcpy(element, readHead, reads * sizeof(float));
        advanceRead(reads);     //advance the write pointer
        element += reads;       //advance the read pointer
        count -= reads;         //subtract the written count

        reads = contigRead();   //how many elements can we write at once?
        if(reads > count){
            reads = count;
        }
    }

    return size();
}


//returns the number of elements in use by the buffer
size_t RingBuffer::size(){
    //return the size of the populated section
    if(isEmpty){
        return 0;
    }else if(readHead >= writeHead){
        return (ringEnd - readHead) + (writeHead - ring);
    }else{
        return writeHead - readHead;
    }
}

size_t RingBuffer::vacant(){
    //return the size of the vacant section
    return (ringEnd - ring) - size();
}

//return the contiguously accessible size from the read pointer
size_t RingBuffer::contigRead(){
    if(isEmpty){
        return 0;    //empty
    }else if(writeHead <= readHead){    //can read up to the write head
        return (ringEnd - readHead);
    }else{                        //can read up to the ring break
        return (writeHead - readHead);
    }
}

//return the contiguously accessible size from the write pointer
size_t RingBuffer::contigWrite(){
    if((!isEmpty) && (readHead == writeHead)){
        return 0;   //full
    }else if(readHead <= writeHead){    //can write up to the read head
        return (ringEnd - writeHead);
    }else{                        //can write up to the ring break
        return (readHead - writeHead);
    }
}

void RingBuffer::advanceWrite(size_t count){
    if(vacant() >= count){
        writeHead += count;
        if(writeHead >= ringEnd) writeHead = ring + (writeHead - ringEnd);

        if(count > 0) isEmpty = false;
    }
}

void RingBuffer::advanceRead(size_t count){
    if(size() >= count){
        readHead += count;
        if(readHead >= ringEnd) readHead = ring + (readHead - ringEnd);

        if((count > 0) && (readHead == writeHead)) isEmpty = true;
    }
}

