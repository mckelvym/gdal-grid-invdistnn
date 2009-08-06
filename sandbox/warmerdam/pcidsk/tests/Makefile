
CXXFLAGS = -g -Wall -I../src -I/usr/local/include

OBJ =   test_main.o testpcidskexception.o testpcidskfile.o testsegments.o \
	testpcidskcreate.o testpcidskupdate.o

LIBS =	../src/pcidsk.a -L/usr/local/lib -ljpeg -lcppunit -ldl 

default:	test_main pcidsk_read pcidsk2pcidsk

pcidsk_read:	pcidsk_read.o  ../src/pcidsk.a 
	g++ pcidsk_read.o $(LIBS) -o pcidsk_read

pcidsk2pcidsk:	pcidsk2pcidsk.o  ../src/pcidsk.a 
	g++ pcidsk2pcidsk.o $(LIBS) -o pcidsk2pcidsk

test_main:	$(OBJ) ../src/pcidsk.a 
	g++ $(OBJ) $(LIBS) -o test_main

../src/pcidsk.a:
	(cd ../src ; $(MAKE))

check:  test_main
	./test_main

clean:
	rm -f *.o
	rm -f test_main

%.o:	%.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<
