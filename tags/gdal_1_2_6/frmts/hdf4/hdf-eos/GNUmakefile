GDAL_ROOT	=	../../..

include $(GDAL_ROOT)/GDALmake.opt

OBJ	= \
	EHapi.o \
	GDapi.o \
	PTapi.o \
	SWapi.o \
	gctp_wrap.o

O_OBJ	=	$(foreach file,$(OBJ),../../o/$(file))

CPPFLAGS	:=	$(GDAL_INCLUDE) $(HDF4_INCLUDE) -I. $(CPPFLAGS) 

default:	$(OBJ)

clean:
	$(RM) *.o *.a

install-obj:    $(SUBLIBS) $(O_OBJ)

../../o/%.o:	%.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<
