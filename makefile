
lib = -lportaudiocpp -lpthread -lportaudio

CPPFLAGS = -Wall -pedantic $(inc)
LDFLAGS = $(lib)


all: recordLoopback

recordLoopback: Buffers.o recordLoopback.o RingBuffer.o
	g++ -g $(LDFLAGS) RingBuffer.o recordLoopback.o Buffers.o -o recordLoopback  -std=gnu++11

recordLoopback.o: recordLoopback.cpp
	g++ -g $(CPPFLAGS) -c recordLoopback.cpp -std=gnu++11

Buffers.o: Buffers.cpp Buffers.h
	g++ -g $(CPPFLAGS) -c Buffers.cpp -std=gnu++11

RingBuffer.o: RingBuffer.cpp RingBuffer.h
	g++ -g $(CPPFLAGS) -c RingBuffer.cpp -std=gnu++11

clean:
	rm *o