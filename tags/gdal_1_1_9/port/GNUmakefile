#
# CPL (Common Portability Library) makefile
#

GDAL_ROOT = ..

ifneq ($(wildcard ../GDALmake.op?),)
include ../GDALmake.opt
else
include GDALmake.opt
endif

OBJ	=	cpl_conv.o cpl_error.o cpl_string.o cpl_vsisimple.o \
		cplgetsymbol.o cpl_dir.o cpl_path.o cpl_csv.o cpl_findfile.o \
		cpl_vsil_unix_stdio_64.o cpl_minixml.o cpl_multiproc.o \
		cpl_list.o

LIB	=	cpl.a

default:	$(LIB)

clean:
	rm -f *.o $(LIB)

%.o:	%.c
	$(CC) -c $(CFLAGS) $*.c -o $*.o

%.o:	%.cpp
	$(CXX) -c $(CXXFLAGS) $*.cpp -o $*.o

$(LIB): $(OBJ)
	rm -f $(LIB)
	$(AR) rc $(LIB) $(OBJ)
	$(RANLIB) $(LIB)

install:	$(LIB)
	for f in *.h ; do $(INSTALL_DATA) $$f $(INST_INCLUDE) ; done
	$(INSTALL_DATA) cpl.a $(INST_LIB)

xmlreformat:	xmlreformat.o $(LIB)
	$(CXX) $(CXXFLAGS) xmlreformat.o $(LIB) -o xmlreformat
