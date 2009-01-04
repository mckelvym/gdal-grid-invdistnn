#!/usr/bin/env python
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test read/write functionality for ECW driver.
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
import string
import array
import gdal

sys.path.append( '../pymod' )

import gdaltest

###############################################################################
# Verify we have the driver.

def ecw_1():

    gdaltest.jp2kak_drv = None
    gdaltest.jpeg2000_drv = None
    gdaltest.jp2mrsid_drv = None

    try:
        gdaltest.ecw_drv = gdal.GetDriverByName( 'ECW' )
        gdaltest.jp2ecw_drv = gdal.GetDriverByName( 'JP2ECW' )
    except:
	gdaltest.ecw_drv = None
        gdaltest.jp2ecw_drv = None
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
        if gdal.GetDriverByName( 'JP2MrSID' ):
            print 'Deregistering JP2MrSID'
            gdaltest.jp2mrsid_drv = gdal.GetDriverByName('JP2MrSID')
            gdaltest.jp2mrsid_drv.Deregister()
    except:
        pass

    return 'success'

###############################################################################
# Verify various information about our test image. 

def ecw_2():

    if gdaltest.ecw_drv is None:
	return 'skip'

    ds = gdal.Open( 'data/jrc.ecw' )
    
    (exp_mean, exp_stddev) = (141.172, 67.3636)
    (mean, stddev) = ds.GetRasterBand(1).ComputeBandStats()

    if abs(mean-exp_mean) > 0.5 or abs(stddev-exp_stddev) > 0.5:
	gdaltest.post_reason( 'mean/stddev of (%g,%g) diffs from expected(%g,%g)' % (mean, stddev,exp_mean, exp_stddev) )
	return 'fail'

    geotransform = ds.GetGeoTransform()
    if abs(geotransform[0]-467498.5) > 0.1 \
	or abs(geotransform[1]-16.5475) > 0.001 \
	or abs(geotransform[2]-0) > 0.001 \
	or abs(geotransform[3]-5077883.2825) > 0.1 \
	or abs(geotransform[4]-0) > 0.001 \
	or abs(geotransform[5]- -16.5475) > 0.001:
	print geotransform
	gdaltest.post_reason( 'geotransform differs from expected' )
	return 'fail'

    return 'success'

###############################################################################
# Verify that an write the imagery out to a new file.

def ecw_3():
    if gdaltest.ecw_drv is None:
	return 'skip'

    ds = gdal.Open( 'data/jrc.ecw' )
    gdaltest.ecw_drv.CreateCopy( 'tmp/jrc_out.ecw', ds )
    ds = None

    return 'success' 
	
###############################################################################
# Verify various information about our generated image. 

def ecw_4():

    if gdaltest.ecw_drv is None:
	return 'skip'

    ds = gdal.Open( 'tmp/jrc_out.ecw' )
    
    (exp_mean, exp_stddev) = (140.290, 66.6303)
    (mean, stddev) = ds.GetRasterBand(1).ComputeBandStats()

    if abs(mean-exp_mean) > 1.5 or abs(stddev-exp_stddev) > 0.5:
	gdaltest.post_reason( 'mean/stddev of (%g,%g) diffs from expected(%g,%g)' % (mean, stddev,exp_mean, exp_stddev) )
	return 'fail'

    geotransform = ds.GetGeoTransform()
    if abs(geotransform[0]-467498.5) > 0.1 \
	or abs(geotransform[1]-16.5475) > 0.001 \
	or abs(geotransform[2]-0) > 0.001 \
	or abs(geotransform[3]-5077883.2825) > 0.1 \
	or abs(geotransform[4]-0) > 0.001 \
	or abs(geotransform[5]- -16.5475) > 0.001:
	print geotransform
	gdaltest.post_reason( 'geotransform differs from expected' )
	return 'fail'

    ds = None

    return 'success'

###############################################################################
# Now try writing a JPEG2000 compressed version of the same with the ECW driver

def ecw_5():
    if gdaltest.jp2ecw_drv is None:
	return 'skip'

    ds = gdal.Open( 'data/small.vrt' )
    gdaltest.jp2ecw_drv.CreateCopy( 'tmp/ecw_5.jp2', ds )
#				    options = ['TARGET=100'] )
    ds = None

    return 'success' 
	
