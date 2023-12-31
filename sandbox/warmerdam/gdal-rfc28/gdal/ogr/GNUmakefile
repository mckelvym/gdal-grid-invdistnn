
include ../GDALmake.opt

include ./file.lst

INST_H_FILES	=	ogr_core.h ogr_feature.h ogr_geometry.h ogr_p.h \
		ogr_spatialref.h ogr_srs_api.h ogrsf_frmts/ogrsf_frmts.h \
		ogr_featurestyle.h ogr_api.h

ifeq ($(HAVE_GEOS),yes)
CPPFLAGS 	:=	-DHAVE_GEOS=1 $(GEOS_CFLAGS) $(CPPFLAGS)
endif

ifeq ($(OGR_ENABLED),yes)
CXXFLAGS 	:=	-DHAVE_MITAB $(CXXFLAGS)
endif

ifeq ($(HAVE_EXPAT),yes)
CPPFLAGS 	:=	-DHAVE_EXPAT $(EXPAT_INCLUDE) $(CPPFLAGS)
endif

CPPFLAGS	:=	-Iogrsf_frmts -I. $(GDAL_INCLUDE) $(PROJ_INCLUDE) $(PROJ_FLAGS) $(CPPFLAGS)

default:	lib

all:	lib

clean:
	rm -f html/*
	(cd ogrsf_frmts; $(MAKE) clean)
	$(RM) *.o

very-clean: clean
	rm -rf html rtf

lib:	sublibs $(OBJ:.o=.$(OBJ_EXT))

$(OBJ): ogr_feature.h ogr_geometry.h

obj:	$(OBJ)

ifeq ($(OGR_ENABLED),yes)

sublibs:
	(cd ogrsf_frmts; $(MAKE))

else

sublibs:

endif

docs:
	rm -rf html/*
	doxygen
	cp ogrsf_frmts/*/drv_*.html html
	cp ogrsf_frmts/ogr_formats.html html
	cp ogr_feature_style.html html

install-docs:
	$(INSTALL_DIR) $(DESTDIR)$(INST_DOCS)/ogr
	cp html/* $(DESTDIR)$(INST_DOCS)/ogr
	cp ../doc/ogr/*.gif $(DESTDIR)$(INST_DOCS)/ogr

gdalso:	$(GDAL_SLIB)

$(GDAL_SLIB):
	(cd ..; $(MAKE) check-lib)

web-update:	docs
	$(INSTALL_DIR) $(INST_HTML)/ogr
	cp html/* $(INST_HTML)/ogr
	cp ogrsf_frmts/*/drv_*.html $(INST_HTML)/ogr
	cp ogrsf_frmts/ogr_formats.html $(INST_HTML)/ogr
	cp ../doc/ogr/*.gif $(INST_HTML)/ogr

install:
	for f in $(INST_H_FILES) ; \
	    do $(INSTALL_DATA) $$f $(DESTDIR)$(INST_INCLUDE) ; \
	done

parser:
	bison -p swq -d -oswq_parser.cpp swq_parser.y

