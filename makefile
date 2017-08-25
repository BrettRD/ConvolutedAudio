
lib = -lportaudiocpp -lpthread -lportaudio

CPPFLAGS = $(inc)
LDFLAGS = $(lib)


all: recordLoopback

recordLoopback: Buffers.o recordLoopback.o
	g++ -g $(LDFLAGS) recordLoopback.o Buffers.o -o recordLoopback  -std=gnu++11

recordLoopback.o: recordLoopback.cpp
	g++ -g $(CPPFLAGS) -c recordLoopback.cpp -std=gnu++11

Buffers.o: Buffers.cpp
	g++ -g $(CPPFLAGS) -c Buffers.cpp -std=gnu++11

clean:
	rm *o