
lib = -lportaudiocpp -lpthread -lportaudio `pkg-config --libs opencv`

CPPFLAGS = -Wall -pedantic $(inc)
LDFLAGS = $(lib)


all: recordLoopback

recordLoopback: Buffers.o recordLoopback.o RingBuffer.o convolver.o Buffers.h
	g++ -g $(LDFLAGS)  convolver.o RingBuffer.o recordLoopback.o Buffers.o -o recordLoopback  -std=gnu++11

recordLoopback.o: recordLoopback.cpp Buffers.h
	g++ -g $(CPPFLAGS) -c recordLoopback.cpp -std=gnu++11

Buffers.o: Buffers.cpp Buffers.h
	g++ -g $(CPPFLAGS) -c Buffers.cpp -std=gnu++11

RingBuffer.o: RingBuffer.cpp RingBuffer.h Buffers.h
	g++ -g $(CPPFLAGS) -c RingBuffer.cpp -std=gnu++11

convolver.o: convolver.cpp convolver.h Buffers.h
	g++ -g $(CPPFLAGS) -c convolver.cpp -std=gnu++11


clean:
	rm *.o