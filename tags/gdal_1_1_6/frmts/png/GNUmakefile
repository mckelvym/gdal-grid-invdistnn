GDAL_ROOT	=	../..

include $(GDAL_ROOT)/GDALmake.opt

ifeq ($(PNG_SETTING),internal)
XTRA_OPT =	-DPNG_NO_GLOBAL_ARRAYS -Ilibpng
OBJ	=	png.o pngerror.o pnggccrd.o pngget.o pngmem.o \
		pngpread.o pngread.o pngrio.o pngrtran.o pngrutil.o \
		pngset.o pngtrans.o pngvcrd.o pngwio.o pngwrite.o \
		pngwtran.o pngwutil.o \
		\
		pngdataset.o
else
OBJ	=	pngdataset.o
endif

XTRA_OPT	:=	$(XTRA_OPT) -I../zlib

default:	$(OBJ)

clean:
	rm -f *.o

%.o:	%.c
	$(CC) -c $(XTRA_OPT) $(GDAL_INCLUDE) $(CFLAGS) $*.c -o $*.o

%.o:	libpng/%.c
	$(CC) -c $(XTRA_OPT) $(GDAL_INCLUDE) $(CFLAGS) libpng/$*.c -o $*.o

%.o:	%.cpp
	$(CXX) -c $(XTRA_OPT) $(GDAL_INCLUDE) $(CXXFLAGS) $*.cpp -o $*.o


all:	$(OBJ)

install-obj:	$(foreach file,$(OBJ),../o/$(file))

../o/%.o:	%.o
	cp $*.o ../o
