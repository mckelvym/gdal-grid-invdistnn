#!/usr/bin/env python
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  gdalwarp testing
# Author:   Even Rouault <even dot rouault @ mines-paris dot org>
# 
###############################################################################
# Copyright (c) 2008, Even Rouault <even dot rouault @ mines-paris dot org>
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

import sys
import os
import stat

sys.path.append( '../pymod' )

import gdal
import gdaltest
import test_cli_utilities

###############################################################################
# Simple test

def test_gdalwarp_1():
    if test_cli_utilities.get_gdalwarp_path() is None:
        return 'skip'

    gdaltest.runexternal(test_cli_utilities.get_gdalwarp_path() + ' ../gcore/data/byte.tif tmp/testgdalwarp1.tif')

    ds = gdal.Open('tmp/testgdalwarp1.tif')
    if ds is None:
        return 'fail'

    if ds.GetRasterBand(1).Checksum() != 4672:
        gdaltest.post_reason('Bad checksum')
        return 'fail'

    ds = None

    return 'success'


###############################################################################
# Test -of option

def test_gdalwarp_2():
    if test_cli_utilities.get_gdalwarp_path() is None:
        return 'skip'

    gdaltest.runexternal(test_cli_utilities.get_gdalwarp_path() + ' -of GTiff ../gcore/data/byte.tif tmp/testgdalwarp2.tif')

    ds = gdal.Open('tmp/testgdalwarp2.tif')
    if ds is None:
        return 'fail'

    if ds.GetRasterBand(1).Checksum() != 4672:
        gdaltest.post_reason('Bad checksum')
        return 'fail'

    ds = None

    return 'success'


###############################################################################
# Test -ot option

def test_gdalwarp_3():
    if test_cli_utilities.get_gdalwarp_path() is None:
        return 'skip'

    gdaltest.runexternal(test_cli_utilities.get_gdalwarp_path() + ' -ot Int16 ../gcore/data/byte.tif tmp/testgdalwarp3.tif')

    ds = gdal.Open('tmp/testgdalwarp3.tif')
    if ds is None:
        return 'fail'

    if ds.GetRasterBand(1).DataType != gdal.GDT_Int16:
        gdaltest.post_reason('Bad data type')
        return 'fail'

    if ds.GetRasterBand(1).Checksum() != 4672:
        gdaltest.post_reason('Bad checksum')
        return 'fail'

    ds = None

    return 'success'

###############################################################################
# Test -t_srs option

def test_gdalwarp_4():
    if test_cli_utilities.get_gdalwarp_path() is None:
        return 'skip'

    gdaltest.runexternal(test_cli_utilities.get_gdalwarp_path() + ' -t_srs EPSG:32611 ../gcore/data/byte.tif tmp/testgdalwarp4.tif')

    ds = gdal.Open('tmp/testgdalwarp4.tif')
    if ds is None:
        return 'fail'

    if ds.GetRasterBand(1).Checksum() != 4672:
        gdaltest.post_reason('Bad checksum')
        return 'fail'

    ds = None

    return 'success'

###############################################################################
# Test warping from GCPs without any explicit option

def test_gdalwarp_5():
    if test_cli_utilities.get_gdalwarp_path() is None:
        return 'skip'

    if test_cli_utilities.get_gdal_translate_path() is None:
        return 'skip'

    gdaltest.runexternal(test_cli_utilities.get_gdal_translate_path() + ' -a_srs EPSG:26711 -gcp 0 0  440720.000 3751320.000 -gcp 20 0 441920.000 3751320.000 -gcp 20 20 441920.000 3750120.000 0 -gcp 0 20 440720.000 3750120.000 ../gcore/data/byte.tif tmp/testgdalwarp_gcp.tif')
    
    gdaltest.runexternal(test_cli_utilities.get_gdalwarp_path() + ' tmp/testgdalwarp_gcp.tif tmp/testgdalwarp5.tif')

    ds = gdal.Open('tmp/testgdalwarp5.tif')
    if ds is None:
        return 'fail'

    if ds.GetRasterBand(1).Checksum() != 4672:
        gdaltest.post_reason('Bad checksum')
        return 'fail'

    if not gdaltest.geotransform_equals(gdal.Open('../gcore/data/byte.tif').GetGeoTransform(), ds.GetGeoTransform(), 1e-9) :
        gdaltest.post_reason('Bad geotransform')
        return 'fail'

    ds = None

    return 'success'


