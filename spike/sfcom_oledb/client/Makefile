
GDAL_ROOT =	C:\warmerda\gdal

LINKFLAGS =	/Zi /MTd
EXTRAFLAGS =	-I$(MSDASDK)\inc \
		-I"$(VCDIR)\ATL\Include" -DWIN32 \
		-I$(GDAL_ROOT)\port -I$(GDAL_ROOT)\ogr 

!INCLUDE $(GDAL_ROOT)\nmake.opt

OLE_LIBS =	oledb.lib ole32.lib oleaut32.lib 
ALL_LIBS =	$(OLE_LIBS) $(GDAL_ROOT)/gdal_i.lib

OBJ_OLEDB =	oledb_sup.obj \
		sfcenumerator.obj sfcdatasource.obj sfctable.obj

default:        sfcdump.exe

oledb:	ogroledb.lib 
	cd sfcom_oledb
	$(MAKE)
	cd ..

geometryidl.h: geometryidl.idl spatialreferenceidl.h
	midl geometryidl.idl

spatialreferenceidl.h: spatialreferenceidl.idl
	midl spatialreferenceidl.idl

ogroledb.lib:	$(OBJ_OLEDB)
	lib /nologo /out:ogroledb.lib $(OBJ_OLEDB)

sfcdump.exe:	geometryidl.h sfcdump.obj ogroledb.lib
	cl $(LINKFLAGS) sfcdump.obj ogroledb.lib $(ALL_LIBS) $(LINKARGS) \
		/link $(LINKER_FLAGS)
    if exist $@.manifest mt -manifest $@.manifest -outputresource:$@;1

clean:
	-del *.obj
	-del *.exe
	-del *.exp
	-del *.exe.manifest
	-del *.lib
	-del *.pdb
	-del *.ilk
	-del *_i.c
	-del *_p.c
	-del geometryidl.h
	-del spatialreferenceidl.h
	-del dlldata.c

