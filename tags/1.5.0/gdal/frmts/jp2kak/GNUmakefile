
include ../../GDALmake.opt

ifneq (($KAKDIR),)
ifneq ($(HAVE_LIBTOOL),yes)
include $(GDAL_ROOT)/frmts/jp2kak/jp2kak.lst
endif
endif

OBJ	=	jp2kakdataset.o

KAKINC = -I$(KAKDIR)/coresys/common -I$(KAKDIR)/apps/compressed_io \
	-I$(KAKDIR)/apps/jp2 -I$(KAKDIR)/apps/image -I$(KAKDIR)/apps/args \
	-I$(KAKDIR)/apps/support -I$(KAKDIR)/apps/kdu_compress

APPOBJ  = $(KAK_OBJ)

INSTOBJ = $(foreach d,$(APPOBJ),../o/$(notdir $(d)))

#CXXFLAGS :=	$(CXXFLAGS) -DFILEIO_DEBUG

CPPFLAGS	:=	$(GDAL_INCLUDE) $(KAKINC) -I. $(CPPFLAGS)

default:	$(OBJ)

clean:
	rm -f *.o $(O_OBJ)
	rm -f $(INSTOBJ)

install-obj:	$(O_OBJ)
	cp $(APPOBJ) ../o

$(OBJ) $(O_OBJ):	subfile_source.h
