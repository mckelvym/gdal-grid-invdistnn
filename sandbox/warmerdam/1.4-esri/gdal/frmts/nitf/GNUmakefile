

include ../../GDALmake.opt

GDAL_OBJ	=	nitfdataset.o
NITFLIB_OBJ	=	nitffile.o nitfimage.o mgrs.o nitfaridpcm.o \
			nitfbilevel.o

OBJ = $(GDAL_OBJ) $(NITFLIB_OBJ)

CPPFLAGS	:=	-I../gtiff/libtiff $(GDAL_INCLUDE) $(CPPFLAGS)

default:	$(OBJ)

clean:
	rm -f *.o $(O_OBJ)

nitfdump:	nitfdump.o $(NITFLIB_OBJ)
	$(CXX) nitfdump.o $(NITFLIB_OBJ) $(CONFIG_LIBS) -o nitfdump

install-obj:	$(O_OBJ)
