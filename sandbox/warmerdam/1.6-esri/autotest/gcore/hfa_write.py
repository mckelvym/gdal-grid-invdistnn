#!/usr/bin/env python
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test read/write functionality for Erdas Imagine (.img) HFA driver.
# Author:   Frank Warmerdam <warmerdam@pobox.com>
# 
###############################################################################
# Copyright (c) 2003, Frank Warmerdam <warmerdam@pobox.com>
# 
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
###############################################################################

import os
import sys
import gdal
import shutil

sys.path.append( '../pymod' )

import gdaltest

###############################################################################
# test that we can write a small file with a custom layer name.

def hfa_write_desc():

    src_ds = gdal.Open( 'data/byte.tif' )

    new_ds = gdal.GetDriverByName('HFA').CreateCopy( 'tmp/test_desc.img',
                                                     src_ds )

    bnd = new_ds.GetRasterBand(1)
    bnd.SetDescription( 'CustomBandName' )

    src_ds = None
    new_ds = None

    new_ds = gdal.Open( 'tmp/test_desc.img' )
    bnd = new_ds.GetRasterBand(1)
    if bnd.GetDescription() != 'CustomBandName':
        gdaltest.post_reason( 'Didnt get custom band name.' )
        return 'fail'

    new_ds = None

    gdal.GetDriverByName('HFA').Delete( 'tmp/test_desc.img' )

    return 'success'

###############################################################################
# test writing 4 bit files.

def hfa_write_4bit():
    drv = gdal.GetDriverByName('HFA')
    src_ds = gdal.Open('data/byte.tif')
    ds = drv.CreateCopy('tmp/4bit.img', src_ds, options = ['NBITS=1'] )
    ds = None
    src_ds = None

    ds = gdal.Open('tmp/4bit.img')
    cs = ds.GetRasterBand(1).Checksum()

    if cs != 252:
        gdaltest.post_reason( 'Got wrong checksum on 4bit image.' )
        print cs
        return 'fail'

    ds = None

    drv.Delete( 'tmp/4bit.img' )

    return 'success'

###############################################################################
# test writing 4 bit files compressed.

def hfa_write_4bit_compressed():
    drv = gdal.GetDriverByName('HFA')
    src_ds = gdal.Open('data/byte.tif')
    ds = drv.CreateCopy('tmp/4bitc.img', src_ds,
                        options = ['NBITS=1', 'COMPRESSED=YES'] )
    ds = None
    src_ds = None

    ds = gdal.Open('tmp/4bitc.img')
    cs = ds.GetRasterBand(1).Checksum()

    if cs != 252:
        gdaltest.post_reason( 'Got wrong checksum on 4bit image.' )
        print cs
        return 'fail'

    ds = None

    drv.Delete( 'tmp/4bitc.img' )

    return 'success'

###############################################################################
# Test creating a file with a nodata value, and fetching otherwise unread
# blocks and verifying they are the nodata value.  (#2427)

def hfa_write_nd_invalid():

    drv = gdal.GetDriverByName('HFA')
    ds = drv.Create('tmp/ndinvalid.img', 512, 512, 1, gdal.GDT_Byte, [] )
    ds.GetRasterBand(1).SetNoDataValue( 200 )
    ds = None

    ds = gdal.Open('tmp/ndinvalid.img')
    cs = ds.GetRasterBand(1).Checksum()

    if cs != 29754:
        gdaltest.post_reason( 'Got wrong checksum on invalid image.' )
        print cs
        return 'fail'

    ds = None

    drv.Delete( 'tmp/ndinvalid.img' )

    return 'success'

###############################################################################
# Test updating .rrd overviews in place (#2524).

def hfa_update_overviews():


    shutil.copyfile( 'data/small_ov.img', 'tmp/small.img' )
    shutil.copyfile( 'data/small_ov.rrd', 'tmp/small.rrd' )

    ds = gdal.Open( 'tmp/small.img', gdal.GA_Update )
    result = ds.BuildOverviews( overviewlist = [2] )

    if result != 0:
        print result
        gdaltest.post_reason( 'BuildOverviews() failed.' )
        return 'fail'
    ds = None

    return 'success'

###############################################################################
# Test cleaning external overviews.

