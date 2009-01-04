#!/usr/bin/env python
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test read/write functionality for MrSID driver.
# Author:   Frank Warmerdam <warmerdam@pobox.com>
# 
###############################################################################
# Copyright (c) 2005, Frank Warmerdam <warmerdam@pobox.com>
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

sys.path.append( '../pymod' )

import gdaltest

###############################################################################
# Read a simple byte file, checking projections and geotransform.

def mrsid_1():

    gdaltest.mrsid_drv = gdal.GetDriverByName( 'MrSID' )
    if gdaltest.mrsid_drv is None:
        return 'skip'

    tst = gdaltest.GDALTest( 'MrSID', 'mercator.sid', 1, None )

    gt = (-15436.385771224039, 60.0, 0.0, 3321987.8617962394, 0.0, -60.0)
    #
    # Old, internally generated.
    # 
    prj = """PROJCS["MER         E000|",
    GEOGCS["NAD27",
        DATUM["North_American_Datum_1927",
            SPHEROID["Clarke 1866",6378206.4,294.9786982138982,
                AUTHORITY["EPSG","7008"]],
            AUTHORITY["EPSG","6267"]],
        PRIMEM["Greenwich",0],
        UNIT["degree",0.0174532925199433],
        AUTHORITY["EPSG","4267"]],
    PROJECTION["Mercator_1SP"],
    PARAMETER["latitude_of_origin",0],
    PARAMETER["central_meridian",0],
    PARAMETER["scale_factor",1],
    PARAMETER["false_easting",0],
    PARAMETER["false_northing",0],
    UNIT["metre",1,
        AUTHORITY["EPSG","9001"]]]"""
    #
    # MrSID SDK getWKT() method.
    # 
    prj = """PROJCS["MER         E000|",
    GEOGCS["NAD27",
        DATUM["North_American_Datum_1927",
            SPHEROID["Clarke 1866",6378206.4,294.9786982139006,
                AUTHORITY["EPSG","7008"]],
            AUTHORITY["EPSG","6267"]],
        PRIMEM["Greenwich",0],
        UNIT["degree",0.0174532925199433],
        AUTHORITY["EPSG","4267"]],
    PROJECTION["Mercator_1SP"],
    PARAMETER["latitude_of_origin",1],
    PARAMETER["central_meridian",1],
    PARAMETER["scale_factor",1],
    PARAMETER["false_easting",1],
    PARAMETER["false_northing",1],
    UNIT["metre",1,
        AUTHORITY["EPSG","9001"]]]"""
    
    return tst.testOpen( check_gt = gt, check_prj = prj, \
        check_stat = (0.0, 255.0, 103.319, 55.153), \
        check_approx_stat = (2.0, 243.0, 103.131, 43.978) )

###############################################################################
# Do a direct IO to read the image at a resolution for which there is no
# builtin overview.  Checks for the bug Steve L found in the optimized
# RasterIO implementation.

def mrsid_2():

    if gdaltest.mrsid_drv is None:
        return 'skip'

    ds = gdal.Open( 'data/mercator.sid' )

    try:
        data = ds.ReadRaster( 0, 0, 515, 515, buf_xsize = 10, buf_ysize = 10 )
    except:
        gdaltest.post_reason( 'Small overview read failed: ' + gdal.GetLastErrorMsg() )
        return 'fail'

    ds = None
    
    # check that we got roughly the right values by checking mean.
    sum = 0
    for i in range(len(data)):
        sum = sum + ord(data[i])

    mean = float(sum) / len(data)

    if mean < 95 or mean > 105:
        gdaltest.post_reason( 'image mean out of range.' )
        return 'fail'
    
    return 'success'

###############################################################################
# Test overview reading.

def mrsid_3():

    if gdaltest.mrsid_drv is None:
        return 'skip'

    ds = gdal.Open( 'data/mercator.sid' )

    band = ds.GetRasterBand(1)
    if band.GetOverviewCount() != 4:
        gdaltest.post_reason( 'did not get expected overview count' )
        return 'fail'

    new_stat = band.GetOverview(3).GetStatistics(0,1)
    
    check_stat = (11.0, 230.0, 103.42607897153351, 39.952592422557757)
    
    stat_epsilon = 0.0001
    for i in range(4):
        if abs(new_stat[i]-check_stat[i]) > stat_epsilon:
            print
            print 'old = ', check_stat
            print 'new = ', new_stat
            post_reason( 'Statistics differ.' )
            return 'fail'
    
    return 'success'

###############################################################################
# Check a new (V3) file which uses a different form for coordinate sys.

