
GDAL_ROOT	=	../../..

include $(GDAL_ROOT)/GDALmake.opt

OBJ	=	ogrpgdriver.o ogrpgdatasource.o ogrpglayer.o ogrpgtablelayer.o\
		ogrpgresultlayer.o

CPPFLAGS	:=	-I.. -I../.. $(PG_INC) $(CPPFLAGS)

default:	$(O_OBJ)

clean:
	rm -f *.o $(O_OBJ)
