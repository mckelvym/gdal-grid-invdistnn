
include ../../GDALmake.opt

OBJ	=	jpeg2000dataset.o

CPPFLAGS	:=	$(GDAL_INCLUDE) $(JASPER_FLAGS) $(CPPFLAGS)

default:	$(OBJ)

clean:
	rm -f $(OBJ) $(O_OBJ)

install-obj:	$(O_OBJ)