def mrsid_4():

    if gdaltest.mrsid_drv is None:
        return 'skip'

    tst = gdaltest.GDALTest( 'MrSID', 'mercator_new.sid', 1, None )

    gt = (-15436.385771224039, 60.0, 0.0, 3321987.8617962394, 0.0, -60.0)
    prj = """PROJCS["MER         E000",
    GEOGCS["NAD27",
        DATUM["North_American_Datum_1927",
            SPHEROID["Clarke 1866",6378206.4,294.9786982138982,
                AUTHORITY["EPSG","7008"]],
            AUTHORITY["EPSG","6267"]],
        PRIMEM["Greenwich",0],
        UNIT["degree",0.0174532925199433],
        AUTHORITY["EPSG","4267"]],
    PROJECTION["Mercator_1SP"],
    PARAMETER["latitude_of_origin",33.76446202777777],
    PARAMETER["central_meridian",-117.4745428888889],
    PARAMETER["scale_factor",1],
    PARAMETER["false_easting",0],
    PARAMETER["false_northing",0],
    UNIT["metre",1,
        AUTHORITY["EPSG","9001"]]]"""
    
    return tst.testOpen( check_gt = gt, check_prj = prj, \
        check_stat = (0.0, 255.0, 103.112, 52.477), \
        check_approx_stat = (0.0, 255.0, 102.684, 51.614) )

###############################################################################
# Test JP2MrSID driver

def mrsid_5():
    gdaltest.jp2kak_drv = None
    gdaltest.jpeg2000_drv = None
    gdaltest.jp2ecw_drv = None

    gdaltest.jp2mrsid_drv = gdal.GetDriverByName( 'JP2MrSID' )
    if gdaltest.jp2mrsid_drv is None:
        return 'skip'

    # Deregister other potential conflicting JPEG2000 drivers that will
    # be re-registered in the cleanup
    try:
        if gdal.GetDriverByName( 'JP2KAK' ):
            print 'Deregistering JP2KAK'
            gdaltest.jp2kak_drv = gdal.GetDriverByName('JP2KAK')
            gdaltest.jp2kak_drv.Deregister()
    except:
        pass

    try:
        if gdal.GetDriverByName( 'JPEG2000' ):
            print 'Deregistering JPEG2000'
            gdaltest.jpeg2000_drv = gdal.GetDriverByName('JPEG2000')
            gdaltest.jpeg2000_drv.Deregister()
    except:
        pass

    try:
        if gdal.GetDriverByName( 'JP2ECW' ):
            print 'Deregistering JP2ECW'
            gdaltest.jp2ecw_drv = gdal.GetDriverByName('JP2ECW')
            gdaltest.jp2ecw_drv.Deregister()
    except:
        pass

    return 'success'
	
###############################################################################
# Open byte.jp2

def mrsid_6():

    if gdaltest.jp2mrsid_drv is None:
	return 'skip'

    srs = """PROJCS["NAD27 / UTM zone 11N",
    GEOGCS["NAD27",
        DATUM["North_American_Datum_1927",
            SPHEROID["Clarke 1866",6378206.4,294.9786982138982,
                AUTHORITY["EPSG","7008"]],
            AUTHORITY["EPSG","6267"]],
        PRIMEM["Greenwich",0],
        UNIT["degree",0.0174532925199433],
        AUTHORITY["EPSG","4267"]],
    PROJECTION["Transverse_Mercator"],
    PARAMETER["latitude_of_origin",0],
    PARAMETER["central_meridian",-117],
    PARAMETER["scale_factor",0.9996],
    PARAMETER["false_easting",500000],
    PARAMETER["false_northing",0],
    UNIT["metre",1,
        AUTHORITY["EPSG","9001"]],
    AUTHORITY["EPSG","26711"]]
"""  
    gt = (440720.0, 60.0, 0.0, 3751320.0, 0.0, -60.0)
    
    tst = gdaltest.GDALTest( 'JP2MrSID', 'byte.jp2', 1, 50054 )
    return tst.testOpen( check_prj = srs, check_gt = gt )

	
###############################################################################
# Open int16.jp2

def mrsid_7():

    if gdaltest.jp2mrsid_drv is None:
	return 'skip'

    ds = gdal.Open( 'data/int16.tif' )
    ds_ref = gdal.Open( 'data/int16.tif' )
    
    maxdiff = gdaltest.compare_ds(ds, ds_ref)
    print ds.GetRasterBand(1).Checksum()
    print ds_ref.GetRasterBand(1).Checksum()

    ds = None
    ds_ref = None
    
    # Perfect match theoritically
    if maxdiff > 1:
        gdaltest.post_reason('Image too different from reference')
        return 'fail'

    return 'success'

###############################################################################
def mrsid_online_1():

    if gdaltest.jp2mrsid_drv is None:
        return 'skip'

    if not gdaltest.download_file('http://download.osgeo.org/gdal/data/jpeg2000/7sisters200.j2k', '7sisters200.j2k'):
        return 'skip'

    # Checksum = 29473 on my PC
    tst = gdaltest.GDALTest( 'JP2MrSID', 'tmp/cache/7sisters200.j2k', 1, None, filename_absolute = 1 )

    if tst.testOpen() != 'success':
        return 'fail'

    ds = gdal.Open('tmp/cache/7sisters200.j2k')
    ds.GetRasterBand(1).Checksum()
    ds = None

    return 'success'

