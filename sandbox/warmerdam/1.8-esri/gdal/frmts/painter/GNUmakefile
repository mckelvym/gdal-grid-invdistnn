
include ../../GDALmake.opt

OBJ		=	painter.o pixelgenerator.o

CPPFLAGS	:=	$(GDAL_INCLUDE) $(BOOST_INCLUDE) $(CPPFLAGS)

default:	$(OBJ:.o=.$(OBJ_EXT))

clean:
	rm -f *.o $(O_OBJ)

install-obj:	$(O_OBJ:.o=.$(OBJ_EXT))
