
GDAL_ROOT = ../..

include $(GDAL_ROOT)/GDALmake.opt

all: build

BINDING = ruby
include ../SWIGmake.base

# Add -prefix "Gdal::" to SWIGARGS
SWIGARGS += -prefix "gdal::"

clean:
	$(MAKE) -f RubyMakefile.mk clean

veryclean: clean
	rm -f $(WRAPPERS)
	$(MAKE) -f RubyMakefile.mk veryclean

generate: ${WRAPPERS}

build: generate
	$(MAKE) -f RubyMakefile.mk build

install: 
	$(MAKE) -f RubyMakefile.mk install
