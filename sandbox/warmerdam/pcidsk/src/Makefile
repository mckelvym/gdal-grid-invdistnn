CXXFLAGS = -g -Wall

OBJ =   io_stdio.o pthread_mutex.o pcidskinterfaces.o \
	pcidskexception.o cpcidskfile.o pcidskbuffer.o pcidsk_utils.o \
	cpcidskchannel.o cbandinterleavedchannel.o cpixelinterleavedchannel.o \
	cpcidsksegment.o cpcidskgeoref.o sysblockmap.o sysvirtualfile.o \
	ctiledchannel.o

default:	pcidsk.a

pcidsk.a:	$(OBJ)
	ar r pcidsk.a $(OBJ)

clean:
	rm -f *.o
	rm -f *.a

check:	default	
	(cd ../tests; $(MAKE) check)

$(OBJ):	pcidsk.h pcidsk_p.h pcidsk_config.h 

%.o:	%.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<