###############################################################################
# Test warping from GCPs with -tps

def test_gdalwarp_6():
    if test_cli_utilities.get_gdalwarp_path() is None:
        return 'skip'

    gdaltest.runexternal(test_cli_utilities.get_gdalwarp_path() + ' -tps tmp/testgdalwarp_gcp.tif tmp/testgdalwarp6.tif')

    ds = gdal.Open('tmp/testgdalwarp6.tif')
    if ds is None:
        return 'fail'

    if ds.GetRasterBand(1).Checksum() != 4672:
        gdaltest.post_reason('Bad checksum')
        return 'fail'

    if not gdaltest.geotransform_equals(gdal.Open('../gcore/data/byte.tif').GetGeoTransform(), ds.GetGeoTransform(), 1e-9) :
        gdaltest.post_reason('Bad geotransform')
        return 'fail'

    ds = None

    return 'success'


###############################################################################
# Test -tr

def test_gdalwarp_7():
    if test_cli_utilities.get_gdalwarp_path() is None:
        return 'skip'

    gdaltest.runexternal(test_cli_utilities.get_gdalwarp_path() + ' -tr 120 120 tmp/testgdalwarp_gcp.tif tmp/testgdalwarp7.tif')

    ds = gdal.Open('tmp/testgdalwarp7.tif')
    if ds is None:
        return 'fail'

    expected_gt = (440720.0, 120.0, 0.0, 3751320.0, 0.0, -120.0)
    if not gdaltest.geotransform_equals(expected_gt, ds.GetGeoTransform(), 1e-9) :
        gdaltest.post_reason('Bad geotransform')
        return 'fail'

    ds = None

    return 'success'

###############################################################################
# Test -ts

def test_gdalwarp_8():
    if test_cli_utilities.get_gdalwarp_path() is None:
        return 'skip'

    gdaltest.runexternal(test_cli_utilities.get_gdalwarp_path() + ' -ts 10 10 tmp/testgdalwarp_gcp.tif tmp/testgdalwarp8.tif')

    ds = gdal.Open('tmp/testgdalwarp8.tif')
    if ds is None:
        return 'fail'

    expected_gt = (440720.0, 120.0, 0.0, 3751320.0, 0.0, -120.0)
    if not gdaltest.geotransform_equals(expected_gt, ds.GetGeoTransform(), 1e-9) :
        gdaltest.post_reason('Bad geotransform')
        return 'fail'

    ds = None

    return 'success'

###############################################################################
# Test -te

def test_gdalwarp_9():
    if test_cli_utilities.get_gdalwarp_path() is None:
        return 'skip'

    gdaltest.runexternal(test_cli_utilities.get_gdalwarp_path() + ' -te 440720.000 3750120.000 441920.000 3751320.000 tmp/testgdalwarp_gcp.tif tmp/testgdalwarp9.tif')

    ds = gdal.Open('tmp/testgdalwarp9.tif')
    if ds is None:
        return 'fail'

    if not gdaltest.geotransform_equals(gdal.Open('../gcore/data/byte.tif').GetGeoTransform(), ds.GetGeoTransform(), 1e-9) :
        gdaltest.post_reason('Bad geotransform')
        return 'fail'

    ds = None

    return 'success'

###############################################################################
# Test -rn

def test_gdalwarp_10():
    if test_cli_utilities.get_gdalwarp_path() is None:
        return 'skip'

    gdaltest.runexternal(test_cli_utilities.get_gdalwarp_path() + ' -ts 40 40 -rn tmp/testgdalwarp_gcp.tif tmp/testgdalwarp10.tif')

    ds = gdal.Open('tmp/testgdalwarp10.tif')
    if ds is None:
        return 'fail'

    if ds.GetRasterBand(1).Checksum() != 18784:
        print(ds.GetRasterBand(1).Checksum())
        gdaltest.post_reason('Bad checksum')
        return 'fail'

    ds = None

    return 'success'

###############################################################################
# Test -rb

