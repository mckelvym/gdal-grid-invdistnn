

include ../../../GDALmake.opt

CORE_OBJ = 

OGR_OBJ =	ogrnetcdfdriver.o ogrnetcdfdatasource.o ogrnetcdflayer.o

OBJ =	$(CORE_OBJ) $(OGR_OBJ)

CPPFLAGS := -I. -I.. -I../.. -I$(GDAL_ROOT)/port -I$(GDAL_ROOT)/gcore -I$(GDAL_ROOT)/alg \
	-I$(GDAL_ROOT)/ogr -I$(GDAL_ROOT)/ogrsf_frmts $(CPPFLAGS)

#CFLAGS	:=	$(filter-out -Wall,$(CFLAGS))

default:	$(O_OBJ:.o=.$(OBJ_EXT))

clean:
	rm -f *.o $(O_OBJ)

$(O_OBJ):	ogr_libkml.h
          
