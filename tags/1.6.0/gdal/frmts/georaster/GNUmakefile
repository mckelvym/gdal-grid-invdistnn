include ../../GDALmake.opt

ifeq ($(LIBZ_SETTING),internal)
	XTRA_OPT =	-I../zlib
else
	XTRA_OPT =
endif

OBJ		=	georaster_driver.o \
			georaster_dataset.o \
			georaster_rasterband.o \
			oci_wrapper.o \
			georaster_wrapper.o

CPPFLAGS	:=	$(XTRA_OPT) $(GDAL_INCLUDE) $(OCI_INCLUDE) $(CPPFLAGS)

PLUGIN_SO	= gdal_GEOR.so

default:	$(OBJ)

clean:
	rm -f *.o $(O_OBJ) *.so

install-obj:	$(O_OBJ)

plugin: $(PLUGIN_SO)

$(PLUGIN_SO):	$(OBJ)
	gcc -shared $(LNK_FLAGS) $(OBJ) $(CONFIG_LIBS_INS) $(EXTRA_LIBS) \
		-o $(PLUGIN_SO)
