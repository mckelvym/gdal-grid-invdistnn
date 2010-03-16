
include ../../GDALmake.opt

OBJ	=	jpipkakdataset.o

KAKINC = -I$(KAKDIR)/coresys/common -I$(KAKDIR)/apps/compressed_io \
	-I$(KAKDIR)/apps/jp2 -I$(KAKDIR)/apps/image -I$(KAKDIR)/apps/args \
	-I$(KAKDIR)/apps/support -I$(KAKDIR)/apps/kdu_compress

APPOBJ  = $(KAK_OBJ)

INSTOBJ = $(foreach d,$(APPOBJ),../o/$(notdir $(d)))

#CXXFLAGS :=	$(CXXFLAGS) -DFILEIO_DEBUG

CPPFLAGS	:=	$(GDAL_INCLUDE) $(KAKINC) -I. $(CPPFLAGS)

default:	$(OBJ:.o=.$(OBJ_EXT))

clean:
	rm -f *.o $(O_OBJ)
	rm -f $(INSTOBJ)

install-obj:	$(O_OBJ:.o=.$(OBJ_EXT))


