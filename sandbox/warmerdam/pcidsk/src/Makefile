CXXFLAGS = -g -Wall

OBJ =   pcidskobject.o io_stdio.o pthread_mutex.o pcidskinterfaces.o \
	pcidskexception.o

default:	pcidsk.a

pcidsk.a:	$(OBJ)
	ar r pcidsk.a $(OBJ)

clean:
	rm -f *.o
	rm -f *.a

$(OBJ):	pcidsk.h pcidsk_config.h 

%.o:	%.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<