###############################################################################
# Verify various information about our generated image. 

def ecw_6():

    if gdaltest.jp2ecw_drv is None:
	return 'skip'

    ds = gdal.Open( 'tmp/ecw_5.jp2' )
    
    (exp_mean, exp_stddev) = (144.422, 44.9075)
    (mean, stddev) = ds.GetRasterBand(1).ComputeBandStats()

    # The difference in the stddev is outragously large between win32 and
    # linux, but I don't know why. 
    if abs(mean-exp_mean) > 1.5 or abs(stddev-exp_stddev) > 6:
	gdaltest.post_reason( 'mean/stddev of (%g,%g) diffs from expected(%g,%g)' % (mean, stddev,exp_mean, exp_stddev) )
	return 'fail'

    (exp_mean, exp_stddev) = (144.422, 44.9075)
    (mean, stddev) = ds.GetRasterBand(2).ComputeBandStats()

    # The difference in the stddev is outragously large between win32 and
    # linux, but I don't know why. 
    if abs(mean-exp_mean) > 1.0 or abs(stddev-exp_stddev) > 6:
	gdaltest.post_reason( 'mean/stddev of (%g,%g) diffs from expected(%g,%g)' % (mean, stddev,exp_mean, exp_stddev) )
	return 'fail'

    geotransform = ds.GetGeoTransform()
    if abs(geotransform[0]-440720) > 0.1 \
	or abs(geotransform[1]-60) > 0.001 \
	or abs(geotransform[2]-0) > 0.001 \
	or abs(geotransform[3]-3751320) > 0.1 \
	or abs(geotransform[4]-0) > 0.001 \
	or abs(geotransform[5]- -60) > 0.001:
	print geotransform
	gdaltest.post_reason( 'geotransform differs from expected' )
	return 'fail'

    prj = ds.GetProjectionRef()
    if string.find(prj,'UTM') == -1 or string.find(prj,'NAD27') == -1 \
       or string.find(prj,'one 11') == -1:
        print prj
        gdaltest.post_reason( 'Coordinate system not UTM 11, NAD27?' )
        return 'fail'

    ds = None

    return 'success'

###############################################################################
# Write the same image to NITF.

def ecw_7():
    if gdaltest.jp2ecw_drv is None:
	return 'skip'

    ds = gdal.Open( 'data/small.vrt' )
    drv = gdal.GetDriverByName( 'NITF' )
    drv.CreateCopy( 'tmp/ecw_7.ntf', ds, options = ['IC=C8'], strict = 0 )
    ds = None

    return 'success' 
	
###############################################################################
# Verify various information about our generated image. 

def ecw_8():

    if gdaltest.jp2ecw_drv is None:
	return 'skip'

    ds = gdal.Open( 'tmp/ecw_7.ntf' )
    
    (exp_mean, exp_stddev) = (145.57, 43.1712)
    (mean, stddev) = ds.GetRasterBand(1).ComputeBandStats()

    if abs(mean-exp_mean) > 1.0 or abs(stddev-exp_stddev) > 1.0:
	gdaltest.post_reason( 'mean/stddev of (%g,%g) diffs from expected(%g,%g)' % (mean, stddev,exp_mean, exp_stddev) )
	return 'fail'

    (exp_mean, exp_stddev) = (145.57, 43.1712)
    (mean, stddev) = ds.GetRasterBand(2).ComputeBandStats()

    if abs(mean-exp_mean) > 1.0 or abs(stddev-exp_stddev) > 1.0:
	gdaltest.post_reason( 'mean/stddev of (%g,%g) diffs from expected(%g,%g)' % (mean, stddev,exp_mean, exp_stddev) )
	return 'fail'

    geotransform = ds.GetGeoTransform()
    if abs(geotransform[0]-440720) > 0.1 \
	or abs(geotransform[1]-60) > 0.001 \
	or abs(geotransform[2]-0) > 0.001 \
	or abs(geotransform[3]-3751320) > 0.1 \
	or abs(geotransform[4]-0) > 0.001 \
	or abs(geotransform[5]- -60) > 0.001:
	print geotransform
	gdaltest.post_reason( 'geotransform differs from expected' )
	return 'fail'

    prj = ds.GetProjectionRef()
    if string.find(prj,'UTM Zone 11') == -1 or string.find(prj,'WGS84') == -1:
        gdaltest.post_reason( 'Coordinate system not UTM 11, WGS84?' )
        return 'fail'

    ds = None

    return 'success'

