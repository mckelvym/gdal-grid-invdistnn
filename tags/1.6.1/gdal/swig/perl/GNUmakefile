BINDING = perl
SWIGDEFINES = -DPERL_CPAN_NAMESPACE

include ../SWIGmake.base

all:

clean:
	-rm -f gdal.bs gdal_wrap.o
	-rm -f gdalconst.bs gdalconst_wrap.o
	-rm -f ogr.bs ogr_wrap.o
	-rm -f osr.bs osr_wrap.o
	-rm -rf blib
	-rm -f pm_to_blib
	-rm -f Makefile_Geo__GDAL Makefile_Geo__GDAL__Const Makefile_Geo__OGR Makefile_Geo__OSR

veryclean: clean
	-rm -f ${WRAPPERS} 
	-rm -f gdal.pm gdalconst.pm osr.pm ogr.pm

generate: ${WRAPPERS}
	if [ ! -d lib ]; then mkdir lib; fi
	if [ ! -d "lib/Geo" ]; then mkdir "lib/Geo"; fi
	if [ ! -d "lib/Geo/GDAL" ]; then mkdir "lib/Geo/GDAL"; fi
	if [ -f GDAL.pm ]; then mv GDAL.pm lib/Geo; fi
	if [ -f Const.pm ]; then mv Const.pm lib/Geo/GDAL; fi
	if [ -f OGR.pm ]; then mv OGR.pm lib/Geo; fi
	if [ -f OSR.pm ]; then mv OSR.pm lib/Geo; fi

build: Makefile_Geo__GDAL
	$(MAKE) -f Makefile_Geo__GDAL
	$(MAKE) -f Makefile_Geo__GDAL__Const
	$(MAKE) -f Makefile_Geo__OGR
	$(MAKE) -f Makefile_Geo__OSR

install: build
	$(MAKE) -f Makefile_Geo__GDAL install
	$(MAKE) -f Makefile_Geo__GDAL__Const install
	$(MAKE) -f Makefile_Geo__OGR install
	$(MAKE) -f Makefile_Geo__OSR install

test: build
	$(MAKE) -f Makefile_Geo__GDAL test

dist: Makefile_Geo__GDAL
	$(MAKE) -f Makefile_Geo__GDAL dist

Makefile_Geo__GDAL:
	perl Makefile.PL
