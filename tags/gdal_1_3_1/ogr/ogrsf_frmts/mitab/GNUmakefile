
GDAL_ROOT	=	../../..

include $(GDAL_ROOT)/GDALmake.opt

OBJ	=	mitab_rawbinblock.o mitab_mapheaderblock.o \
		mitab_mapindexblock.o mitab_mapobjectblock.o \
		mitab_mapcoordblock.o mitab_feature.o mitab_feature_mif.o \
		mitab_mapfile.o mitab_idfile.o mitab_datfile.o \
		mitab_tabfile.o mitab_miffile.o mitab_utils.o \
		mitab_imapinfofile.o mitab_middatafile.o mitab_bounds.o \
		mitab_maptoolblock.o mitab_tooldef.o mitab_coordsys.o \
		mitab_spatialref.o mitab_ogr_driver.o mitab_indfile.o \
		mitab_tabview.o mitab_ogr_datasource.o mitab_geometry.o \
		mitab_tabseamless.o

CPPFLAGS	:=	-I.. -I../.. $(GDAL_INCLUDE) $(CPPFLAGS) -DOGR

default:	$(O_OBJ)

clean:
	rm -f *.o $(O_OBJ)
