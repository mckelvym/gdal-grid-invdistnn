
GDAL_ROOT	=	../../..

include $(GDAL_ROOT)/GDALmake.opt

OBJ	=	tigerfilebase.o tigercompletechain.o tigeraltname.o \
		tigerfeatureids.o tigerzipcodes.o tigerlandmarks.o \
		tigerarealandmarks.o tigerkeyfeatures.o tigerpolychainlink.o \
		tigerpolygon.o tigerpip.o tigerentitynames.o tigeridhistory.o \
		tigertlidrange.o tigerzipplus4.o \
		ogrtigerdriver.o ogrtigerdatasource.o ogrtigerlayer.o 

ALL_C_FLAGS	=	-I.. -I../.. -I$(GDAL_ROOT)/port $(CFLAGS)

LIBS	:=	../ogrsf_frmts.a ../../ogr.a $(CPL_LIB) $(LIBS) 


default:	$(OBJ)

clean:
	rm -f *.o

%.o:	%.cpp
	$(CXX) -c $(ALL_C_FLAGS) $*.cpp -o $*.o
	ar r ../ogrsf_frmts.a $*.o

%.o:	%.c
	$(CC) -c $(ALL_C_FLAGS) $*.c -o $*.o
	ar r ../ogrsf_frmts.a $*.o

tigerinfo:	tigerinfo.o $(OBJ)
	$(CXX) $(LNK_FLAGS) tigerinfo.o $(LIBS) -o tigerinfo



