
include ../../GDALmake.opt

ifneq (($KAKDIR),)
ifneq ($(HAVE_LIBTOOL),yes)
include $(GDAL_ROOT)/frmts/jp2kak/jp2kak.lst
endif
endif

OBJ	=	jp2kakdataset.o

KAKINC = -I$(KAKDIR)/coresys/common -I$(KAKDIR)/apps/compressed_io \
	-I$(KAKDIR)/apps/jp2 -I$(KAKDIR)/apps/image -I$(KAKDIR)/apps/args 

APPOBJ  = $(KAK_OBJ)

# The following are just for Kakadu 5.1 or later.
APPOBJ +=	$(KAKDIR)/apps/make/kdu_tiff.o $(KAKDIR)/apps/make/jpx.o

INSTOBJ = $(foreach d,$(APPOBJ),../o/$(notdir $(d)))

#CXXFLAGS :=	$(CXXFLAGS) -DFILEIO_DEBUG

CPPFLAGS	:=	$(GDAL_INCLUDE) $(KAKINC) -I. $(CPPFLAGS)

default:	$(OBJ)

clean:
	rm -f *.o $(O_OBJ)
	rm -f $(INSTOBJ)

install-obj:	$(O_OBJ)
	cp $(APPOBJ) ../o

$(OBJ) $(O_OBJ):	jp2kak_roi.h subfile_source.h dbg_file_source.h
