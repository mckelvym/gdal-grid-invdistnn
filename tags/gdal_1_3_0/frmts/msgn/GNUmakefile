GDAL_ROOT      =       ../..

include $(GDAL_ROOT)/GDALmake.opt

OBJ    =       msgndataset.o msg_basic_types.o msg_reader_core.o

CPPFLAGS :=	$(GDAL_INCLUDE) $(CPPFLAGS) -I. -DGDAL_SUPPORT

default:       $(OBJ)

clean:
	rm -f *.o $(O_OBJ)
       
install-obj:   $(O_OBJ)
