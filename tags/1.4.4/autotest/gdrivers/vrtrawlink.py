#!/usr/bin/env python
###############################################################################
# $Id: vrtrawlink.py,v 1.2 2006/10/27 04:27:12 fwarmerdam Exp $
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test VRTRawRasterBand support.
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
# 
#  $Log: vrtrawlink.py,v $
#  Revision 1.2  2006/10/27 04:27:12  fwarmerdam
#  fixed license text
#
#  Revision 1.1  2004/07/30 21:49:24  warmerda
#  New
#
#

import os
import sys
import gdal

sys.path.append( '../pymod' )

import gdaltest

###############################################################################
# Verify reading from simple existing raw definition.

def vrtrawlink_1():

    tst = gdaltest.GDALTest( 'VRT', 'small.vrt', 2, 12816 )
    return tst.testOpen()

###############################################################################
# Create a new VRT raw link via the AddBand() method.

def vrtrawlink_2():

    driver = gdal.GetDriverByName( "VRT" );
    ds = driver.Create( 'tmp/rawlink.vrt', 31, 35, 0 )

    options = [
        'subClass=VRTRawRasterBand',
        'SourceFilename=data/small.raw'
        ]
    
    result = ds.AddBand( gdal.GDT_Byte, options )
    if result != gdal.CE_None:
        gdaltest.post_reason( 'AddBand() returned error code' )
        return 'fail'

    band = ds.GetRasterBand(1)
    chksum = band.Checksum()

    if chksum != 12481:
        gdaltest.post_reason('Wrong checksum')
        return 'fail'

    # Force it to be written to disk. 
    ds = None

    return 'success'

###############################################################################
# Confirm that the newly written file is was saved properly

def vrtrawlink_3():

    gdaltest.rawlink_ds = gdal.Open( 'tmp/rawlink.vrt', gdal.GA_Update )
    band = gdaltest.rawlink_ds.GetRasterBand(1)
    chksum = band.Checksum()

    if chksum != 12481:
        gdaltest.post_reason('Wrong checksum')
        return 'fail'

    return 'success'

###############################################################################
# Add a new band, and we will test if we can write to it. 

def vrtrawlink_4():

    # force creation of the file.
    open( 'tmp/rawlink.dat', 'w' ).write( chr(0) )

    # Add a new band pointing to this bogus file. 
    options = [
        'subClass=VRTRawRasterBand',
        'SourceFilename=tmp/rawlink.dat',
        'relativeToVRT=0',
        'ImageOffset=100',
        'PixelOffset=3',
        'LineOffset=93',
        'ByteOrder=MSB'
        ]

    result = gdaltest.rawlink_ds.AddBand( gdal.GDT_UInt16, options )
    if result != gdal.CE_None:
        gdaltest.post_reason( 'AddBand() returned error code' )
        return 'fail'

    # write out some simple data.
    band_1 = gdaltest.rawlink_ds.GetRasterBand(1)
    byte_data = band_1.ReadRaster(0,0,31,35)

    band = gdaltest.rawlink_ds.GetRasterBand(2)
    band.WriteRaster(0,0,31,35,byte_data,31,35,gdal.GDT_Byte)

    gdaltest.rawlink_ds.FlushCache()

    # Verify it seems to be right.
    chksum = band.Checksum()

    if chksum != 12481:
        gdaltest.post_reason('Wrong checksum')
        return 'fail'

    # Close and reopen to ensure we are getting data from disk.
    gdaltest_rawlink_ds = None
    gdaltest.rawlink_ds = gdal.Open( 'tmp/rawlink.vrt', gdal.GA_Update )

    band = gdaltest.rawlink_ds.GetRasterBand(2)
    chksum = band.Checksum()

    if chksum != 12481:
        gdaltest.post_reason('Wrong checksum')
        return 'fail'

    # verify file length.
    statinfo = os.stat( 'tmp/rawlink.dat' )
    if statinfo.st_size != 3354:
        gdaltest.post_reason( 'data file is wrong size' )
        return 'fail'

    return 'success'

###############################################################################
# Cleanup.

def vrtrawlink_cleanup():
    gdaltest.rawlink_ds = None
        
    try:
        os.remove( 'tmp/rawlink.vrt' )
        os.remove( 'tmp/rawlink.dat' )
    except:
        pass
    return 'success'

gdaltest_list = [
    vrtrawlink_1,
    vrtrawlink_2,
    vrtrawlink_3,
    vrtrawlink_4,
    vrtrawlink_cleanup ]

if __name__ == '__main__':

    gdaltest.setup_run( 'vrtrawlink' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

