
include ../../GDALmake.opt

OBJ	=	gsagdataset.o gsbgdataset.o gs7bgdataset.o

CPPFLAGS	:=	$(GDAL_INCLUDE) $(CPPFLAGS)

default:	$(OBJ)

clean:
	rm -f *.o

install-obj:	$(O_OBJ)
