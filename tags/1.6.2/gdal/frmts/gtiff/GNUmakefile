
include ../../GDALmake.opt

OBJ	=	geotiff.o gt_wkt_srs.o gt_overview.o \
		tif_float.o tifvsi.o

SUBLIBS 	=

ifeq ($(TIFF_SETTING),internal)
SUBLIBS	:= lib-tiff $(SUBLIBS) 
TIFF_OPTS	:=	-Ilibtiff $(TIFF_OPTS)
endif

ifeq ($(GEOTIFF_SETTING),internal)
SUBLIBS	:= lib-geotiff $(SUBLIBS) 
TIFF_OPTS	:=	-Ilibgeotiff $(TIFF_OPTS)
endif

CPPFLAGS	:=	$(GDAL_INCLUDE) $(GEOTIFF_INCLUDE) $(TIFF_OPTS) $(CPPFLAGS)

default:	$(OBJ:.o=.$(OBJ_EXT)) $(SUBLIBS)

clean:
	rm -f *.o $(O_OBJ)
	(cd libtiff; $(MAKE) clean)
	(cd libgeotiff; $(MAKE) clean)

gt_test:	gt_test.$(OBJ_EXT) gt_gs.$(OBJ_EXT) cpl_csv.$(OBJ_EXT)
	$(CC) gt_test.$(OBJ_EXT) gt_gs.$(OBJ_EXT) cpl_csv.$(OBJ_EXT) ../../port/*.$(OBJ_EXT) \
		libgeotiff/libgeotiff.a libtiff/libtiff.a $(LIBS) -o gt_test

gt_write:	gt_write.$(OBJ_EXT) gt_gs.$(OBJ_EXT) cpl_csv.$(OBJ_EXT)
	$(CC) gt_write.$(OBJ_EXT) gt_gs.$(OBJ_EXT) cpl_csv.$(OBJ_EXT) ../../port/*.$(OBJ_EXT) \
		libgeotiff/libgeotiff.a libtiff/libtiff.a $(LIBS) -o gt_write

epsg_to_wkt:	epsg_to_wkt.$(OBJ_EXT) gt_wkt_srs.$(OBJ_EXT) 
	$(CXX) epsg_to_wkt.$(OBJ_EXT) gt_wkt_srs.$(OBJ_EXT) ../../port/*.$(OBJ_EXT) \
	    libgeotiff/libgeotiff.a libtiff/libtiff.a \
	    $(GDAL_LIB) $(LIBS) -o epsg_to_wkt

lib-tiff:
	(cd libtiff; $(MAKE) install-obj)

lib-geotiff:
	(cd libgeotiff; $(MAKE) install-obj)

install-obj:	$(SUBLIBS) $(O_OBJ:.o=.$(OBJ_EXT))
