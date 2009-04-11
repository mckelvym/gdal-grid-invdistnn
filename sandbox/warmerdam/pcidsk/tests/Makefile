
CXXFLAGS = -g -Wall -I../src -I/usr/local/include

OBJ =   test_main.o testpcidskexception.o testpcidskfile.o

LIBS =	../src/pcidsk.a -L/usr/local/lib -lcppunit -ldl 

default:	test_main

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
