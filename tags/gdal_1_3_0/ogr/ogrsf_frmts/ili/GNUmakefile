
GDAL_ROOT	=	../../..

SUBDIRS-yes     := iom

include $(GDAL_ROOT)/GDALmake.opt

CORE_OBJ =	 ili1reader.o ili2reader.o ili2handler.o

OGR_OBJ =	iomhelper.o ogrili1driver.o ogrili1datasource.o ogrili1layer.o ogrili2driver.o ogrili2datasource.o ogrili2layer.o

OBJ =	$(CORE_OBJ) $(OGR_OBJ)

ifeq ($(HAVE_GEOS),yes)
CPPFLAGS 	:=	-DHAVE_GEOS=1 $(GEOS_INC) $(CPPFLAGS)
endif

CPPFLAGS :=	-I.. -I../.. $(GDAL_INCLUDE) $(XERCES_INCLUDE) $(CPPFLAGS)


default:	$(foreach d,$(SUBDIRS-yes),$(d)-target)	$(O_OBJ)

clean: $(foreach d,$(SUBDIRS-yes),$(d)-clean)
	rm -f *.o $(O_OBJ)