def test_gdalwarp_11():
    if test_cli_utilities.get_gdalwarp_path() is None:
        return 'skip'

    gdaltest.runexternal(test_cli_utilities.get_gdalwarp_path() + ' -ts 40 40 -rb tmp/testgdalwarp_gcp.tif tmp/testgdalwarp11.tif')

    ds = gdal.Open('tmp/testgdalwarp11.tif')
    if ds is None:
        return 'fail'

    ref_ds = gdal.Open('ref_data/testgdalwarp11.tif')
    maxdiff = gdaltest.compare_ds(ds, ref_ds, verbose=0)
    ref_ds = None

    if maxdiff > 1:
        gdaltest.compare_ds(ds, ref_ds, verbose=1)
        gdaltest.post_reason('Image too different from reference')
        return 'fail'

    ds = None

    return 'success'


###############################################################################
# Test -rc

def test_gdalwarp_12():
    if test_cli_utilities.get_gdalwarp_path() is None:
        return 'skip'

    gdaltest.runexternal(test_cli_utilities.get_gdalwarp_path() + ' -ts 40 40 -rc tmp/testgdalwarp_gcp.tif tmp/testgdalwarp12.tif')

    ds = gdal.Open('tmp/testgdalwarp12.tif')
    if ds is None:
        return 'fail'

    ref_ds = gdal.Open('ref_data/testgdalwarp12.tif')
    maxdiff = gdaltest.compare_ds(ds, ref_ds, verbose=0)
    ref_ds = None

    if maxdiff > 1:
        gdaltest.compare_ds(ds, ref_ds, verbose=1)
        gdaltest.post_reason('Image too different from reference')
        return 'fail'

    ds = None

    return 'success'


###############################################################################
# Test -rsc

def test_gdalwarp_13():
    if test_cli_utilities.get_gdalwarp_path() is None:
        return 'skip'

    gdaltest.runexternal(test_cli_utilities.get_gdalwarp_path() + ' -ts 40 40 -rcs tmp/testgdalwarp_gcp.tif tmp/testgdalwarp13.tif')

    ds = gdal.Open('tmp/testgdalwarp13.tif')
    if ds is None:
        return 'fail'

    ref_ds = gdal.Open('ref_data/testgdalwarp13.tif')
    maxdiff = gdaltest.compare_ds(ds, ref_ds)
    ref_ds = None

    if maxdiff > 1:
        gdaltest.post_reason('Image too different from reference')
        return 'fail'

    ds = None

    return 'success'


###############################################################################
# Test -r lanczos

def test_gdalwarp_14():
    if test_cli_utilities.get_gdalwarp_path() is None:
        return 'skip'

    gdaltest.runexternal(test_cli_utilities.get_gdalwarp_path() + ' -ts 40 40 -r lanczos tmp/testgdalwarp_gcp.tif tmp/testgdalwarp14.tif')

    ds = gdal.Open('tmp/testgdalwarp14.tif')
    if ds is None:
        return 'fail'

    ref_ds = gdal.Open('ref_data/testgdalwarp14.tif')
    maxdiff = gdaltest.compare_ds(ds, ref_ds)
    ref_ds = None

    if maxdiff > 1:
        gdaltest.post_reason('Image too different from reference')
        return 'fail'

    ds = None

    return 'success'

###############################################################################
# Test -dstnodata

def test_gdalwarp_15():
    if test_cli_utilities.get_gdalwarp_path() is None:
        return 'skip'

    gdaltest.runexternal(test_cli_utilities.get_gdalwarp_path() + ' -dstnodata 1 -t_srs EPSG:32610 tmp/testgdalwarp_gcp.tif tmp/testgdalwarp15.tif')

    ds = gdal.Open('tmp/testgdalwarp15.tif')
    if ds is None:
        return 'fail'

    if ds.GetRasterBand(1).GetNoDataValue() != 1:
        print(ds.GetRasterBand(1).GetNoDataValue())
        gdaltest.post_reason('Bad nodata value')
        return 'fail'

    if ds.GetRasterBand(1).Checksum() != 4523:
        print(ds.GetRasterBand(1).Checksum())
        gdaltest.post_reason('Bad checksum')
        return 'fail'

    ds = None

    return 'success'

###############################################################################
# Test -of VRT which is a special case

def test_gdalwarp_16():
    if test_cli_utilities.get_gdalwarp_path() is None:
        return 'skip'

    gdaltest.runexternal(test_cli_utilities.get_gdalwarp_path() + ' -of VRT tmp/testgdalwarp_gcp.tif tmp/testgdalwarp16.vrt')

    ds = gdal.Open('tmp/testgdalwarp16.vrt')
    if ds is None:
        return 'fail'

    if ds.GetRasterBand(1).Checksum() != 4672:
        print(ds.GetRasterBand(1).Checksum())
        gdaltest.post_reason('Bad checksum')
        return 'fail'

    ds = None

    return 'success'

