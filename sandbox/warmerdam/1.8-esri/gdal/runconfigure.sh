#!/bin/bash
./configure --with-hide-internal-symbols \
 --without-ogr --with-jasper=no --with-mysql=no --with-xerces=no --with-expat=no --with-xml2=no \
 --with-pcre=no --with-spatialite=no --with-openjpeg=no --with-oci=no --with-geos=no --with-webp=no --with-odbc=no \
 --with-poppler=no --with-podofo=no --with-static-proj4=no --with-cfitsio=no --with-grass=no --with-libgrass=no \
 --with-pg=no --with-freexl=no --with-sqlite \
 --with-libz=internal --with-jpeg=internal --with-libjson-c=internal --with-png=internal --with-libtiff=internal \
 --with-geotiff=internal --with-gif=internal --with-curl \
 --with-netcdf=/home/robin/install --with-kakadu=/home/robin/projects/kakadu-7.3.2 \
 --with-mrsid=/home/robin/install --with-mrsid_lidar=/home/robin/install \
 --with-hdf4=/home/robin/install \
 CFLAGS='-g -O0' CXXFLAGS='-g -O0'

#--with-ecw=/home/robin/projects/ecwjp2_sdk
