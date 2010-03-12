#
# Builds OGR LIBKML driver
#
include ../../../GDALmake.opt

CORE_OBJ = ogrlibkmlgeometry.o \
           ogrlibkmlfield.o \
           ogrlibkmlfeature.o \
           ogrlibkmlfeaturestyle.o \
           ogrlibkmllayerstyle.o \
           ogrlibkmldatasetstyle.o \
           ogrlibkmlstyle.o

OBJ = $(OGR_OBJ)

CPPFLAGS :=	-I.. -I../.. $(GDAL_INCLUDE) $(LIBKML_INCLUDE) $(CPPFLAGS)

#CFLAGS	:= $(filter-out -Wall,$(CFLAGS))

default: $(O_OBJ:.o=.$(OBJ_EXT))

clean:
	rm -f *.o $(O_OBJ)

$(O_OBJ):	ogrlibkmlgeometry.h \
          ogrlibkmlfield.h \
          ogrlibkmlfeature.h \
          ogrlibkmlfeaturestyle.h \
          ogrlibkmllayerstyle.h \
          ogrlibkmldatasetstyle.h \
          ogrlibkmlstyle.h \
          
