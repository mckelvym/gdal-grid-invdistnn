GDAL_ROOT	=	..

GDALMODULE	=	_gdalmodule.so
OBJ        	=       gdal_wrap.o numpydataset.o gdalnumeric.o

include ../GDALmake.opt

default:	$(GDALMODULE)

clean:
	rm -f *.o *.so

%.o:	%.c
	$(CC) -c  $(GDAL_INCLUDE) $(PYTHON_INCLUDES)\
		$(CFLAGS) $(NUMPY_FLAG) $*.c -o $*.o

%.o:	%.cpp
	$(CXX) -c $(GDAL_INCLUDE) $(PYTHON_INCLUDES) \
		 $(CXXFLAGS) $(NUMPY_FLAG) $*.cpp -o $*.o


gdal_wrap.c:	gdal.i
	-swig -python gdal.i

$(GDAL_SLIB):

$(GDALMODULE):	$(OBJ) $(GDAL_LIB)
	$(LD_SHARED) $(OBJ) \
		$(GDAL_SLIB_LINK) $(LIBS) -o $(GDALMODULE)


install:
	$(INSTALL_DIR) $(INST_PYMOD)
	for f in *.py $(GDALMODULE) ; do $(INSTALL) $$f $(INST_PYMOD) ; done
