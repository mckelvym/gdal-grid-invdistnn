#!/usr/bin/env python
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test functioning of the PAM metadata support.
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
import shutil

sys.path.append( '../pymod' )

import gdaltest
import gdal

###############################################################################
# Check that we can read PAM metadata for existing PNM file.

def pam_1():

    gdaltest.pam_setting = gdal.GetConfigOption( 'GDAL_PAM_ENABLED', "NULL" )
    gdal.SetConfigOption( 'GDAL_PAM_ENABLED', 'YES' )

    ds = gdal.Open( "data/byte.pnm" )

    base_md = ds.GetMetadata()
    if len(base_md) != 2 or base_md['other'] != 'red' \
       or base_md['key'] != 'value':
        gdaltest.post_reason( 'Default domain metadata missing' )
        return 'fail'

    xml_md = ds.GetMetadata( 'xml:test' )

    if len(xml_md) != 1:
        gdaltest.post_reason( 'xml:test metadata missing' )
        return 'fail'

    if type(xml_md) != type(['abc']):
        gdaltest.post_reason( 'xml:test metadata not returned as list.' )
        return 'fail'

    expected_xml = """<?xml version="2.0"?>
<TestXML>Value</TestXML>
"""

    if xml_md[0] != expected_xml:
        gdaltest.post_reason( 'xml does not match' )
        print(xml_md)
        return 'fail'
    
    return 'success' 

###############################################################################
# Verify that we can write XML to a new file. 

def pam_2():

    driver = gdal.GetDriverByName( 'PNM' )
    ds = driver.Create( 'tmp/pam.pnm', 10, 10 )
    band = ds.GetRasterBand( 1 )

    band.SetMetadata( { 'other' : 'red', 'key' : 'value' } )

    expected_xml = """<?xml version="2.0"?>
<TestXML>Value</TestXML>
"""

    band.SetMetadata( [ expected_xml ], 'xml:test' )

    band.SetNoDataValue( 100 )

    ds = None

    return 'success' 

###############################################################################
# Check that we can read PAM metadata for existing PNM file.

def pam_3():

    ds = gdal.Open( "tmp/pam.pnm" )

    band = ds.GetRasterBand(1)
    base_md = band.GetMetadata()
    if len(base_md) != 2 or base_md['other'] != 'red' \
       or base_md['key'] != 'value':
        gdaltest.post_reason( 'Default domain metadata missing' )
        return 'fail'

    xml_md = band.GetMetadata( 'xml:test' )

    if len(xml_md) != 1:
        gdaltest.post_reason( 'xml:test metadata missing' )
        return 'fail'

    if type(xml_md) != type(['abc']):
        gdaltest.post_reason( 'xml:test metadata not returned as list.' )
        return 'fail'

    expected_xml = """<?xml version="2.0"?>
<TestXML>Value</TestXML>
"""

    if xml_md[0] != expected_xml:
        gdaltest.post_reason( 'xml does not match' )
        print(xml_md)
        return 'fail'

    if band.GetNoDataValue() != 100:
        gdaltest.post_reason( 'nodata not saved via pam' )
        return 'fail'
    
    return 'success' 

###############################################################################
# Check that PAM binary encoded nodata values work properly.
#
def pam_4():

    # Copy test dataset to tmp directory so that the .aux.xml file
    # won't be rewritten with the statistics in the master dataset.
    shutil.copyfile( 'data/mfftest.hdr.aux.xml', 'tmp/mfftest.hdr.aux.xml' )
    shutil.copyfile( 'data/mfftest.hdr', 'tmp/mfftest.hdr' )
    shutil.copyfile( 'data/mfftest.r00', 'tmp/mfftest.r00' )

    ds = gdal.Open( 'tmp/mfftest.hdr' )
    stats = ds.GetRasterBand(1).GetStatistics(0,1)

    if stats[0] != 0 or stats[1] != 4:
        gdaltest.post_reason( 'Got wrong min/max, likely nodata not working?' )
        print(stats)
        return 'fail'

    return 'success'

###############################################################################
# Verify that .aux files that don't match the configuration of the
# dependent file are not utilized. (#2471)
#
def pam_5():

    ds = gdal.Open( 'data/sasha.tif' )

    try:
        filelist = ds.GetFileList()
    except:
        # old bindings?
        return 'skip'

    ds = None

    if len(filelist) != 1:
        print(filelist)

        gdaltest.post_reason( 'did not get expected file list.' )
        return 'fail'

    return 'success'

###############################################################################
# Verify we can read nodata values from .aux files (#2505)
#
def pam_6():

    ds = gdal.Open( 'data/f2r23.tif' )
    if ds.GetRasterBand(1).GetNoDataValue() != 0:
        gdaltest.post_reason( 'did not get expect .aux sourced nodata.' )
        return 'fail'
    ds = None

    return 'success'

