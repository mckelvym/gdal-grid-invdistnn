GDAL_ROOT	=	../..

include ../../GDALmake.opt

GDALMODULE	=	_gdalmodule
GDALMODULE_SO   =	_gdalmodule.$(PY_SO_EXT)

OBJ        	=       gdal_wrap.o numpydataset.o gdalnumeric.o

CPPFLAGS	:=	$(GDAL_INCLUDE) $(PYTHON_INCLUDES) $(CPPFLAGS)
CFLAGS		:=	$(PYTHON_CFLAGS) $(CFLAGS)
CXXFLAGS	:=	$(PYTHON_CFLAGS) $(CXXFLAGS)

PY_COMMANDS	=	epsg_tr.py gdalchksum.py gdal2xyz.py gcps2wld.py \
			gdalimport.py gdal_merge.py pct2rgb.py rgb2pct.py \
			gcps2vec.py
PY_MODULES	=	gdal.py ogr.py osr.py gdalconst.py gdalnumeric.py

# this is needed during install due to the _gdalmodule.so library
# being relinked by libtool
ifeq ($(HAVE_LIBTOOL),yes)
LIBS	+=	-L$(GDAL_ROOT)/.libs
endif

default:	$(GDALMODULE_SO)

clean:
	$(RM) *.o *.$(SO_EXT) *.la

gdal_wrap.cpp:	gdal.i
	/usr/bin/swig -I$(GDAL_ROOT) -c++ -python -o gdal_wrap.cpp gdal.i 

$(GDALMODULE_SO):	$(OBJ) $(EXE_DEP_LIBS)
	$(PY_LD_SHARED) $(OBJ) \
		$(GDAL_SLIB_LINK) $(PYTHON_LIBS) $(CONFIG_LIBS) -o $@

%.lo:	%.o ;

$(GDALMODULE).la:	$(OBJ:.o=.lo) $(EXE_DEP_LIBS)
	$(LD) $(OBJ:.o=.lo) $(PYTHON_LIBS) $(CONFIG_LIBS) -o $@ \
		-rpath $(INST_PYMOD) \
		-no-undefined \
		-avoid-version \
		-module

# special care is taken so that python scripts are installed as executables
install:	$(GDALMODULE_SO)
	$(INSTALL_DIR) $(INST_PYMOD)
	$(INSTALL_LIB) $(GDALMODULE_SO) $(INST_PYMOD)
	for f in $(PY_COMMANDS) ; do $(INSTALL) $$f $(INST_BIN) ; done
	for f in $(PY_MODULES) ; do $(INSTALL_DATA) $$f $(INST_PYMOD) ; done
	# ugh! spurius relinking leaves a file owned by root into .libs/.
	rm -f .libs/*.soT
