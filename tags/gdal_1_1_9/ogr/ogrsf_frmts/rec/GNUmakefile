
GDAL_ROOT	=	../../..

OBJ	=	ogrrecdriver.o ogrrecdatasource.o ogrreclayer.o

FRMTSLIB = 	../ogrsf_frmts.a

ALL_C_FLAGS	=	-I.. -I../.. -I$(GDAL_ROOT)/port $(CFLAGS)

include $(GDAL_ROOT)/GDALmake.opt

default:	$(OBJ)

clean:
	rm -f *.o

%.o:	%.cpp
	$(CXX) -c $(ALL_C_FLAGS) $*.cpp -o $*.o
	ar r ../ogrsf_frmts.a $*.o
