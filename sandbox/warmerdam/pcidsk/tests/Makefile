
CXXFLAGS = -g -Wall -I../src -I/usr/local/include

OBJ =   test_main.o testpcidskexception.o

LIBS =	../src/pcidsk.a -L/usr/local/lib -lcppunit -ldl 

default:	test_main

test_main:	$(OBJ)
	g++ $(OBJ) $(LIBS) -o test_main

check:  test_main
	./test_main

clean:
	rm -f *.o
	rm -f test_main

%.o:	%.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<
