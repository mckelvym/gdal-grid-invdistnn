
GDAL_ROOT	=	../../..

include $(GDAL_ROOT)/GDALmake.opt

OBJ	=	shape2ogr.o shpopen.o dbfopen.o shptree.o \
		ogrshapedriver.o ogrshapedatasource.o ogrshapelayer.o

ALL_C_FLAGS	=	-I.. -I../.. -I$(GDAL_ROOT)/port $(CFLAGS)



default:	$(OBJ)

clean:
	rm -f *.o

%.o:	%.cpp
	$(CXX) -c $(ALL_C_FLAGS) $*.cpp -o $*.o
	ar r ../ogrsf_frmts.a $*.o

%.o:	%.c
	$(CC) -c $(ALL_C_FLAGS) $*.c -o $*.o
	ar r ../ogrsf_frmts.a $*.o

