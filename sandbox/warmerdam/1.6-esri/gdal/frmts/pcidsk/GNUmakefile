
include ../../GDALmake.opt

ifeq ($(JPEG_SETTING),internal)
JPEG_INCLUDE	=	-I../jpeg/libjpeg
endif

OBJ	=	pcidskdataset2.o vsi_pcidsk_io.o gdal_edb.o $(SDKOBJ)
CPPFLAGS	:=	-DHAVE_LIBJPEG $(JPEG_INCLUDE) -DPCIDSK_INTERNAL \
	-Isdk $(GDAL_INCLUDE) $(CPPFLAGS)

SDKOBJ 	=	\
	cbandinterleavedchannel.o \
	cpcidskchannel.o \
	cpixelinterleavedchannel.o \
	ctiledchannel.o \
	cexternalchannel.o \
	cpcidskfile.o \
	libjpeg_io.o \
	edb_pcidsk.o \
	metadataset_p.o \
	pcidskbuffer.o \
	pcidskcreate.o \
	pcidskexception.o \
	pcidskinterfaces.o \
	pcidskopen.o \
	pcidsk_pubutils.o \
	pcidsk_utils.o \
	sysvirtualfile.o \
	cpcidskgeoref.o \
	cpcidsksegment.o \
	cpcidskvectorsegment.o \
	cpcidskvectorsegment_consistencycheck.o \
	vecsegheader.o \
	vecsegdataindex.o \
	metadatasegment_p.o \
	sysblockmap.o \
	cpcidskpct.o \
	cpcidskrpcmodel.o \
	cpcidskgcp2segment.o \
	cpcidskbitmap.o \
	cpcidsk_tex.o \
	cpcidskapmodel.o \
	clinksegment.o \
	cpcidsktoutinmodel.o \
	cpcidskbinarysegment.o \
	cpcidsk_array.o \
	cpcidskephemerissegment.o \
	cpcidskads40model.o 


default:	$(OBJ:.o=.$(OBJ_EXT))

clean:
	rm -f *.o $(O_OBJ)

../o/%.$(OBJ_EXT):	sdk/%.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

../o/%.$(OBJ_EXT):	sdk/channel/%.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

../o/%.$(OBJ_EXT):	sdk/core/%.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

../o/%.$(OBJ_EXT):	sdk/port/%.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

../o/%.$(OBJ_EXT):	sdk/segment/%.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

%.$(OBJ_EXT):	sdk/%.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

%.$(OBJ_EXT):	sdk/channel/%.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

%.$(OBJ_EXT):	sdk/core/%.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

%.$(OBJ_EXT):	sdk/port/%.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

%.$(OBJ_EXT):	sdk/segment/%.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

install-obj:	$(O_OBJ:.o=.$(OBJ_EXT))

import:
	cp ~/pcidsk/src/*.h sdk
	cp ~/pcidsk/src/channel/*.cpp sdk/channel
	cp ~/pcidsk/src/channel/*.h sdk/channel
	cp ~/pcidsk/src/segment/*.cpp sdk/segment
	cp ~/pcidsk/src/segment/*.h sdk/segment
	cp ~/pcidsk/src/core/*.cpp sdk/core
	cp ~/pcidsk/src/core/*.h sdk/core
	cp ~/pcidsk/src/port/*.cpp sdk/port