###############################################################################
# Try writing 16bit JP2 file directly using Create().

def ecw_9():
    if gdaltest.jp2ecw_drv is None:
	return 'skip'

    ds = gdaltest.jp2ecw_drv.Create( 'tmp/ecw9.jp2', 200, 100, 1,
                                     gdal.GDT_Int16 )
    ds.SetGeoTransform( (100, 0.1, 0.0, 30.0, 0.0, -0.1 ) )

    ds.SetProjection( 'GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563,AUTHORITY[\"EPSG\",\"7030\"]],TOWGS84[0,0,0,0,0,0,0],AUTHORITY[\"EPSG\",\"6326\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9108\"]],AXIS[\"Lat\",NORTH],AXIS[\"Long\",EAST],AUTHORITY[\"EPSG\",\"4326\"]]' )

    raw_data = array.array('h',range(200)).tostring()

    for line in range(100):
        ds.WriteRaster( 0, line, 200, 1, raw_data,
                        buf_type = gdal.GDT_Int16 )
    ds = None

    return 'success'

###############################################################################
# Verify previous 16bit file.

def ecw_10():
    if gdaltest.jp2ecw_drv is None:
	return 'skip'

    ds = gdal.Open( 'tmp/ecw9.jp2' )
    
    (exp_mean, exp_stddev) = (98.49, 57.7129)
    (mean, stddev) = ds.GetRasterBand(1).ComputeBandStats()

    if abs(mean-exp_mean) > 1.1 or abs(stddev-exp_stddev) > 0.1:
	gdaltest.post_reason( 'mean/stddev of (%g,%g) diffs from expected(%g,%g)' % (mean, stddev,exp_mean, exp_stddev) )
	return 'fail'

    geotransform = ds.GetGeoTransform()
    if abs(geotransform[0]-100) > 0.1 \
	or abs(geotransform[1]-0.1) > 0.001 \
	or abs(geotransform[2]-0) > 0.001 \
	or abs(geotransform[3]-30) > 0.1 \
	or abs(geotransform[4]-0) > 0.001 \
	or abs(geotransform[5]- -0.1) > 0.001:
	print geotransform
	gdaltest.post_reason( 'geotransform differs from expected' )
	return 'fail'

    # should check the projection, but I'm too lazy just now.

    return 'success' 
	

###############################################################################
# Test direct creation of an NITF/JPEG2000 file.

def ecw_11():
    if gdaltest.jp2ecw_drv is None:
	return 'skip'

    drv = gdal.GetDriverByName( 'NITF' )
    ds = drv.Create( 'tmp/test_11.ntf', 200, 100, 3, gdal.GDT_Byte,
                     [ 'ICORDS=G' ] )
    ds.SetGeoTransform( (100, 0.1, 0.0, 30.0, 0.0, -0.1 ) )

    list = range(200) + range(20,220) + range(30,230)
    raw_data = array.array('h',list).tostring()

    for line in range(100):
        ds.WriteRaster( 0, line, 200, 1, raw_data,
                        buf_type = gdal.GDT_Int16,
                        band_list = [1,2,3] )

    ds.GetRasterBand( 1 ).SetRasterColorInterpretation( gdal.GCI_BlueBand )
    ds.GetRasterBand( 2 ).SetRasterColorInterpretation( gdal.GCI_GreenBand )
    ds.GetRasterBand( 3 ).SetRasterColorInterpretation( gdal.GCI_RedBand )

    ds = None

    return 'success'

###############################################################################
# Verify previous file