###############################################################################
# Test -dstalpha

def test_gdalwarp_17():
    if test_cli_utilities.get_gdalwarp_path() is None:
        return 'skip'

    gdaltest.runexternal(test_cli_utilities.get_gdalwarp_path() + ' -dstalpha ../gcore/data/rgbsmall.tif tmp/testgdalwarp17.tif')

    ds = gdal.Open('tmp/testgdalwarp17.tif')
    if ds is None:
        return 'fail'

    if ds.GetRasterBand(4) is None:
        gdaltest.post_reason('No alpha band generated')
        return 'fail'

    ds = None

    return 'success'

###############################################################################
# Test -wm -multi

def test_gdalwarp_18():
    if test_cli_utilities.get_gdalwarp_path() is None:
        return 'skip'

    (ret_stdout, ret_stderr) = gdaltest.runexternal_out_and_err(test_cli_utilities.get_gdalwarp_path() + ' -wm 20 -multi ../gcore/data/byte.tif tmp/testgdalwarp18.tif')

    # This error will be returned if GDAL is not compiled with thread support
    if ret_stderr.find('CPLCreateThread() failed in ChunkAndWarpMulti()') != -1:
        print('GDAL not compiled with thread support')
        return 'skip'

    ds = gdal.Open('tmp/testgdalwarp18.tif')
    if ds is None:
        return 'fail'

    if ds.GetRasterBand(1).Checksum() != 4672:
        gdaltest.post_reason('Bad checksum')
        return 'fail'

    ds = None

    return 'success'

###############################################################################
# Test -et 0 which is a special case

def test_gdalwarp_19():
    if test_cli_utilities.get_gdalwarp_path() is None:
        return 'skip'

    gdaltest.runexternal(test_cli_utilities.get_gdalwarp_path() + ' -et 0 tmp/testgdalwarp_gcp.tif tmp/testgdalwarp19.tif')

    ds = gdal.Open('tmp/testgdalwarp19.tif')
    if ds is None:
        return 'fail'

    if ds.GetRasterBand(1).Checksum() != 4672:
        print(ds.GetRasterBand(1).Checksum())
        gdaltest.post_reason('Bad checksum')
        return 'fail'

    ds = None

    return 'success'

###############################################################################
# Test -of VRT -et 0 which is a special case

def test_gdalwarp_20():
    if test_cli_utilities.get_gdalwarp_path() is None:
        return 'skip'

    gdaltest.runexternal(test_cli_utilities.get_gdalwarp_path() + ' -of VRT -et 0 tmp/testgdalwarp_gcp.tif tmp/testgdalwarp20.vrt')

    ds = gdal.Open('tmp/testgdalwarp20.vrt')
    if ds is None:
        return 'fail'

    if ds.GetRasterBand(1).Checksum() != 4672:
        print(ds.GetRasterBand(1).Checksum())
        gdaltest.post_reason('Bad checksum')
        return 'fail'

    ds = None

    return 'success'


###############################################################################
# Test cutline from OGR datasource.

def test_gdalwarp_21():
    if test_cli_utilities.get_gdalwarp_path() is None:
        return 'skip'

    gdaltest.runexternal(test_cli_utilities.get_gdalwarp_path() + ' ../gcore/data/utmsmall.tif tmp/testgdalwarp21.tif -cutline data/cutline.vrt -cl cutline')

    ds = gdal.Open('tmp/testgdalwarp21.tif')
    if ds is None:
        return 'fail'

    if ds.GetRasterBand(1).Checksum() != 19139:
        print(ds.GetRasterBand(1).Checksum())
        gdaltest.post_reason('Bad checksum')
        return 'fail'

    ds = None

    return 'success'


###############################################################################
# Test with a cutline and an output at a different resolution.

