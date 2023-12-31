#!/usr/bin/env python
###############################################################################
# $Id: pam_md.py 11065 2007-03-24 09:35:32Z mloskot $
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test RFC 15 "mask band" default functionality (nodata/alpha/etc)
# Author:   Frank Warmerdam <warmerdam@pobox.com>
# 
###############################################################################
# Copyright (c) 2007, Frank Warmerdam <warmerdam@pobox.com>
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

sys.path.append( '../pymod' )

import gdaltest
import gdal
import gdalconst

###############################################################################
# Verify the checksum and flags for "all valid" case.

def mask_1():

    try:
        x = gdalconst.GMF_ALL_VALID
        gdaltest.have_ng = 1
    except:
        gdaltest.have_ng = 0
        return 'skip'

    ds = gdal.Open('data/byte.tif')
    band = ds.GetRasterBand(1)

    if band.GetMaskFlags() != gdal.GMF_ALL_VALID:
        gdaltest.post_reason( 'Did not get expected mask.' )
        return 'fail'

    cs = band.GetMaskBand().Checksum()
    if cs != 4873:
        gdaltest.post_reason( 'Got wrong mask checksum' )
        print cs
        return 'fail'
    
    return 'success' 

###############################################################################
# Verify the checksum and flags for "nodata" case.

def mask_2():

    if gdaltest.have_ng == 0:
        return 'skip'

    ds = gdal.Open('data/byte.vrt')
    band = ds.GetRasterBand(1)

    if band.GetMaskFlags() != gdal.GMF_NODATA:
        gdaltest.post_reason( 'Did not get expected mask.' )
        return 'fail'

    cs = band.GetMaskBand().Checksum()
    if cs != 4209:
        gdaltest.post_reason( 'Got wrong mask checksum' )
        print cs
        return 'fail'
    
    return 'success' 

###############################################################################
# Verify the checksum and flags for "alpha" case.

def mask_3():

    if gdaltest.have_ng == 0:
        return 'skip'

    ds = gdal.Open('data/stefan_full_rgba.png')

    # Test first mask.
    
    band = ds.GetRasterBand(1)

    if band.GetMaskFlags() != gdal.GMF_ALPHA + gdal.GMF_PER_DATASET:
        gdaltest.post_reason( 'Did not get expected mask.' )
        return 'fail'

    cs = band.GetMaskBand().Checksum()
    if cs != 10807:
        gdaltest.post_reason( 'Got wrong mask checksum' )
        print cs
        return 'fail'

    # Verify second and third same as first.

    band_2 = ds.GetRasterBand(2)
    band_3 = ds.GetRasterBand(3)
    
    if band_2.GetMaskFlags() != band.GetMaskFlags() \
       or band_3.GetMaskFlags() != band.GetMaskFlags() \
       or str(band_2.GetMaskBand()) != str(band.GetMaskBand()) \
       or str(band_3.GetMaskBand()) != str(band.GetMaskBand()):
        gdaltest.post_reason( 'Band 2 or 3 does not seem to match first mask')
        return 'fail'

    # Verify alpha has no mask.
    band = ds.GetRasterBand(4)
    if band.GetMaskFlags() != gdal.GMF_ALL_VALID:
        gdaltest.post_reason( 'Did not get expected mask for alpha.' )
        return 'fail'

    cs = band.GetMaskBand().Checksum()
    if cs != 36074:
        gdaltest.post_reason( 'Got wrong alpha mask checksum' )
        print cs
        return 'fail'
    
    return 'success' 

###############################################################################
# Copy a *real* masked dataset, and confirm masks copied properly.

def mask_4():

    if gdaltest.have_ng == 0:
        return 'skip'

    src_ds = gdal.Open('../gdrivers/data/masked.jpg')

    # NOTE: for now we copy to PNM since it does everything (overviews too)
    # externally. Should eventually test with gtiff, hfa.
    drv = gdal.GetDriverByName('PNM')
    ds = drv.CreateCopy( 'tmp/mask_4.pnm', src_ds )
    src_ds = None

    # confirm we got the custom mask on the copied dataset.
    if ds.GetRasterBand(1).GetMaskFlags() != 2:
        gdaltest.post_reason( 'did not get expected mask flags' )
        print ds.GetRasterBand(1).GetMaskFlags()
        return 'fail'

    msk = ds.GetRasterBand(1).GetMaskBand()
    cs = msk.Checksum()
    expected_cs = 770
    
    if cs != expected_cs:
        gdaltest.post_reason( 'Did not get expected checksum' )
        print cs
        return 'fail'

    msk = None
    ds = None

    return 'success'

###############################################################################
# Create overviews for masked file, and verify the overviews have proper
# masks built for them.

def mask_5():

    if gdaltest.have_ng == 0:
        return 'skip'

    ds = gdal.Open('tmp/mask_4.pnm',gdal.GA_Update)
    ds.BuildOverviews( overviewlist = [2,4] )

    # confirm mask flags on overview.
    ovr = ds.GetRasterBand(1).GetOverview(1)
    
    if ovr.GetMaskFlags() != 2:
        gdaltest.post_reason( 'did not get expected mask flags' )
        print ovr.GetMaskFlags()
        return 'fail'

    msk = ovr.GetMaskBand()
    cs = msk.Checksum()
    expected_cs = 20505
    
    if cs != expected_cs:
        gdaltest.post_reason( 'Did not get expected checksum' )
        print cs
        return 'fail'
    ovr = None
    msk = None
    ds = None

    # Reopen and confirm we still get same results.
    ds = gdal.Open('tmp/mask_4.pnm')
    
    # confirm mask flags on overview.
    ovr = ds.GetRasterBand(1).GetOverview(1)
    
    if ovr.GetMaskFlags() != 2:
        gdaltest.post_reason( 'did not get expected mask flags' )
        print ovr.GetMaskFlags()
        return 'fail'

    msk = ovr.GetMaskBand()
    cs = msk.Checksum()
    expected_cs = 20505
    
    if cs != expected_cs:
        gdaltest.post_reason( 'Did not get expected checksum' )
        print cs
        return 'fail'

    ovr = None
    msk = None
    ds = None

    gdal.GetDriverByName('PNM').Delete( 'tmp/mask_4.pnm' )

    return 'success'

###############################################################################
# Cleanup.


gdaltest_list = [
    mask_1,
    mask_2,
    mask_3,
    mask_4,
    mask_5 ]

if __name__ == '__main__':

    gdaltest.setup_run( 'mask' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

