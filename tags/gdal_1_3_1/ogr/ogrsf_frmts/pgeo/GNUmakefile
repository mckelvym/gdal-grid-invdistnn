
GDAL_ROOT	=	../../..

include $(GDAL_ROOT)/GDALmake.opt

OBJ	=	ogrpgeodatasource.o ogrpgeolayer.o ogrpgeodriver.o \
		ogrpgeotablelayer.o ogrpgeoselectlayer.o

CPPFLAGS	:=	-I.. -I../.. $(CPPFLAGS)

default:	$(O_OBJ)

clean:
	rm -f *.o $(O_OBJ)
