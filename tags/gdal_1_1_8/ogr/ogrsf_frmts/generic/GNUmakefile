
GDAL_ROOT	=	../../..

include $(GDAL_ROOT)/GDALmake.opt

OBJ	=	ogrsfdriverregistrar.o ogrlayer.o ogrdatasource.o \
		ogrsfdriver.o ogrregisterall.o ogr_gensql.o

ifeq ($(HAVE_OGDI),yes)
CFLAGS   :=	$(CFLAGS) -DOGDI_ENABLED
CXXFLAGS :=	$(CXXFLAGS) -DOGDI_ENABLED
endif

ifeq ($(HAVE_OGR_PG),yes)
CFLAGS   :=	$(CFLAGS) -DPG_ENABLED
CXXFLAGS :=	$(CXXFLAGS) -DPG_ENABLED
endif

ifeq ($(HAVE_FME),yes)
CFLAGS   :=	$(CFLAGS) -DFME_ENABLED
CXXFLAGS :=	$(CXXFLAGS) -DFME_ENABLED
endif

ALL_C_FLAGS	=	-I.. -I../.. -I$(GDAL_ROOT)/port $(CFLAGS)



default:	$(OBJ)

clean:
	rm -f *.o

%.o:	%.cpp
	$(CXX) -c $(ALL_C_FLAGS) -DINST_DATA=\"$(INST_DATA)\" $*.cpp -o $*.o
	ar r ../ogrsf_frmts.a $*.o

%.o:	%.c
	$(CC) -c $(ALL_C_FLAGS) $*.c -o $*.o
	ar r ../ogrsf_frmts.a $*.o

