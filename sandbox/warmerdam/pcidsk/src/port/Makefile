# Makefile for portability abstraction code
OBJECTS = io_stdio.o pthread_mutex.o
CFLAGS = -O0 -g
CXX = g++
INCLUDES = -I..

all: port
	
port: $(OBJECTS)
	
%.o: %.cpp
	$(CXX) -c $(CFLAGS) $(INCLUDES) $<
	
clean:
	-rm -f *.o