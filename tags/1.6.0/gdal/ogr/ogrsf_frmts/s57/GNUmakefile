

ISODIR	=	../../../frmts/iso8211
ISOLIB  =	$(ISODIR)/libiso8211.a

include ../../../GDALmake.opt

OBJ	=	ogrs57driver.o ogrs57datasource.o ogrs57layer.o \
		s57reader.o s57writer.o ddfrecordindex.o \
		s57classregistrar.o s57filecollector.o \
	        s57featuredefns.o

CPPFLAGS	:=	-I.. -I../.. -I$(ISODIR) $(GDAL_INCLUDE) $(CPPFLAGS)

default:	$(O_OBJ)

clean:
	rm -f *.o s57dump$(EXE) mks57$(EXE) $(O_OBJ)

all:	default s57dump$(EXE) mks57$(EXE)

s57dump$(EXE):	s57dump.o
	$(LD) $(LDFLAGS) s57dump.o $(CONFIG_LIBS) -o s57dump$(EXE)

mks57$(EXE):	mks57.o
	$(LD) $(LDFLAGS) mks57.o $(CONFIG_LIBS) -o mks57$(EXE)

write_soundg$(EXE):	write_soundg.o
	$(LD) $(LDFLAGS) write_soundg.o $(CONFIG_LIBS) -o write_soundg$(EXE)
