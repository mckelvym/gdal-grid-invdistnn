
GDAL_ROOT	=	../../..

include $(GDAL_ROOT)/GDALmake.opt

OBJ	=	ogrpgdriver.o ogrpgdatasource.o ogrpglayer.o ogrpgtablelayer.o\
		ogrpgresultlayer.o

ALL_C_FLAGS	=	-I.. -I../.. -I$(GDAL_ROOT)/port \
		$(PG_INC) $(CFLAGS)

default:	$(OBJ)

clean:
	rm -f *.o

%.o:	%.cpp
	$(CXX) -c $(ALL_C_FLAGS) $*.cpp -o $*.o
	ar r ../ogrsf_frmts.a $*.o

%.o:	%.c
	$(CC) -c $(ALL_C_FLAGS) $*.c -o $*.o
	ar r ../ogrsf_frmts.a $*.o

