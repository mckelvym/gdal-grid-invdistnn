

include ../../../GDALmake.opt

OBJ	=	ogrgpxdriver.o ogrgpxdatasource.o ogrgpxlayer.o

ifeq ($(HAVE_EXPAT),yes)
CPPFLAGS +=   -DHAVE_EXPAT
endif

CPPFLAGS	:=	-I.. -I../.. $(EXPAT_INCLUDE) $(GDAL_INCLUDE) $(CPPFLAGS)

default:	$(O_OBJ)

clean:
	rm -f *.o $(O_OBJ)

$(O_OBJ):	ogr_gpx.h