###############################################################################
# Verify we can create overviews on PNG with PAM disabled (#3693)
#
def pam_7():

    gdal.SetConfigOption( 'GDAL_PAM_ENABLED', 'NO' )

    shutil.copyfile( 'data/stefan_full_rgba.png', 'tmp/stefan_full_rgba.png' )
    ds = gdal.Open('tmp/stefan_full_rgba.png')
    ds.BuildOverviews('NEAR', [2])
    ds = None

    ds = gdal.Open('tmp/stefan_full_rgba.png')
    ovr_count = ds.GetRasterBand(1).GetOverviewCount()
    ds = None

    os.remove( 'tmp/stefan_full_rgba.png' )
    os.remove( 'tmp/stefan_full_rgba.png.ovr' )

    if ovr_count != 1:
        return 'fail'

    return 'success'

###############################################################################
# Test that Band.SetDescription() goes through PAM (#3780)
#
def pam_8():

    gdal.SetConfigOption( 'GDAL_PAM_ENABLED', 'YES' )

    ds = gdal.GetDriverByName('GTiff').Create('/vsimem/pam_8.tif', 1, 1, 1)
    ds.GetRasterBand(1).SetDescription('foo')
    ds = None

    ds = gdal.Open('/vsimem/pam_8.tif')
    desc = ds.GetRasterBand(1).GetDescription()
    ds = None

    gdal.GetDriverByName('GTiff').Delete('/vsimem/pam_8.tif')

    if desc != 'foo':
        print(desc)
        return 'fail'

    return 'success'

###############################################################################
# Test that we can retrieve projection from xml:ESRI domain
#
def pam_9():

    ds = gdal.GetDriverByName('GTiff').Create('/vsimem/pam_9.tif', 1, 1, 1)
    ds = None

    f = gdal.VSIFOpenL('/vsimem/pam_9.tif.aux.xml', 'wb')
    content = """<PAMDataset>
  <Metadata domain="xml:ESRI" format="xml">
    <GeodataXform xsi:type="typens:IdentityXform" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xmlns:xs="http://www.w3.org/2001/XMLSchema" xmlns:typens="http://www.esri.com/schemas/ArcGIS/9.2">
      <SpatialReference xsi:type="typens:ProjectedCoordinateSystem">
        <WKT>PROJCS[&quot;NAD_1983_UTM_Zone_14N&quot;,GEOGCS[&quot;GCS_North_American_1983&quot;,DATUM[&quot;D_North_American_1983&quot;,SPHEROID[&quot;GRS_1980&quot;,6378137.0,298.257222101]],PRIMEM[&quot;Greenwich&quot;,0.0],UNIT[&quot;Degree&quot;,0.0174532925199433]],PROJECTION[&quot;Transverse_Mercator&quot;],PARAMETER[&quot;false_easting&quot;,500000.0],PARAMETER[&quot;false_northing&quot;,0.0],PARAMETER[&quot;central_meridian&quot;,-99.0],PARAMETER[&quot;scale_factor&quot;,0.9996],PARAMETER[&quot;latitude_of_origin&quot;,0.0],UNIT[&quot;Meter&quot;,1.0]]</WKT>
        <HighPrecision>true</HighPrecision>
      </SpatialReference>
    </GeodataXform>
  </Metadata>
</PAMDataset>"""
    gdal.VSIFWriteL(content, 1, len(content), f)
    gdal.VSIFCloseL(f)

    ds = gdal.Open('/vsimem/pam_9.tif')
    wkt = ds.GetProjectionRef()
    ds = None

    gdal.GetDriverByName('GTiff').Delete('/vsimem/pam_9.tif')

    expected_wkt = """PROJCS["NAD_1983_UTM_Zone_14N",GEOGCS["GCS_North_American_1983",DATUM["North_American_Datum_1983",SPHEROID["GRS_1980",6378137.0,298.257222101]],PRIMEM["Greenwich",0.0],UNIT["Degree",0.0174532925199433]],PROJECTION["Transverse_Mercator"],PARAMETER["false_easting",500000.0],PARAMETER["false_northing",0.0],PARAMETER["central_meridian",-99.0],PARAMETER["scale_factor",0.9996],PARAMETER["latitude_of_origin",0.0],UNIT["Meter",1.0]]"""

    if wkt != expected_wkt:
        print(wkt)
        return 'fail'

    return 'success'

###############################################################################
# Cleanup.

def pam_cleanup():
    gdaltest.clean_tmp()
    if gdaltest.pam_setting != 'NULL':
        gdal.SetConfigOption( 'GDAL_PAM_ENABLED', gdaltest.pam_setting )
    else:
        gdal.SetConfigOption( 'GDAL_PAM_ENABLED', None )
    return 'success'

gdaltest_list = [
    pam_1,
    pam_2,
    pam_3,
    pam_4,
    pam_5,
    pam_6,
    pam_7,
    pam_8,
    pam_9,
    pam_cleanup ]

if __name__ == '__main__':

    gdaltest.setup_run( 'pam' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

