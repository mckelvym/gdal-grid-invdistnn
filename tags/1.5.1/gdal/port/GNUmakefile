#
# CPL (Common Portability Library) makefile
#

ifneq ($(wildcard ../GDALmake.op?),)
include ../GDALmake.opt
else
include GDALmake.opt
endif

CPPFLAGS	:= $(CPPFLAGS)	$(CURL_INC)

OBJ =	cpl_conv.o cpl_error.o cpl_string.o cplgetsymbol.o \
	cpl_strtod.o cpl_path.o cpl_csv.o cpl_findfile.o cpl_minixml.o \
	cpl_multiproc.o cpl_list.o cpl_getexecpath.o cplstring.o \
	cpl_vsil_win32.o cpl_vsisimple.o cpl_vsil.o cpl_vsi_mem.o \
	cpl_vsil_unix_stdio_64.o cpl_http.o

ifeq ($(ODBC_SETTING),yes)
OBJ	:= 	$(OBJ) cpl_odbc.o
endif


ifeq ($(CURL_SETTING),yes)
CPPFLAGS	:=	$(CPPFLAGS) -DHAVE_CURL 
endif

default:	$(OBJ)

clean:
	$(RM) *.o $(LIB)

install:
	for f in *.h ; do $(INSTALL_DATA) $$f $(DESTDIR)$(INST_INCLUDE) ; done

xmlreformat:	xmlreformat.o 
	$(CXX) $(CXXFLAGS) xmlreformat.o $(CONFIG_LIBS) -o xmlreformat
