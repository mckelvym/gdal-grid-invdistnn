VERSION =	1_1
DISTDIR =	sdts_$(VERSION)
WEB_DIR =	/u/www/projects/sdts

SHPDIR		=	../shapelib
ISO8211DIR	=	../iso8211
GDAL_ROOT	=	../..

include $(GDAL_ROOT)/GDALmake.opt

OBJ	=	sdtsiref.o sdtscatd.o sdtslinereader.o sdtslib.o \
		sdtspointreader.o sdtsattrreader.o sdtstransfer.o \
		sdtspolygonreader.o sdtsxref.o sdtsrasterreader.o \
		sdtsindexedreader.o

ALL_CXX_FLAGS =	-I$(ISO8211DIR) $(GDAL_INCLUDE) $(CXXFLAGS)

SDTSLIB	=	libsdts_al.a
LIBS	:=	$(SDTSLIB) $(ISO8211DIR)/libiso8211.a $(CPL_LIB) $(LIBS)



default:	$(SDTSLIB) sdtsdataset.o

all:	$(SDTSLIB) sdts2shp

clean:	clean-dist
	rm -rf *.o sdts2shp html man

clean-dist:
	rm -rf $(DISTDIR) $(DISTDIR).zip $(DISTDIR).tar.gz

%.o:	%.cpp
	$(CXX) -c $(ALL_CXX_FLAGS) -I$(GDAL_ROOT)/core -I$(GDAL_ROOT)/port \
		$*.cpp -o $*.o

$(SDTSLIB):	$(OBJ)
	ar r $(SDTSLIB) $?

sdts2shp:	sdts2shp.cpp $(SDTSLIB)
	$(CXX) $(ALL_CXX_FLAGS) -I$(SHPDIR) sdts2shp.cpp \
		$(SHPDIR)/shpopen.o $(SHPDIR)/dbfopen.o \
		$(LIBS) -o sdts2shp

install-obj:	$(SDTSLIB) $(foreach file,$(OBJ),../o/$(file)) ../o/sdtsdataset.o

../o/%.o:	%.o
	cp $*.o ../o

%.o:	%.cpp
	$(CXX) -c $(ALL_CXX_FLAGS) $*.cpp -o $*.o

docs:
	rm -rf html
	mkdir html
	doxygen
	rm html/index.html
	cp html/sdts_al_main.html html/index.html

dist:	docs
	rm -rf $(DISTDIR)
	mkdir $(DISTDIR)
	mkdir $(DISTDIR)/html
	cp html/* $(DISTDIR)/html
	autoconf
	cp *.cpp *.h configure Makefile.in $(DISTDIR)
	cp makefile.vc.dist $(DISTDIR)/Makefile.vc
	rm $(DISTDIR)/sdtsdataset.cpp
	cp $(ISO8211DIR)/{*.cpp,*.h} $(DISTDIR)
	rm configure
	cp $(GDAL_ROOT)/port/{cpl_error*,cpl_port*,cpl_string*} $(DISTDIR)
	cp $(GDAL_ROOT)/port/{cpl_vsisimple.cpp,cpl_config.h.in} $(DISTDIR)
	cp $(GDAL_ROOT)/port/{cpl_vsi.h,cpl_conv.*} $(DISTDIR)
	cp $(GDAL_ROOT)/port/cpl_config.h.in $(DISTDIR)/cpl_config.h
	cp $(SHPDIR)/{shpopen.c,dbfopen.c,shapefil.h} $(DISTDIR)
	rm $(DISTDIR)/*.o
	tar czf $(DISTDIR).tar.gz $(DISTDIR)
	zip -r $(DISTDIR).zip $(DISTDIR)

update-web:	dist docs
	cp html/* $(WEB_DIR)
	cp $(DISTDIR).tar.gz $(DISTDIR).zip /u/ftp/pub/outgoing
