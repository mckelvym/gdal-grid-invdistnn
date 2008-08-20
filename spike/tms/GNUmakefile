# $Id: GNUmakefile 10656 2007-01-19 01:31:01Z mloskot $
#
# Makefile to build libpng using GNU Make and GCC.
#

include ../../GDALmake.opt

OBJ	= tmsdataset.o tmsrasterbands.o tmsutil.o

CPPFLAGS	:=	$(XTRA_OPT) $(GDAL_INCLUDE) $(CPPFLAGS) -O0

default:	$(OBJ)

clean:
	rm -f *.o $(O_OBJ)

all:	$(OBJ)

install-obj:	$(O_OBJ)

cpp:
	cpp $(GDAL_INCLUDE) tmsdataset.cpp
