

include ../../../GDALmake.opt

OBJ     =  ogrtigerdriver.o           \
	   ogrtigerdatasource.o       \
	   ogrtigerlayer.o            \
	   tigerfilebase.o            \
	   tigercompletechain.o       \
	   tigeraltname.o             \
	   tigerfeatureids.o          \
	   tigerzipcodes.o            \
	   tigerlandmarks.o           \
	   tigerarealandmarks.o       \
	   tigerkeyfeatures.o         \
	   tigerpolygon.o             \
	   tigerpolygoncorrections.o  \
	   tigerentitynames.o         \
	   tigerpolygoneconomic.o     \
	   tigeridhistory.o           \
	   tigerpolychainlink.o       \
	   tigerpip.o                 \
	   tigerspatialmetadata.o     \
	   tigertlidrange.o           \
	   tigerzerocellid.o          \
	   tigeroverunder.o           \
	   tigerzipplus4.o	      \
	   tigerpoint.o



CPPFLAGS	:=	-I.. -I../.. $(GDAL_INCLUDE) $(CPPFLAGS)

LIBS	:=	$(GDAL_LIB) $(LIBS)


default:	$(O_OBJ:.o=.$(OBJ_EXT))

clean:
	rm -f *.o $(O_OBJ)

tigerinfo$(EXE):	tigerinfo.$(OBJ_EXT)
	$(LD) $(LDFLAGS) tigerinfo.$(OBJ_EXT) $(CONFIG_LIBS) -o tigerinfo$(EXE)
