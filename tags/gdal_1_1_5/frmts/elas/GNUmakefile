
GDAL_ROOT	=	../..

include $(GDAL_ROOT)/GDALmake.opt

OBJ	=	elasdataset.o

default:	$(OBJ)

clean:
	rm -f *.o

install-obj:	$(foreach file,$(OBJ),../o/$(file))

%.o:    %.c
	$(CC) -c $(GDAL_INCLUDE) $(CFLAGS) $*.c -o $*.o

%.o:    %.cpp
	$(CXX) -c $(GDAL_INCLUDE) $(CXXFLAGS) $*.cpp -o $*.o

../o/%.o:	%.o
	cp $*.o ../o


