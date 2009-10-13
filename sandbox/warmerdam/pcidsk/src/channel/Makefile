# Makefile for Channel management code

OBJECTS = $(patsubst %.cpp,%.o,$(wildcard *.cpp))
INCLUDE = -I ..
CFLAGS = -fPIC -O0 -g
CXX = g++


all: segments
    
.cpp.o:
	$(CXX) $(CFLAGS) $(INCLUDE) -c $<

segments: $(OBJECTS)
    
clean:
	-rm *.o