def hfa_clean_external_overviews():

    ds = gdal.Open( 'tmp/small.img', gdal.GA_Update )
    result = ds.BuildOverviews( overviewlist = [] )

    if result != 0:
        gdaltest.post_reason( 'BuildOverviews() failed.' )
        return 'fail'

    if ds.GetRasterBand(1).GetOverviewCount() != 0:
        gdaltest.post_reason( 'Overviews still exist.' )
        return 'fail'

    ds = None
    ds = gdal.Open( 'tmp/small.img' )
    if ds.GetRasterBand(1).GetOverviewCount() != 0:
        gdaltest.post_reason( 'Overviews still exist.' )
        return 'fail'
    ds = None

    try:
        fd = open('tmp/small.rrd')
    except:
        fd = None

    if fd is not None:
        gdaltest.post_reason( 'small.rrd still present.' )
        return 'fail'

    fd = None

    gdal.GetDriverByName('HFA').Delete( 'tmp/small.img' )

    return 'success'

###############################################################################
# Test writing high frequency data (#2525).

def hfa_bug_2525():
    drv = gdal.GetDriverByName('HFA')
    ds = drv.Create('tmp/test_hfa.img', 64, 64, 1, gdal.GDT_UInt16, options = [ 'COMPRESSED=YES'] )
    import struct
    data = struct.pack('H' * 64, 0, 65535, 0, 65535, 0, 65535, 0, 65535,  0, 65535, 0, 65535, 0, 65535, 0, 65535, 0, 65535, 0, 65535, 0, 65535, 0, 65535,  0, 65535, 0, 65535, 0, 65535, 0, 65535, 0, 65535, 0, 65535, 0, 65535, 0, 65535,  0, 65535, 0, 65535, 0, 65535, 0, 65535, 0, 65535, 0, 65535, 0, 65535, 0, 65535,  0, 65535, 0, 65535, 0, 65535, 0, 65535)
    for i in range(64):
        ds.GetRasterBand(1).WriteRaster( 0, i, 64, 1, data)
    ds = None

    drv.Delete( 'tmp/test_hfa.img' )

    return 'success'

###############################################################################
# Get the driver, and verify a few things about it. 

init_list = [ \
    ('byte.tif', 1, 4672, None),
    ('int16.tif', 1, 4672, None),
    ('uint16.tif', 1, 4672, None),
    ('int32.tif', 1, 4672, None),
    ('uint32.tif', 1, 4672, None),
    ('float32.tif', 1, 4672, None),
    ('float64.tif', 1, 4672, None),
    ('cfloat32.tif', 1, 5028, None),
    ('cfloat64.tif', 1, 5028, None),
    ('utmsmall.tif', 1, 50054, None) ]

gdaltest_list = [ hfa_write_desc,
                  hfa_write_4bit,
                  hfa_write_4bit_compressed,
                  hfa_write_nd_invalid,
                  hfa_update_overviews,
                  hfa_clean_external_overviews,
                  hfa_bug_2525 ]

# full set of tests for normal mode.

for item in init_list:
    ut1 = gdaltest.GDALTest( 'HFA', item[0], item[1], item[2] )
    if ut1 is None:
	print( 'HFA tests skipped' )
    gdaltest_list.append( (ut1.testCreateCopy, item[0]) )
    gdaltest_list.append( (ut1.testCreate, item[0]) )
    gdaltest_list.append( (ut1.testSetGeoTransform, item[0]) )
    gdaltest_list.append( (ut1.testSetProjection, item[0]) )
    gdaltest_list.append( (ut1.testSetMetadata, item[0]) )

# Just a few for spill file, and compressed support.
short_list = [ \
    ('byte.tif', 1, 4672, None),
    ('uint16.tif', 1, 4672, None),
    ('float64.tif', 1, 4672, None) ]

for item in short_list:
    ut2 = gdaltest.GDALTest( 'HFA', item[0], item[1], item[2],
                             options = [ 'USE_SPILL=YES' ] )
    if ut2 is None:
	print( 'HFA tests skipped' )
    gdaltest_list.append( (ut2.testCreateCopy, item[0] + ' (spill)') )
    gdaltest_list.append( (ut2.testCreate, item[0] + ' (spill)') )

    ut2 = gdaltest.GDALTest( 'HFA', item[0], item[1], item[2],
                             options = [ 'COMPRESS=YES' ] )
    if ut2 is None:
	print( 'HFA tests skipped' )
#    gdaltest_list.append( (ut2.testCreateCopy, item[0] + ' (compressed)') )
    gdaltest_list.append( (ut2.testCreate, item[0] + ' (compressed)') )


if __name__ == '__main__':

    gdaltest.setup_run( 'hfa_write' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

