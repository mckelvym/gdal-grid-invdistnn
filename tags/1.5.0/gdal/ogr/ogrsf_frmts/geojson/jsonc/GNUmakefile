# $Id$
#
# Makefile building jcon-c library (http://oss.metaparadigm.com/json-c/)
# 
include ../../../../GDALmake.opt

OBJ = \
	arraylist.o \
	debug.o \
	json_object.o \
	json_tokener.o \
	json_util.o \
	linkhash.o \
	printbuf.o

O_OBJ = $(foreach file,$(OBJ),../../o/$(file))

CPPFLAGS := $(CPPFLAGS)

default:	$(O_OBJ)

../../o/%.o:	%.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

clean:
	rm -f *.o $(O_OBJ)
	rm -f *~
