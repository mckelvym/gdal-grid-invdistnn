#
# Builds OGR LIBKML driver
#
include ../../../GDALmake.opt

OGR_OBJ = \
	ogrlibkmlgeometry.o \
	ogrlibkmlfield.o \
   	ogrlibkmlfeature.o \
   	ogrlibkmlfeaturestyle.o \
   	ogrlibkmllayerstyle.o \
   	ogrlibkmlstyle.o \
	ogrlibkmldriver.o \
	ogrlibkmldatasource.o \
	ogrlibkmllayer.o

OBJ = $(OGR_OBJ)

CPPFLAGS :=	-I.. -I../.. $(GDAL_INCLUDE) $(LIBKML_INCLUDE) $(CPPFLAGS)

#CFLAGS	:= $(filter-out -Wall,$(CFLAGS))

default: $(O_OBJ:.o=.$(OBJ_EXT))

clean:
	rm -f *.o kmlview $(O_OBJ)

$(O_OBJ): ogrlibkmlgeometry.h \
          ogrlibkmlfield.h \
          ogrlibkmlfeature.h \
          ogrlibkmlfeaturestyle.h \
          ogrlibkmllayerstyle.h \
          ogrlibkmlstyle.h \
