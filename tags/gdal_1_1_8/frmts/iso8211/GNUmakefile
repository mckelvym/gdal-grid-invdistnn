GDAL_ROOT	=	../..

include $(GDAL_ROOT)/GDALmake.opt

VERSION =	1.1
DISTDIR =	iso8211lib-$(VERSION)
WEB_DIR =	/u/www/projects/iso8211

INCLUDE	=	-I. -I$(GDAL_ROOT)/port

ISOLIB = libiso8211.a
OBJ =	ddfmodule.o ddfutils.o ddffielddefn.o ddfrecord.o ddffield.o \
	ddfsubfielddefn.o

default:	$(ISOLIB) 8211view

all:	$(ISOLIB) 8211dump 8211view docs

clean:
	rm -rf *.o 8211dump 8211view $(DISTDIR) $(DISTDIR).tar.gz html/* \
		$(ISOLIB)

dist-clean:	clean
	rm -rf $(DISTDIR)

%.o:	%.cpp
	$(CXX) -c $(INCLUDE) $(CXXFLAGS) $*.cpp -o $*.o

%.o:	%.c
	$(CC) -c $(INCLUDE) $(CFLAGS) $*.c -o $*.o

$(ISOLIB):	$(OBJ)
	$(AR) r $(ISOLIB) $?
	$(RANLIB) $(ISOLIB)

8211dump:	8211dump.o $(ISOLIB)
	$(CXX) $(CXX_ODFLAGS) 8211dump.o \
		$(ISOLIB) $(CPL_LIB) $(LIBS) -o 8211dump

8211view:	8211view.o $(ISOLIB)
	$(CXX) $(CXX_ODFLAGS) 8211view.o \
		$(ISOLIB) $(CPL_LIB) $(LIBS) -o 8211view

timetest:	timetest.o $(ISOLIB)
	$(CXX) $(CXX_ODFLAGS) timetest.o \
		$(ISOLIB) $(CPL_LIB) $(LIBS) -o timetest

upd_test:	upd_test.o $(ISOLIB)
	$(CXX) $(CXX_ODFLAGS) upd_test.o \
		$(ISOLIB) $(CPL_LIB) $(LIBS) -o upd_test

docs:
	rm -rf html
	mkdir html
	doxygen
	rm html/index.html
	cp html/ISO8211Lib.html html/index.html

dist:	docs
	rm -rf $(DISTDIR)
	mkdir $(DISTDIR)
	mkdir $(DISTDIR)/html
	cp html/* $(DISTDIR)/html
	mkdir $(DISTDIR)/testdata
	cp testdata/*.* $(DISTDIR)/testdata
	cp teststream.sh teststream.out $(DISTDIR)
	autoconf
	cp *.cpp *.h configure Makefile.in $(DISTDIR)
	rm configure
	cp $(GDAL_ROOT)/port/{cpl_error*,cpl_port*,cpl_string*} $(DISTDIR)
	cp $(GDAL_ROOT)/port/{cpl_vsisimple.cpp,cpl_config.h.in} $(DISTDIR)
	cp $(GDAL_ROOT)/port/{cpl_vsi.h,cpl_conv.*} $(DISTDIR)
	rm $(DISTDIR)/*.o
	tar czf $(DISTDIR).tar.gz $(DISTDIR)
	zip -r $(DISTDIR).zip $(DISTDIR)

update-web:	dist docs
	cp html/* $(WEB_DIR)
	cp $(DISTDIR).tar.gz $(WEB_DIR)
	cp $(DISTDIR).tar.gz /u/ftp/pub/outgoing


test:	8211dump
	@./teststream.sh > t1.out
	@if test "`diff t1.out teststream.out`" = '' ; then \
	    echo "******* Stream 1 Succeeded *********"; \
	    rm t1.out; \
	else \
	    echo "******* Stream 1 Failed *********"; \
	    diff t1.out teststream.out; \
	fi


install-obj:	$(ISOLIB) $(foreach file,$(OBJ),../o/$(file))

../o/%.o:	%.o
	cp $*.o ../o