def ecw_12():
    if gdaltest.jp2ecw_drv is None:
	return 'skip'

    ds = gdal.Open( 'tmp/test_11.ntf' )
    
    geotransform = ds.GetGeoTransform()
    if abs(geotransform[0]-100) > 0.1 \
	or abs(geotransform[1]-0.1) > 0.001 \
	or abs(geotransform[2]-0) > 0.001 \
	or abs(geotransform[3]-30.0) > 0.1 \
	or abs(geotransform[4]-0) > 0.001 \
	or abs(geotransform[5]- -0.1) > 0.001:
	print geotransform
	gdaltest.post_reason( 'geotransform differs from expected' )
	return 'fail'

    if ds.GetRasterBand(1).GetRasterColorInterpretation() != gdal.GCI_BlueBand:
        gdaltest.post_reason( 'Got wrong color interpretation.' )
        return 'fail'

    if ds.GetRasterBand(2).GetRasterColorInterpretation() !=gdal.GCI_GreenBand:
        gdaltest.post_reason( 'Got wrong color interpretation.' )
        return 'fail'

    if ds.GetRasterBand(3).GetRasterColorInterpretation() != gdal.GCI_RedBand:
        gdaltest.post_reason( 'Got wrong color interpretation.' )
        return 'fail'

    ds = None

    return 'success'

###############################################################################
# This is intended to verify that the ECWDataset::RasterIO() special case
# works properly.  It is used to copy subwindow into a memory dataset
# which we then checksum.  To stress the RasterIO(), we also change data
# type and select an altered band list. 

def ecw_13():
    if gdaltest.jp2ecw_drv is None:
	return 'skip'

    ds = gdal.Open( 'data/rgb16_ecwsdk.jp2' )

    wrktype = gdal.GDT_Float32
    raw_data = ds.ReadRaster( 10, 10, 40, 40, buf_type = wrktype,
                              band_list = [3,2,1] )
    ds = None

    drv = gdal.GetDriverByName( 'MEM' )
    ds = drv.Create( 'workdata', 40, 40, 3, wrktype )
    ds.WriteRaster( 0, 0, 40, 40, raw_data, buf_type = wrktype )
    
    checksums = ( ds.GetRasterBand(1).Checksum(),
                  ds.GetRasterBand(2).Checksum(),
                  ds.GetRasterBand(3).Checksum() )
    ds = None

    if checksums != ( 19253, 17848, 19127 ):
        gdaltest.post_reason( 'Expected checksums do match expected checksums')
        print checksums
        return 'fail'

    return 'success'

###############################################################################
# Write out image with GCPs.

def ecw_14():
    if gdaltest.jp2ecw_drv is None:
	return 'skip'

    ds = gdal.Open( 'data/rgb_gcp.vrt' )
    gdaltest.jp2ecw_drv.CreateCopy( 'tmp/rgb_gcp.jp2', ds )
    ds = None

    return 'success' 
	
###############################################################################
# Verify various information about our generated image. 

def ecw_15():

    if gdaltest.jp2ecw_drv is None:
	return 'skip'

    ds = gdal.Open( 'tmp/rgb_gcp.jp2' )

    gcp_srs = ds.GetGCPProjection()
    if gcp_srs[:6] != 'GEOGCS' \
       or string.find(gcp_srs,'WGS') == -1 \
       or string.find(gcp_srs,'84') == -1:
        gdaltest.post_reason('GCP Projection not retained.')
        print gcp_srs
        return 'fail'

    gcps = ds.GetGCPs()
    if len(gcps) != 4 \
       or gcps[1].GCPPixel != 0 \
       or gcps[1].GCPLine  != 50 \
       or gcps[1].GCPX     != 0 \
       or gcps[1].GCPY     != 50 \
       or gcps[1].GCPZ     != 0:
        gdaltest.post_reason( 'GCPs wrong.' )
        print gcps
        return 'fail'
    
    ds = None

    return 'success'
		
###############################################################################
# Open byte.jp2

def ecw_16():

    if gdaltest.jp2ecw_drv is None:
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
    
    tst = gdaltest.GDALTest( 'JP2ECW', 'byte.jp2', 1, 50054 )
    return tst.testOpen( check_prj = srs, check_gt = gt )

###############################################################################
# Open int16.jp2

def ecw_17():

    if gdaltest.jp2ecw_drv is None:
	return 'skip'

    ds = gdal.Open( 'data/int16.jp2' )
    ds_ref = gdal.Open( 'data/int16.tif' )
    
    maxdiff = gdaltest.compare_ds(ds, ds_ref)
    print ds.GetRasterBand(1).Checksum()
    print ds_ref.GetRasterBand(1).Checksum()

    ds = None
    ds_ref = None
    
    # Quite a bit of difference...
    if maxdiff > 6:
        gdaltest.post_reason('Image too different from reference')
        return 'fail'

    return 'success'

