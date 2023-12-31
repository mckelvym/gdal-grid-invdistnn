#!/usr/bin/env python
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test basic GDAL open
# Author:   Even Rouault <even dot rouault at mines dash paris dot org>
# 
###############################################################################
# Copyright (c) 2008, Even Rouault <even dot rouault at mines dash paris dot org>
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

# Nothing exciting here. Just trying to open non existing files,
# or empty names, or files that are not valid datasets...

def basic_test_1():
    gdal.PushErrorHandler( 'CPLQuietErrorHandler' )
    ds = gdal.Open('non_existing_ds', gdal.GA_ReadOnly)
    gdal.PopErrorHandler()
    if ds is None and gdal.GetLastErrorMsg() == '`non_existing_ds\' does not exist in the file system,\nand is not recognised as a supported dataset name.\n':
        return 'success'
    else:
        return 'fail'

def basic_test_2():
    gdal.PushErrorHandler( 'CPLQuietErrorHandler' )
    ds = gdal.Open('non_existing_ds', gdal.GA_Update)
    gdal.PopErrorHandler()
    if ds is None and gdal.GetLastErrorMsg() == '`non_existing_ds\' does not exist in the file system,\nand is not recognised as a supported dataset name.\n':
        return 'success'
    else:
        return 'fail'

def basic_test_3():
    gdal.PushErrorHandler( 'CPLQuietErrorHandler' )
    ds = gdal.Open('', gdal.GA_ReadOnly)
    gdal.PopErrorHandler()
    if ds is None and gdal.GetLastErrorMsg() == '`\' does not exist in the file system,\nand is not recognised as a supported dataset name.\n':
        return 'success'
    else:
        return 'fail'

def basic_test_4():
    gdal.PushErrorHandler( 'CPLQuietErrorHandler' )
    ds = gdal.Open('', gdal.GA_Update)
    gdal.PopErrorHandler()
    if ds is None and gdal.GetLastErrorMsg() == '`\' does not exist in the file system,\nand is not recognised as a supported dataset name.\n':
        return 'success'
    else:
        return 'fail'

def basic_test_5():
    gdal.PushErrorHandler( 'CPLQuietErrorHandler' )
    ds = gdal.Open('data/doctype.xml', gdal.GA_ReadOnly)
    gdal.PopErrorHandler()
    if ds is None and gdal.GetLastErrorMsg() == '`data/doctype.xml\' not recognised as a supported file format.\n':
        return 'success'
    else:
        return 'fail'

gdaltest_list = [ basic_test_1,
                  basic_test_2,
                  basic_test_3,
                  basic_test_4,
                  basic_test_5 ]


if __name__ == '__main__':

    gdaltest.setup_run( 'basic_test' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

