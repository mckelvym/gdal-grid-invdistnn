
OBJ	=	ogrtigerdriver.obj ogrtigerdatasource.obj ogrtigerlayer.obj \
		tigercompletechain.obj tigerfilebase.obj \
		tigeraltname.obj tigerfeatureids.obj tigerzipcodes.obj \
		tigerlandmarks.obj tigerarealandmarks.obj tigerkeyfeatures.obj\
		tigerpolychainlink.obj tigerpolygon.obj tigerpip.obj \
		tigerentitynames.obj tigeridhistory.obj tigertlidrange.obj \
		tigerzipplus4.obj

EXTRAFLAGS =	-I.. -I..\..

GDAL_ROOT	=	..\..\..

!INCLUDE $(GDAL_ROOT)\nmake.opt

default:	$(OBJ)

clean:
	-del *.obj *.pdb



