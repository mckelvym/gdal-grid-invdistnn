
# Comment out to skip use of libjpeg
JPEG_FLAGS =

CXXFLAGS = -g -Wall -fPIC $(JPEG_FLAGS)

OBJ = channel/*.o core/*.o port/*.o segment/*.o

default:	pcidsk.a pcidsk.so

pcidsk.a: core_comp
	ar r pcidsk.a $(OBJ)

pcidsk.so: core_comp
	g++ -fPIC -ldl -shared -o libpcidsk.so $(OBJ)

clean:
	(cd core; $(MAKE) clean)
	(cd channel; $(MAKE) clean)
	(cd segment; $(MAKE) clean)
	(cd port; $(MAKE) clean)
	-rm -f *.o
	-rm -f *.a
	-rm -f *.so

core_comp:
	(cd core; $(MAKE))
	(cd channel; $(MAKE))
	(cd segment; $(MAKE))
	(cd port; $(MAKE))

check:	default	
	(cd ../tests; $(MAKE) check)

$(OBJ):	pcidsk.h pcidsk_config.h 

%.o:	%.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<

docs:
	doxygen

