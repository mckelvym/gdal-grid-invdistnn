
GDAL_ROOT	=	../../..

include $(GDAL_ROOT)/GDALmake.opt

CORE_OBJ =	gmlpropertydefn.o gmlfeatureclass.o gmlfeature.o gmlreader.o

OGR_OBJ =	gml2ogrgeometry.o ogr2gmlgeometry.o \
		ogrgmldriver.o ogrgmldatasource.o ogrgmllayer.o

ALL_C_FLAGS	=	$(XERCES_INCLUDE) -I.. -I../.. -I$(GDAL_ROOT)/port \
	$(filter-out -Wall,$(CFLAGS))

ifeq ($(HAVE_XERCES),yes)
CORE_OBJ :=	$(CORE_OBJ) gmlreadstate.o gmlhandler.o trstring.o
ALL_C_FLAGS :=  -DHAVE_XERCES=1 $(ALL_C_FLAGS)
else
ALL_C_FLAGS :=  -DHAVE_XERCES=0 $(ALL_C_FLAGS)
endif

default:	$(CORE_OBJ) $(OGR_OBJ)

clean:
	rm -f *.o gmlview

%.o:	%.cpp
	$(CXX) -c $(ALL_C_FLAGS) $*.cpp -o $*.o
	ar r ../ogrsf_frmts.a $*.o

%.o:	%.c
	$(CC) -c $(ALL_C_FLAGS) $*.c -o $*.o
#	ar r ../ogrsf_frmts.a $*.o

gmlview:	gmlview.cpp $(CORE_OBJ)
	$(CXX) $(ALL_C_FLAGS) $(LNK_FLAGS) \
		gmlview.cpp gml2ogrgeometry.o $(CORE_OBJ) \
		../ogrsf_frmts.a ../../ogr.a $(CPL_LIB) $(LIBS) \
		-o gmlview


xtest:	xtest.cpp
	$(CXX) $(ALL_C_FLAGS) $(LNK_FLAGS) xtest.cpp \
		$(LIBS) -o xtest

