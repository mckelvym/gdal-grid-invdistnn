
GDAL_ROOT	=	../../..

include $(GDAL_ROOT)/GDALmake.opt

OBJ	=	ogrfmecacheindex.o ogrfmedatasource.o ogrfmedriver.o \
		ogrfmelayer.o ogrfmelayercached.o ogrfmelayerdb.o \
		fme2ogr_utils.o

CPPFLAGS	+=	-I.. -I../.. $(FME_INCLUDE) \
		-DSUPPORT_INDIRECT_FMEDLL -DSUPPORT_PERSISTENT_CACHE

default:	$(O_OBJ)

clean:
	rm -f *.o $(O_OBJ)
