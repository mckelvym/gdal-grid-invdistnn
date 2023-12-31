
GDAL_ROOT	=	../../..

include $(GDAL_ROOT)/GDALmake.opt

MIN_OBJ	=	dgnopen.o dgnread.o dgnfloat.o dgnhelp.o dgnwrite.o dgnstroke.o
OBJ =	ogrdgndriver.o ogrdgndatasource.o ogrdgnlayer.o $(MIN_OBJ)

CPPFLAGS	:=	-I.. -I../.. $(GDAL_INCLUDE) $(CPPFLAGS)

CPL_DIR	=	$(GDAL_ROOT)/port

WEB_DIR =	$(HOME)/dgnlib-web
DIST_REV =	1.9
DIST_DIR =	dgnlib-$(DIST_REV)
DIST_FILE = 	dgnlib-$(DIST_REV).zip
DIST_FILES =	dgnread.cpp dgnopen.cpp dgnlib.h dgnlibp.h dgnfloat.cpp \
		dgnstroke.cpp dgnwrite.cpp dgnhelp.cpp README \
		dgnwritetest.c dgndump.c \
		\
		$(CPL_DIR)/cpl_config.h $(CPL_DIR)/cpl_port.h \
		$(CPL_DIR)/cpl_vsi.h $(CPL_DIR)/cpl_vsisimple.cpp \
		$(CPL_DIR)/cpl_conv.h $(CPL_DIR)/cpl_conv.cpp \
		$(CPL_DIR)/cpl_error.h $(CPL_DIR)/cpl_error.cpp \
		$(CPL_DIR)/cpl_string.h $(CPL_DIR)/cpl_string.cpp \
		$(CPL_DIR)/cpl_config.h.vc

default:	$(O_OBJ)

clean:
	rm -rf *.o $(DEST_DIR) dgnlib-$(DIST_REV).zip html man $(O_OBJ)
	rm -f dgndump dgnwritetest

dgndump:	dgndump.c $(MIN_OBJ)
	$(CXX) $(CPPFLAGS) $(CFLAGS) dgndump.c $(MIN_OBJ) $(GDAL_LIB) -ldl -lm -o dgndump 

dgnwritetest:	dgnwritetest.c $(MIN_OBJ)
	$(CXX) $(CPPFLAGS) $(CFLAGS) dgnwritetest.c $(MIN_OBJ) $(GDAL_LIB) -ldl \
	-lm -o dgnwritetest

docs:
	doxygen

dist:
	rm -rf $(DIST_DIR)
	mkdir $(DIST_DIR)
	cp $(DIST_FILES) $(DIST_DIR)
	cp Makefile.dist $(DIST_DIR)/Makefile
	cp Makefile.vc.dist $(DIST_DIR)/Makefile.vc
	zip -r $(DIST_FILE) $(DIST_DIR)

web-docs-update:	docs
	cp html/* $(WEB_DIR)/libhtml

web-update:	web-docs-update dist
	cp $(DIST_FILE) $(WEB_DIR)/dl

pge_test:	pge_test.cpp dgn_pge.cpp $(OBJ)
	gcc -Wall -g pge_test.cpp dgn_pge.cpp $(OBJ) $(GDAL_LIB) -lm -o pge_test
