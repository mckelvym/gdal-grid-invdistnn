

include ../../GDALmake.opt

all: build

BINDING = lua
include ../SWIGmake.base
EXTRA_INC=-I../../port -I../../gcore -I../../alg -I../../ogr

LUA_MODULES = gdal.so ogr.so

clean:
	-rm -rf ${LUA_MODULES}

veryclean: clean
	-rm -f ${WRAPPERS} ${LUA_MODULES}
	-rm -f *.o *.lo

generate: ${WRAPPERS}

build: ${LUA_MODULES}

install: build

$(LUA_MODULES): %.so: %_wrap.o
	$(LD) -shared $(LDFLAGS) $(CONFIG_LIBS) -llua $< -o $@

lua_%.so: %_wrap.o
	$(CC) $(LDFLAGS) $< -o $@ $(EXTRA_LIB)

%.o: %.cpp
	$(CXX) $(EXTRA_INC) $(CFLAGS) -c $<
%.o: %.c
	$(CC) $(EXTRA_INC) $(CFLAGS) -c $<
