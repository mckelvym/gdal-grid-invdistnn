GDAL_ROOT	=	../..

include $(GDAL_ROOT)/GDALmake.opt

OBJ	=	usgsdemdataset.o

CPPFLAGS	:=	$(GDAL_INCLUDE) $(CPPFLAGS)

default:	$(OBJ)

clean:
	rm -f *.o

install-obj:	$(O_OBJ)