def test_gdalwarp_22():
    if test_cli_utilities.get_gdalwarp_path() is None:
        return 'skip'

    gdaltest.runexternal(test_cli_utilities.get_gdalwarp_path() + ' ../gcore/data/utmsmall.tif tmp/testgdalwarp22.tif -cutline data/cutline.vrt -cl cutline -tr 30 30')

    ds = gdal.Open('tmp/testgdalwarp22.tif')
    if ds is None:
        return 'fail'

    if ds.GetRasterBand(1).Checksum() != 14047:
        print(ds.GetRasterBand(1).Checksum())
        gdaltest.post_reason('Bad checksum')
        return 'fail'

    ds = None

    return 'success'


###############################################################################
# Test cutline with ALL_TOUCHED enabled.

def test_gdalwarp_23():
    if test_cli_utilities.get_gdalwarp_path() is None:
        return 'skip'

    gdaltest.runexternal(test_cli_utilities.get_gdalwarp_path() + ' -wo CUTLINE_ALL_TOUCHED=TRUE ../gcore/data/utmsmall.tif tmp/testgdalwarp23.tif -cutline data/cutline.vrt -cl cutline')

    ds = gdal.Open('tmp/testgdalwarp23.tif')
    if ds is None:
        return 'fail'

    if ds.GetRasterBand(1).Checksum() != 20123:
        print(ds.GetRasterBand(1).Checksum())
        gdaltest.post_reason('Bad checksum')
        return 'fail'

    ds = None

    return 'success'

###############################################################################
# Test warping an image crossing the 180E/180W longitude (#3206)

def test_gdalwarp_24():
    if test_cli_utilities.get_gdalwarp_path() is None:
        return 'skip'
        
    ds = gdal.GetDriverByName('GTiff').Create('tmp/testgdalwarp24src.tif', 100, 100)
    ds.SetGeoTransform([179.5, 0.01, 0, 45, 0, -0.01])
    ds.SetProjection('GEOGCS["GCS_WGS_1984",DATUM["D_WGS_1984",SPHEROID["WGS_1984",6378137.0,298.257223563]],PRIMEM["Greenwich",0.0],UNIT["Degree",0.0174532925199433]]')
    ds.GetRasterBand(1).Fill(255)
    ds = None

    gdaltest.runexternal(test_cli_utilities.get_gdalwarp_path() + ' -t_srs EPSG:32660 tmp/testgdalwarp24src.tif tmp/testgdalwarp24dst.tif')

    ds = gdal.Open('tmp/testgdalwarp24dst.tif')
    if ds is None:
        return 'fail'

    if ds.GetRasterBand(1).Checksum() != 50634:
        print(ds.GetRasterBand(1).Checksum())
        gdaltest.post_reason('Bad checksum')
        return 'fail'

    ds = None

    return 'success'

###############################################################################
# Test warping a full EPSG:4326 extent to +proj=sinu (#2305)

def test_gdalwarp_25():
    if test_cli_utilities.get_gdalwarp_path() is None:
        return 'skip'

    gdaltest.runexternal(test_cli_utilities.get_gdalwarp_path() + ' -t_srs "+proj=sinu" data/w_jpeg.tiff tmp/testgdalwarp25.tif')

    ds = gdal.Open('tmp/testgdalwarp25.tif')
    if ds is None:
        return 'fail'

    if ds.GetRasterBand(1).Checksum() != 8016:
        print(ds.GetRasterBand(1).Checksum())
        gdaltest.post_reason('Bad checksum')
        return 'fail'
        
    gt = ds.GetGeoTransform()
    expected_gt = [-20037508.342789248, 78245.302611923355, 0.0, 10001965.729313632, 0.0, -77939.656898595524]
    for i in range(6):
        if abs(gt[i] - expected_gt[i]) > 1:
            print(gt)
            gdaltest.post_reason('Bad gt')
            return 'fail'

    ds = None

    return 'success'

###############################################################################
# Test warping a full EPSG:4326 extent to +proj=eck4 (#2305)

def test_gdalwarp_26():
    if test_cli_utilities.get_gdalwarp_path() is None:
        return 'skip'

    gdaltest.runexternal(test_cli_utilities.get_gdalwarp_path() + ' -t_srs "+proj=eck4" data/w_jpeg.tiff tmp/testgdalwarp26.tif')

    ds = gdal.Open('tmp/testgdalwarp26.tif')
    if ds is None:
        return 'fail'

    if ds.GetRasterBand(1).Checksum() != 8582:
        print(ds.GetRasterBand(1).Checksum())
        gdaltest.post_reason('Bad checksum')
        return 'fail'
        
    gt = ds.GetGeoTransform()
    expected_gt = [-16921202.922943164, 41752.719393322564, 0.0, 8460601.4614715818, 0.0, -41701.109109770863]
    for i in range(6):
        if abs(gt[i] - expected_gt[i]) > 1:
            print(gt)
            gdaltest.post_reason('Bad gt')
            return 'fail'

    ds = None

    return 'success'

