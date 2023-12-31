GDAL_ROOT	=	..\..

DIRLIST		=	generic shape ntf sdts tiger s57 dgn mitab gml avc \
			rec mem vrt csv \
			$(OGDIDIR) $(FMEDIR) $(OCIDIR) $(PG_DIR) \
			$(ODBCDIR) $(SQLITE_DIR)

!INCLUDE $(GDAL_ROOT)\nmake.opt

!IFDEF OGDIDIR
OGDIDIR	=	ogdi
OGDIOBJ =	ogdi\*.obj
!ENDIF

!IFDEF ODBC_SUPPORTED
ODBCDIR	=	odbc
ODBCOBJ =	odbc\*.obj
!ENDIF

!IFDEF OCI_LIB
OCIDIR	=	oci
OCIOBJ =	oci\*.obj
!ENDIF

!IFDEF FME_DIR
FMEDIR = fme
FMEOBJ = fme\*.obj
!ENDIF

!IFDEF PG_INC_DIR
PG_DIR = pg
PG_OBJ = pg\*.obj
!ENDIF

!IFDEF SQLITE_LIB
SQLITE_DIR = sqlite
SQLITE_OBJ = sqlite\*.obj
!ENDIF

default:
	for %d in ( $(DIRLIST) ) do \
		cd %d \
		&& nmake /NOLOGO /f makefile.vc \
		&& cd ..

	lib /out:ogrsf_frmts.lib generic\*.obj shape\*.obj ntf\*.obj \
				 sdts\*.obj s57\*.obj tiger\*.obj gml\*.obj \
				 mitab\*.obj dgn\*.obj avc\*.obj mem\*.obj \
				 vrt\*.obj csv\*.obj \
				$(OGDIOBJ) $(ODBCOBJ) $(SQLITE_OBJ) \
				$(FMEOBJ) $(OCIOBJ) $(PG_OBJ) rec\*.obj
	lib /out:ogrsf_frmts_sup.lib \
				 ..\..\frmts\iso8211\*.obj \
				 ..\..\frmts\sdts\sdtsattrreader.obj \
				 ..\..\frmts\sdts\sdtscatd.obj \
				 ..\..\frmts\sdts\sdtsiref.obj \
				 ..\..\frmts\sdts\sdtslib.obj \
				 ..\..\frmts\sdts\sdtslinereader.obj \
				 ..\..\frmts\sdts\sdtspointreader.obj \
				 ..\..\frmts\sdts\sdtspolygonreader.obj \
				 ..\..\frmts\sdts\sdtsrasterreader.obj \
				 ..\..\frmts\sdts\sdtstransfer.obj \
				 ..\..\frmts\sdts\sdtsindexedreader.obj \
				 ..\..\frmts\sdts\sdtsxref.obj

clean:
	-del ogrsf_frmts.lib
	-del ogrsf_frmts_sup.lib
	for %d in ( $(DIRLIST) ) do \
		cd %d \
		&& nmake /NOLOGO /f makefile.vc clean \
		&& cd ..

html-install:
	copy *.html $(HTMLDIR)
	-for %d in ( $(DIRLIST) ) do \
		copy %d\drv_*.html $(HTMLDIR)
