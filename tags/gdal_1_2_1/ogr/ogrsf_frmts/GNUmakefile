GDAL_ROOT	=	../..

include ../../GDALmake.opt

SUBDIRS-yes	:= generic shape ntf sdts tiger s57 mitab dgn gml avc rec mem \
		   vrt
SUBDIRS-$(HAVE_OGDI)	+= ogdi
SUBDIRS-$(HAVE_OCI)	+= oci
SUBDIRS-$(HAVE_OGR_PG)	+= pg
SUBDIRS-$(HAVE_FME)	+= fme
SUBDIRS-$(HAVE_DODS)	+= dods
SUBDIRS-$(ODBC_SETTING)	+= odbc

default:	$(foreach d,$(SUBDIRS-yes),$(d)-target)

clean:	$(foreach d,$(SUBDIRS-yes),$(d)-clean)
	rm -f o/*.o
	$(RM) o/*.lo

