GDAL_ROOT	=	..\..

DIRLIST		=	generic shape ntf sdts tiger s57 dgn mitab gml \
			$(OGDIDIR)

!INCLUDE $(GDAL_ROOT)\nmake.opt

!IFDEF OGDIDIR
OGDIDIR	=	ogdi
OGDIOBJ =	ogdi\*.obj
!ENDIF

default:
	for %d in ( $(DIRLIST) ) do ..\..\submake.bat %d

	lib /out:ogrsf_frmts.lib generic\*.obj shape\*.obj ntf\*.obj \
				 sdts\*.obj s57\*.obj tiger\*.obj gml\*.obj \
				 mitab\*.obj dgn\*.obj $(OGDIOBJ)
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
	for %d in ( $(DIRLIST) ) do ..\..\submake.bat %d clean
