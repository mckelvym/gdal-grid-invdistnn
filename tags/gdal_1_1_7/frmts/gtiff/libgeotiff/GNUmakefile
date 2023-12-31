GDAL_ROOT	=	../../..

include $(GDAL_ROOT)/GDALmake.opt

OBJ	= \
	xtiff.o \
	geo_free.o \
	geo_get.o \
	geo_names.o \
	geo_new.o \
	geo_print.o \
	geo_set.o \
	geo_tiffp.o \
	geo_write.o \
	geo_normalize.o \
	geotiff_proj4.o \
	geo_extra.o \
	geo_trans.o

O_OBJ	=	$(foreach file,$(OBJ),../../o/$(file))

ALL_C_FLAGS	=	$(CFLAGS)

ifeq ($(TIFF_SETTING),internal)
ALL_C_FLAGS	:=	-I../libtiff $(ALL_C_FLAGS)
endif


default:	$(OBJ)

clean:
	rm -f $(O_OBJ) *.o *.a

install-obj:	$(O_OBJ)

../../o/%.o:	%.o
	cp $*.o ../../o

%.o:	%.c
	$(CC) -c -I$(GDAL_ROOT)/port $(ALL_C_FLAGS) $*.c -o $*.o

#
# Updating to the latest libgeotiff involves copying all matching source
# except for a few files that hook to GDALs own CPL services.
#
import:
	@if test ! -d ~/osrs/geotiff ; then \
	  echo reimport requires libgeotiff checked out ~/osrs/geotiff ; \
	  exit 1; \
	fi

	rm -rf safe
	mkdir safe
	mv cpl_serv.h geo_config.h safe

	copymatch.sh ~/osrs/geotiff/libgeotiff *.cpp *.c *.h *.inc
	copymatch.sh ~/osrs/geotiff/libgeotiff/libxtiff xtiff.c xtiffio.h

	mv safe/* .
	rm -rf safe

	@echo
	@echo 'Now do something like:'
	@echo '% cvs commit -m "updated to libgeotiff 1.1.x"'
	@echo
