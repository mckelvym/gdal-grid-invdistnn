CXX = g++
CXXFLAGS =`gdal-config --cflags` -Wall -I. -Itut $(CPPFLAGS)
LDFLAGS = `gdal-config --libs`

PROGS = gdal_unit_test testperfcopywords testcopywords

all: $(PROGS)

OBJ = \
    gdal_unit_test.o \
    test_cpl.o \
    test_gdal_aaigrid.o \
    test_gdal_dted.o \
    test_gdal_gtiff.o \
    test_ogr_geos.o \
    test_ogr_shape.o \
    test_osr.o \
    test_osr_ct.o \
    test_osr_pci.o \
    test_osr_proj4.o \
    tut/tut_gdal.o

gdal_unit_test: $(OBJ)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@

testperfcopywords: testperfcopywords.cpp
	$(CXX) $(CXXFLAGS) $< $(LDFLAGS) -o $@
	
testcopywords: testcopywords.cpp
	$(CXX) -O2 $(CXXFLAGS) $< $(LDFLAGS) -o $@

clean:
	$(RM) $(PROGS)
	$(RM) *.o
	$(RM) *.a
	$(RM) *.out
	$(RM) *~

