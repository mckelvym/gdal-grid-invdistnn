

include ../../GDALmake.opt

OBJ	=	adrgdataset.o

CPPFLAGS	:=	$(GDAL_INCLUDE) $(CPPFLAGS) -I../iso8211

default:	$(OBJ:.o=.$(OBJ_EXT))

clean:
	rm -f *.o

install-obj:	$(O_OBJ:.o=.$(OBJ_EXT))