###############################################################################
# Test warping a full EPSG:4326 extent to +proj=vandg (#2305)

def test_gdalwarp_27():
    if test_cli_utilities.get_gdalwarp_path() is None:
        return 'skip'

    gdaltest.runexternal(test_cli_utilities.get_gdalwarp_path() + ' -t_srs "+proj=vandg" data/w_jpeg.tiff tmp/testgdalwarp27.tif')

    ds = gdal.Open('tmp/testgdalwarp27.tif')
    if ds is None:
        return 'fail'

    if ds.GetRasterBand(1).Checksum() != 22006:
        print(ds.GetRasterBand(1).Checksum())
        gdaltest.post_reason('Bad checksum')
        return 'fail'
        
    gt = ds.GetGeoTransform()
    expected_gt = [-20015109.356056381, 98651.645855415176, 0.0, 20015109.356056374, 0.0, -98651.645855415176]
    for i in range(6):
        if abs(gt[i] - expected_gt[i]) > 1:
            print(gt)
            gdaltest.post_reason('Bad gt')
            return 'fail'

    ds = None

    return 'success'

###############################################################################
# Test warping a full EPSG:4326 extent to +proj=aeqd +lat_0=45 +lon_0=90 (#2305)

def test_gdalwarp_28():
    if test_cli_utilities.get_gdalwarp_path() is None:
        return 'skip'

    gdaltest.runexternal(test_cli_utilities.get_gdalwarp_path() + ' -t_srs "+proj=aeqd +lat_0=45 +lon_0=90" data/w_jpeg.tiff tmp/testgdalwarp28.tif')

    ds = gdal.Open('tmp/testgdalwarp28.tif')
    if ds is None:
        return 'fail'

    # First is GCC; Second is MSVC 6.0
    cs = ds.GetRasterBand(1).Checksum()
    if cs != 47506 and cs != 46728:
        print(ds.GetRasterBand(1).Checksum())
        gdaltest.post_reason('Bad checksum')
        return 'fail'
        
    gt = ds.GetGeoTransform()
    # First is GCC; Second is MSVC 6.0
    expected_gt1 = [-10009026.853177125, 43693.733128680084, 0.0, 5024463.6669970695, 0.0, -43693.733128680084]
    expected_gt2 = [-10009026.853177125, 43691.280523668691, 0.0, 5022121.8610583926, 0.0, -43691.280523668691]
    for i in range(6):
        if abs(gt[i] - expected_gt1[i]) > 1 and abs(gt[i] - expected_gt2[i]) > 1:
            print(gt)
            gdaltest.post_reason('Bad gt')
            return 'fail'

    ds = None

    return 'success'

###############################################################################
# Test warping a full EPSG:4326 extent to EPSG:3785 (#2305)

def test_gdalwarp_29():
    if test_cli_utilities.get_gdalwarp_path() is None:
        return 'skip'

    gdaltest.runexternal(test_cli_utilities.get_gdalwarp_path() + ' -t_srs EPSG:3785 data/w_jpeg.tiff tmp/testgdalwarp29.tif')

    ds = gdal.Open('tmp/testgdalwarp29.tif')
    if ds is None:
        return 'fail'

    if ds.GetRasterBand(1).Checksum() != 55149:
        print(ds.GetRasterBand(1).Checksum())
        gdaltest.post_reason('Bad checksum')
        return 'fail'
        
    gt = ds.GetGeoTransform()
    expected_gt = [ -20037508.342789248, 90054.726863985939, 0.0, 16213801.067583967, 0.0, -90056.750611190684 ]
    for i in range(6):
        if abs(gt[i] - expected_gt[i]) > 1:
            print(gt)
            gdaltest.post_reason('Bad gt')
            return 'fail'

    ds = None

    return 'success'

###############################################################################
# Test the effect of the -wo OPTIMIZE_SIZE=TRUE option (#3459, #1866)

