

include ../../../GDALmake.opt

OBJ	=	ogridbdatasource.o ogridblayer.o ogridbdriver.o \
		ogridbtablelayer.o ogridbselectlayer.o

IDB_DEFS =	-DIT_DLLIB -DIT_DO_NOT_SIMULATE_BOOL

CPPFLAGS :=	-I.. -I../.. $(GDAL_INCLUDE) $(IDB_INC) $(IDB_DEFS) $(CPPFLAGS)

default:	$(O_OBJ)

clean:
	rm -f *.o $(O_OBJ)