###############################################################################
def ecw_online_1():
    if gdaltest.jp2ecw_drv is None:
	return 'skip'
    
    if not gdaltest.download_file('http://download.osgeo.org/gdal/data/jpeg2000/7sisters200.j2k', '7sisters200.j2k'):
        return 'skip'

    # checksum = 32316 on my PC
    tst = gdaltest.GDALTest( 'JP2ECW', 'tmp/cache/7sisters200.j2k', 1, None, filename_absolute = 1 )

    if tst.testOpen() != 'success':
        return 'fail'

    ds = gdal.Open('tmp/cache/7sisters200.j2k')
    ds.GetRasterBand(1).Checksum()
    ds = None

    return 'success'

###############################################################################
def ecw_online_2():
    if gdaltest.jp2ecw_drv is None:
	return 'skip'
    
    if not gdaltest.download_file('http://download.osgeo.org/gdal/data/jpeg2000/gcp.jp2', 'gcp.jp2'):
        return 'skip'

    # checksum = 1292 on my PC
    tst = gdaltest.GDALTest( 'JP2ECW', 'tmp/cache/gcp.jp2', 1, None, filename_absolute = 1 )

    if tst.testOpen() != 'success':
        return 'fail'

    ds = gdal.Open('tmp/cache/gcp.jp2')
    ds.GetRasterBand(1).Checksum()
    if len(ds.GetGCPs()) != 15:
        gdaltest.post_reason('bad number of GCP')
        return 'fail'

    expected_wkt = """GEOGCS["WGS 84",DATUM["WGS_1984",SPHEROID["WGS 84",6378137,298.2572235629972,AUTHORITY["EPSG","7030"]],AUTHORITY["EPSG","6326"]],PRIMEM["Greenwich",0],UNIT["degree",0.0174532925199433],AUTHORITY["EPSG","4326"]]"""
    if ds.GetGCPProjection() != expected_wkt:
        gdaltest.post_reason('bad GCP projection')
        return 'fail'

    ds = None

    return 'success'

###############################################################################
def ecw_online_3():
    if gdaltest.jp2ecw_drv is None:
	return 'skip'

    if not gdaltest.download_file('http://www.openjpeg.org/samples/Bretagne1.j2k', 'Bretagne1.j2k'):
        return 'skip'
    if not gdaltest.download_file('http://www.openjpeg.org/samples/Bretagne1.bmp', 'Bretagne1.bmp'):
        return 'skip'

    # checksum = 16481 on my PC
    tst = gdaltest.GDALTest( 'JP2ECW', 'tmp/cache/Bretagne1.j2k', 1, None, filename_absolute = 1 )

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
    if maxdiff > 16:
        gdaltest.post_reason('Image too different from reference')
        return 'fail'

    return 'success'

###############################################################################
def ecw_online_4():

    if gdaltest.jp2ecw_drv is None:
        return 'skip'

    if not gdaltest.download_file('http://www.openjpeg.org/samples/Bretagne2.j2k', 'Bretagne2.j2k'):
        return 'skip'
    if not gdaltest.download_file('http://www.openjpeg.org/samples/Bretagne2.bmp', 'Bretagne2.bmp'):
        return 'skip'

    # Checksum = 53054 on my PC
    tst = gdaltest.GDALTest( 'JP2ECW', 'tmp/cache/Bretagne2.j2k', 1, None, filename_absolute = 1 )

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
def ecw_cleanup():

    #gdaltest.clean_tmp()

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
        gdaltest.jp2mrsid_drv.Register()
        print 'Registering JP2MrSID'
    except:
        pass
    
    return 'success'

gdaltest_list = [
    ecw_1,
    ecw_2,
    ecw_3,
    ecw_4,
    ecw_5,
    ecw_6,
    ecw_7,
    ecw_8,
    ecw_9,
    ecw_10,
    ecw_11,
    ecw_12,
    ecw_13,
    ecw_14,
    ecw_15,
    ecw_16,
    ecw_17,
    ecw_online_1,
    ecw_online_2,
    ecw_online_3,
    ecw_online_4,
    ecw_cleanup ]

if __name__ == '__main__':

    gdaltest.setup_run( 'ecw' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