def test_gdalwarp_30():
    if test_cli_utilities.get_gdalwarp_path() is None:
        return 'skip'

    # First run : no parameter
    gdaltest.runexternal(test_cli_utilities.get_gdalwarp_path() + " data/w_jpeg.tiff tmp/testgdalwarp30_1.tif  -t_srs EPSG:3785 -co COMPRESS=LZW -wm 500000  --config GDAL_CACHEMAX 1 -ts 1000 500 -co TILED=YES")

    # Second run : with  -wo OPTIMIZE_SIZE=TRUE
    gdaltest.runexternal(test_cli_utilities.get_gdalwarp_path() + " data/w_jpeg.tiff tmp/testgdalwarp30_2.tif  -t_srs EPSG:3785 -co COMPRESS=LZW -wm 500000 -wo OPTIMIZE_SIZE=TRUE  --config GDAL_CACHEMAX 1 -ts 1000 500 -co TILED=YES")

    file_size1 = os.stat('tmp/testgdalwarp30_1.tif')[stat.ST_SIZE]
    file_size2 = os.stat('tmp/testgdalwarp30_2.tif')[stat.ST_SIZE]

    ds = gdal.Open('tmp/testgdalwarp30_1.tif')
    if ds is None:
        return 'fail'

    if ds.GetRasterBand(1).Checksum() != 64629:
        print(ds.GetRasterBand(1).Checksum())
        gdaltest.post_reason('Bad checksum on testgdalwarp30_1')
        return 'fail'

    ds = None

    ds = gdal.Open('tmp/testgdalwarp30_2.tif')
    if ds is None:
        return 'fail'

    if ds.GetRasterBand(1).Checksum() != 64629:
        print(ds.GetRasterBand(1).Checksum())
        gdaltest.post_reason('Bad checksum on testgdalwarp30_2')
        return 'fail'

    ds = None

    if file_size1 <= file_size2:
        print(file_size1)
        print(file_size2)
        gdaltest.post_reason('Size with -wo OPTIMIZE_SIZE=TRUE larger than without !')
        return 'fail'

    return 'success'
    
###############################################################################
# Cleanup

def test_gdalwarp_cleanup():

    # We don't clean up when run in debug mode.
    if gdal.GetConfigOption( 'CPL_DEBUG', 'OFF' ) == 'ON':
        return 'success'
    
    for i in range(29):
        try:
            os.remove('tmp/testgdalwarp' + str(i+1) + '.tif')
        except:
            pass
        try:
            os.remove('tmp/testgdalwarp' + str(i+1) + '.vrt')
        except:
            pass
        try:
            os.remove('tmp/testgdalwarp' + str(i+1) + '.tif.aux.xml')
        except:
            pass
    try:
        os.remove('tmp/testgdalwarp_gcp.tif')
    except:
        pass
    try:
        os.remove('tmp/testgdalwarp24src.tif')
    except:
        pass
    try:
        os.remove('tmp/testgdalwarp24dst.tif')
    except:
        pass
    try:
        os.remove('tmp/testgdalwarp30_1.tif')
    except:
        pass
    try:
        os.remove('tmp/testgdalwarp30_2.tif')
    except:
        pass
    return 'success'

gdaltest_list = [
    test_gdalwarp_cleanup,
    test_gdalwarp_1,
    test_gdalwarp_2,
    test_gdalwarp_3,
    test_gdalwarp_4,
    test_gdalwarp_5,
    test_gdalwarp_6,
    test_gdalwarp_7,
    test_gdalwarp_8,
    test_gdalwarp_9,
    test_gdalwarp_10,
    test_gdalwarp_11,
    test_gdalwarp_12,
    test_gdalwarp_13,
    test_gdalwarp_14,
    test_gdalwarp_15,
    test_gdalwarp_16,
    test_gdalwarp_17,
    test_gdalwarp_18,
    test_gdalwarp_19,
    test_gdalwarp_20,
    test_gdalwarp_21,
    test_gdalwarp_22,
    test_gdalwarp_23,
    test_gdalwarp_24,
    test_gdalwarp_25,
    test_gdalwarp_26,
    test_gdalwarp_27,
    test_gdalwarp_28,
    test_gdalwarp_29,
    test_gdalwarp_30,
    test_gdalwarp_cleanup
    ]

if __name__ == '__main__':

    gdaltest.setup_run( 'test_gdalwarp' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()
