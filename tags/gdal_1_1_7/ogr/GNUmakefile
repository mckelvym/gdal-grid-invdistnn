GDAL_ROOT	=	..

include ../GDALmake.opt

OBJ	=	ogrgeometryfactory.o ogrpoint.o ogrcurve.o ogrlinestring.o \
		ogrlinearring.o ogrpolygon.o ogrutils.o ogrgeometry.o \
		ogrgeometrycollection.o ogrmultipolygon.o ogrsurface.o \
		ogrmultipoint.o ogrmultilinestring.o \
		\
		ogrfeature.o ogrfeaturedefn.o ogrfeaturestyle.o ogrfielddefn.o\
		ogrspatialreference.o ogr_srsnode.o ogr_srs_proj4.o \
		ogr_fromepsg.o ogrct.o ogr_opt.o ogr_srs_esri.o \
		ogrfeaturequery.o swq.o ogr_srs_validate.o ogr_srs_xml.o \
		ograssemblepolygon.o

INST_H_FILES	=	ogr_core.h ogr_feature.h ogr_geometry.h ogr_p.h \
		ogr_spatialref.h ogr_srs_api.h ogrsf_frmts/ogrsf_frmts.h \
		ogr_featurestyle.h

OGR_LIB  =	ogr.a

LIBS	:=	$(OGR_LIB) $(CPL_LIB) $(LIBS) $(PG_LIB) $(OGDI_LIBS)

FRMTS_LIB = 	ogrsf_frmts/ogrsf_frmts.a

LINCLUDE =	-Iogrsf_frmts -I. -I$(GDAL_ROOT)/port 

ifeq ($(OGDI_FLAG),ENABLED)
CFLAGS   :=	$(CFLAGS) -DOGDI_ENABLED
CXXFLAGS :=	$(CXXFLAGS) -DOGDI_ENABLED
endif

default:	lib ogrinfo ogr2ogr

all:	sublibs test1 test2 test_ogrsf ogrinfo ogr2ogr docs ogrtindex

apps:	ogrinfo ogr2ogr

clean:
	rm -f *.o test1 test2 
	(cd ogrsf_frmts; $(MAKE) clean)

very-clean: clean
	rm -rf html

test1:	test1.o $(OGR_LIB)
	$(CXX) $(LNK_FLAGS) test1.o $(LIBS) -o test1

test2:	test2.o $(OGR_LIB) $(FRMTS_LIB)
	$(CXX) $(LNK_FLAGS) test2.o $(FRMTS_LIB) $(LIBS) -o test2

test_ogrsf:	test_ogrsf.o $(OGR_LIB) $(FRMTS_LIB)
	$(CXX) $(LNK_FLAGS) test_ogrsf.o $(FRMTS_LIB) $(LIBS) -o test_ogrsf

ogrinfo:	ogrinfo.o $(OGR_LIB) $(FRMTS_LIB)
	$(CXX) $(LNK_FLAGS) ogrinfo.o $(FRMTS_LIB) $(LIBS) -o ogrinfo

ogr2ogr:	ogr2ogr.o $(OGR_LIB) $(FRMTS_LIB)
	$(CXX) $(LNK_FLAGS) ogr2ogr.o $(FRMTS_LIB) $(LIBS) -o ogr2ogr

sdts2mi:	sdts2mi.o $(OGR_LIB) $(FRMTS_LIB)
	$(CXX) $(LNK_FLAGS) sdts2mi.o $(FRMTS_LIB) $(LIBS) -o sdts2mi

testepsg:	testepsg.o $(OGR_LIB) $(FRMTS_LIB)
	$(CXX) $(LNK_FLAGS) testepsg.o $(FRMTS_LIB) $(LIBS) -o testepsg

ogrtindex:	ogrtindex.o $(OGR_LIB) $(FRMTS_LIB)
	$(CXX) $(LNK_FLAGS) ogrtindex.o $(FRMTS_LIB) $(LIBS) -o ogrtindex

$(FRMTS_LIB):
	(cd ogrsf_frmts; $(MAKE))

lib:	$(OGR_LIB) 

sublibs:
	(cd ogrsf_frmts; $(MAKE))

docs:
	rm -rf html/*
	doxygen


$(OGR_LIB):	$(OBJ)
	$(AR) r $(OGR_LIB) $?
	$(RANLIB) $(OGR_LIB)

%.o:	%.c
	$(CC) -c $(LINCLUDE) $(CFLAGS) $*.c -o $*.o

%.o:	%.cpp
	$(CXX) -c $(LINCLUDE) $(CXXFLAGS) $*.cpp -o $*.o

web-update:	docs
	cp html/* /u/www/projects/opengis/ogrhtml
	cp ogrsf_frmts/*/drv_*.html /u/www/projects/opengis/ogrhtml
	cp ogrsf_frmts/ogr_formats.html /u/www/projects/opengis/ogrhtml

test:	reg-test reg-test2

reg-test:
	@rm -f s.out
	@./reg_test.sh > s.out
	@if test "`diff s.out reg_test.log`" = '' ; then \
	    echo "******* Results Match Expected *********"; \
	    grep FAILURE s.out; \
	    rm s.out; \
	else \
	    echo "******* Results Don't Match Expected *********"; \
	    diff s.out reg_test.log; \
	fi

reg-test2:
	@rm -f s2.out
	@./reg_test2.sh > s2.out
	@if test "`diff s2.out reg_test2.log`" = '' ; then \
	    echo "******* Results Match Expected *********"; \
	    grep FAILURE s2.out; \
	    rm s2.out; \
	else \
	    echo "******* Results Don't Match Expected *********"; \
	    diff s2.out reg_test2.log; \
	fi

ifeq ($(OGR_ENABLED),yes)
install-full:	ogrinfo ogr2ogr ogrtindex
	$(INSTALL) ogr2ogr $(INST_BIN)
	$(INSTALL) ogrinfo $(INST_BIN)
	$(INSTALL) ogrtindex $(INST_BIN)
	$(INSTALL) $(OGRFRMTS_LIB) $(INST_LIB)
else
install-full:
endif

install:	$(OGR_LIB) install-full
	for f in $(INST_H_FILES) ; do $(INSTALL) $$f $(INST_INCLUDE) ; done
	$(INSTALL) $(OGR_LIB) $(INST_LIB)