###############################################################################
def mrsid_online_2():

    if gdaltest.jp2mrsid_drv is None:
        return 'skip'

    if not gdaltest.download_file('http://download.osgeo.org/gdal/data/jpeg2000/gcp.jp2', 'gcp.jp2'):
        return 'skip'

    # Checksum = 209 on my PC
    tst = gdaltest.GDALTest( 'JP2MrSID', 'tmp/cache/gcp.jp2', 1, None, filename_absolute = 1 )

    if tst.testOpen() != 'success':
        return 'fail'

    # The JP2MrSID driver doesn't handle GCPs
    ds = gdal.Open('tmp/cache/gcp.jp2')
    ds.GetRasterBand(1).Checksum()
    #if len(ds.GetGCPs()) != 15:
    #    gdaltest.post_reason('bad number of GCP')
    #    return 'fail'
    #
    #expected_wkt = """GEOGCS["WGS 84",DATUM["WGS_1984",SPHEROID["WGS 84",6378137,298.2572235629972,AUTHORITY["EPSG","7030"]],AUTHORITY["EPSG","6326"]],PRIMEM["Greenwich",0],UNIT["degree",0.0174532925199433],AUTHORITY["EPSG","4326"]]"""
    #if ds.GetGCPProjection() != expected_wkt:
    #    gdaltest.post_reason('bad GCP projection')
    #    return 'fail'

    ds = None

    return 'success'

###############################################################################
def mrsid_online_3():

    if gdaltest.jp2mrsid_drv is None:
        return 'skip'

    if not gdaltest.download_file('http://www.openjpeg.org/samples/Bretagne1.j2k', 'Bretagne1.j2k'):
        return 'skip'
    if not gdaltest.download_file('http://www.openjpeg.org/samples/Bretagne1.bmp', 'Bretagne1.bmp'):
        return 'skip'

    # checksum = 14443 on my PC
    tst = gdaltest.GDALTest( 'JP2MrSID', 'tmp/cache/Bretagne1.j2k', 1, None, filename_absolute = 1 )

    if tst.testOpen() != 'success':
        return 'fail'

    ds = gdal.Open('tmp/cache/Bretagne1.j2k')
    ds_ref = gdal.Open('tmp/cache/Bretagne1.bmp')
    maxdiff = gdaltest.compare_ds(ds, ds_ref)
    print ds.GetRasterBand(1).Checksum()
    print ds_ref.GetRasterBand(1).Checksum()

    ds = None
    ds_ref = None

    # Difference between the image before and after compression
    if maxdiff > 17:
        gdaltest.post_reason('Image too different from reference')
        return 'fail'

    return 'success'

###############################################################################
def mrsid_online_4():

    if gdaltest.jp2mrsid_drv is None:
        return 'skip'

    if not gdaltest.download_file('http://www.openjpeg.org/samples/Bretagne2.j2k', 'Bretagne2.j2k'):
        return 'skip'
    if not gdaltest.download_file('http://www.openjpeg.org/samples/Bretagne2.bmp', 'Bretagne2.bmp'):
        return 'skip'

    # Checksum = 53186 on my PC
    tst = gdaltest.GDALTest( 'JP2MrSID', 'tmp/cache/Bretagne2.j2k', 1, None, filename_absolute = 1 )

    if tst.testOpen() != 'success':
        return 'fail'

    ds = gdal.Open('tmp/cache/Bretagne2.j2k')
    ds_ref = gdal.Open('tmp/cache/Bretagne2.bmp')
    maxdiff = gdaltest.compare_ds(ds, ds_ref, width = 256, height = 256)
    print ds.GetRasterBand(1).Checksum()
    print ds_ref.GetRasterBand(1).Checksum()

    ds = None
    ds_ref = None

    # Difference between the image before and after compression
    if maxdiff > 1:
        gdaltest.post_reason('Image too different from reference')
        return 'fail'

    return 'success'
###############################################################################
# Cleanup.

def mrsid_cleanup():
    gdaltest.mrsid_drv = None

    try:
        os.remove( 'data/mercator.sid.aux.xml' )
        os.remove( 'data/mercator_new.sid.aux.xml' )
    except:
        pass
    
    try:
        gdaltest.jp2kak_drv.Register()
        print 'Registering JP2KAK'
    except:
        pass
    
    try:
        gdaltest.jpeg2000_drv.Register()
        print 'Registering JPEG2000'
    except:
        pass
    
    try:
        gdaltest.jp2ecw_drv.Register()
        print 'Registering JP2ECW'
    except:
        pass
    
    return 'success'

gdaltest_list = [
    mrsid_1,
    mrsid_2,
    mrsid_3,
    mrsid_4,
    mrsid_5,
    mrsid_6,
    mrsid_7,
    mrsid_online_1,
    mrsid_online_2,
    mrsid_online_3,
    mrsid_online_4,
    mrsid_cleanup ]

if __name__ == '__main__':

    gdaltest.setup_run( 'mrsid' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

