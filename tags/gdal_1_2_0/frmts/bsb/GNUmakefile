
GDAL_ROOT	=	../..

VERSION =	1.2

include $(GDAL_ROOT)/GDALmake.opt

OBJ	=	bsb_read.o bsbdataset.o

DISTDIR =	bsb-$(VERSION)

CPPFLAGS	:=	$(GDAL_INCLUDE) $(CPPFLAGS)

default:	$(OBJ)

clean:
	rm -f *.o

bsb2raw:	bsb2raw.o bsb_read.o
	$(CXX) bsb2raw.o bsb_read.o $(GDAL_LIB) $(LIBS) -o bsb2raw


install-obj:	$(O_OBJ)

dist:	
	rm -rf $(DISTDIR)
	mkdir $(DISTDIR)
	cp *.cpp *.c *.h $(DISTDIR)
	rm $(DISTDIR)/bsbdataset.cpp
	cp Makefile.dist $(DISTDIR)/Makefile
	cp README.dist $(DISTDIR)/README
	cp $(GDAL_ROOT)/port/{cpl_error*,cpl_port*,cpl_string*} $(DISTDIR)
	cp $(GDAL_ROOT)/port/{cpl_vsisimple.cpp,cpl_config.h.in} $(DISTDIR)
	cp $(GDAL_ROOT)/port/{cpl_vsi.h,cpl_conv.*,cpl_path.cpp} $(DISTDIR)
	cp $(GDAL_ROOT)/port/cpl_config.h.in $(DISTDIR)/cpl_config.h
	rm $(DISTDIR)/*.o
	tar czf $(DISTDIR).tar.gz $(DISTDIR)
	zip -r $(DISTDIR).zip $(DISTDIR)
