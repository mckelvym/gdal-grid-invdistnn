#!/usr/bin/env python
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test GEOS integration in OGR - geometric operations.
# Author:   Frank Warmerdam <warmerdam@pobox.com>
# 
###############################################################################
# Copyright (c) 2004, Frank Warmerdam <warmerdam@pobox.com>
# 
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
# 
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
# 
# You should have received a copy of the GNU Library General Public
# License along with this library; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.
###############################################################################

import os
import sys

sys.path.append( '../pymod' )

import gdaltest
import ogrtest
import ogr

###############################################################################
# Establish whether we have GEOS support integrated, testing simple Union.

def ogr_geos_union():

    if not ogrtest.have_geos():
        return 'skip'

    pnt1 = ogr.CreateGeometryFromWkt( 'POINT(10 20)' )
    pnt2 = ogr.CreateGeometryFromWkt( 'POINT(30 20)' )

    result = pnt1.Union( pnt2 )

    pnt1.Destroy()
    pnt2.Destroy()
        
    if ogrtest.check_feature_geometry( result, 'MULTIPOINT (10 20,30 20)' ):
        return 'fail'

    result.Destroy()

    return 'success'

###############################################################################
# Test polygon intersection.

def ogr_geos_intersection():

    if not ogrtest.have_geos():
        return 'skip'

    g1 = ogr.CreateGeometryFromWkt( 'POLYGON((0 0, 10 10, 10 0, 0 0))' )
    g2 = ogr.CreateGeometryFromWkt( 'POLYGON((0 0, 0 10, 10 0, 0 0))' )

    result = g1.Intersection( g2 )

    g1.Destroy()
    g2.Destroy()

    if ogrtest.check_feature_geometry( result, 'POLYGON ((0 0,5 5,10 0,0 0))'):
        print 'Got: ', result.ExportToWkt()
        return 'fail'

    result.Destroy()

    return 'success'

###############################################################################
# Test polygon difference.

def ogr_geos_difference():

    if not ogrtest.have_geos():
        return 'skip'

    g1 = ogr.CreateGeometryFromWkt( 'POLYGON((0 0, 10 10, 10 0, 0 0))' )
    g2 = ogr.CreateGeometryFromWkt( 'POLYGON((0 0, 0 10, 10 0, 0 0))' )

    result = g1.Difference( g2 )

    g1.Destroy()
    g2.Destroy()

    if ogrtest.check_feature_geometry( result,
                                       'POLYGON ((5 5,10 10,10 0,5 5))'):
        print 'Got: ', result.ExportToWkt()
        return 'fail'

    result.Destroy()

    return 'success'

###############################################################################
# Test polygon symmetric difference.

def ogr_geos_symmetric_difference():

    if not ogrtest.have_geos():
        return 'skip'

    g1 = ogr.CreateGeometryFromWkt( 'POLYGON((0 0, 10 10, 10 0, 0 0))' )
    g2 = ogr.CreateGeometryFromWkt( 'POLYGON((0 0, 0 10, 10 0, 0 0))' )

    result = g1.SymmetricDifference( g2 )

    g1.Destroy()
    g2.Destroy()

    if ogrtest.check_feature_geometry( result,
           'MULTIPOLYGON (((5 5,0 0,0 10,5 5)),((5 5,10 10,10 0,5 5)))'):
        print 'Got: ', result.ExportToWkt()
        return 'fail'

    result.Destroy()

    return 'success'

###############################################################################
# Test Intersect().

def ogr_geos_intersect():

    if not ogrtest.have_geos():
        return 'skip'

    g1 = ogr.CreateGeometryFromWkt( 'LINESTRING(0 0, 10 10)' )
    g2 = ogr.CreateGeometryFromWkt( 'LINESTRING(10 0, 0 10)' )

    result = g1.Intersect( g2 )

    g1.Destroy()
    g2.Destroy()

    if result == 0:
        gdaltest.post_reason( 'wrong result (got false)' )
        return 'fail'

    g1 = ogr.CreateGeometryFromWkt( 'LINESTRING(0 0, 10 10)' )
    g2 = ogr.CreateGeometryFromWkt( 'POLYGON((20 20, 20 30, 30 20, 20 20))' )

    result = g1.Intersect( g2 )

    g1.Destroy()
    g2.Destroy()

    if result != 0:
        gdaltest.post_reason( 'wrong result (got true)' )
        return 'fail'

    return 'success'

###############################################################################
# Test disjoint().

def ogr_geos_disjoint():

    if not ogrtest.have_geos():
        return 'skip'

    g1 = ogr.CreateGeometryFromWkt( 'LINESTRING(0 0, 10 10)' )
    g2 = ogr.CreateGeometryFromWkt( 'LINESTRING(10 0, 0 10)' )

    result = g1.Disjoint( g2 )

    g1.Destroy()
    g2.Destroy()

    if result != 0:
        gdaltest.post_reason( 'wrong result (got true)' )
        return 'fail'

    g1 = ogr.CreateGeometryFromWkt( 'LINESTRING(0 0, 10 10)' )
    g2 = ogr.CreateGeometryFromWkt( 'POLYGON((20 20, 20 30, 30 20, 20 20))' )

    result = g1.Disjoint( g2 )

    g1.Destroy()
    g2.Destroy()

    if result == 0:
        gdaltest.post_reason( 'wrong result (got false)' )
        return 'fail'

    return 'success'

###############################################################################
# Test touches.

def ogr_geos_touches():

    if not ogrtest.have_geos():
        return 'skip'

    g1 = ogr.CreateGeometryFromWkt( 'LINESTRING(0 0, 10 10)' )
    g2 = ogr.CreateGeometryFromWkt( 'LINESTRING(0 0, 0 10)' )

    result = g1.Touches( g2 )

    g1.Destroy()
    g2.Destroy()

    if result == 0:
        gdaltest.post_reason( 'wrong result (got false)' )
        return 'fail'

    g1 = ogr.CreateGeometryFromWkt( 'LINESTRING(0 0, 10 10)' )
    g2 = ogr.CreateGeometryFromWkt( 'POLYGON((20 20, 20 30, 30 20, 20 20))' )

    result = g1.Touches( g2 )

    g1.Destroy()
    g2.Destroy()

    if result != 0:
        gdaltest.post_reason( 'wrong result (got true)' )
        return 'fail'

    return 'success'

###############################################################################
# Test crosses.

def ogr_geos_crosses():

    if not ogrtest.have_geos():
        return 'skip'

    g1 = ogr.CreateGeometryFromWkt( 'LINESTRING(0 0, 10 10)' )
    g2 = ogr.CreateGeometryFromWkt( 'LINESTRING(10 0, 0 10)' )

    result = g1.Crosses( g2 )

    g1.Destroy()
    g2.Destroy()

    if result == 0:
        gdaltest.post_reason( 'wrong result (got false)' )
        return 'fail'

    g1 = ogr.CreateGeometryFromWkt( 'LINESTRING(0 0, 10 10)' )
    g2 = ogr.CreateGeometryFromWkt( 'LINESTRING(0 0, 0 10)' )

    result = g1.Crosses( g2 )

    g1.Destroy()
    g2.Destroy()

    if result != 0:
        gdaltest.post_reason( 'wrong result (got true)' )
        return 'fail'

    return 'success'

###############################################################################

def ogr_geos_within():

    if not ogrtest.have_geos():
        return 'skip'

    g1 = ogr.CreateGeometryFromWkt( 'POLYGON((0 0, 10 10, 10 0, 0 0))' )
    g2 = ogr.CreateGeometryFromWkt( 'POLYGON((-90 -90, -90 90, 190 -90, -90 -90))')

    result = g1.Within( g2 )

    if result == 0:
        gdaltest.post_reason( 'wrong result (got false)' )
        return 'fail'

    result = g2.Within( g1 )

    g1.Destroy()
    g2.Destroy()

    if result != 0:
        gdaltest.post_reason( 'wrong result (got true)' )
        return 'fail'

    return 'success'

###############################################################################

def ogr_geos_contains():

    if not ogrtest.have_geos():
        return 'skip'

    g1 = ogr.CreateGeometryFromWkt( 'POLYGON((0 0, 10 10, 10 0, 0 0))' )
    g2 = ogr.CreateGeometryFromWkt( 'POLYGON((-90 -90, -90 90, 190 -90, -90 -90))')

    result = g2.Contains( g1 )

    if result == 0:
        gdaltest.post_reason( 'wrong result (got false)' )
        return 'fail'

    result = g1.Contains( g2 )

    g1.Destroy()
    g2.Destroy()

    if result != 0:
        gdaltest.post_reason( 'wrong result (got true)' )
        return 'fail'

    return 'success'

###############################################################################

def ogr_geos_overlaps():

    if not ogrtest.have_geos():
        return 'skip'

    g1 = ogr.CreateGeometryFromWkt( 'POLYGON((0 0, 10 10, 10 0, 0 0))' )
    g2 = ogr.CreateGeometryFromWkt( 'POLYGON((-90 -90, -90 90, 190 -90, -90 -90))')

    result = g2.Overlaps( g1 )

    if result != 0:
        gdaltest.post_reason( 'wrong result (got true)' )
        return 'fail'

    g1.Destroy()
    g2.Destroy()

    return 'success'

###############################################################################

def ogr_geos_centroid():

    if not ogrtest.have_geos():
        return 'skip'

    g1 = ogr.CreateGeometryFromWkt( 'POLYGON((0 0, 10 10, 10 0, 0 0))' )

    centroid = g1.Centroid()

    g1.Destroy()

    if ogrtest.check_feature_geometry( centroid,
                                       'POINT(6.666666667 3.333333333)') != 0:
        print 'Got: ', centroid.ExportToWkt()
        return 'fail'

    centroid.Destroy()

    return 'success'

gdaltest_list = [ 
    ogr_geos_union,
    ogr_geos_intersection,
    ogr_geos_difference,
    ogr_geos_symmetric_difference,
    ogr_geos_intersect,
    ogr_geos_disjoint,
    ogr_geos_touches,
    ogr_geos_crosses,
    ogr_geos_within,
    ogr_geos_contains,
    ogr_geos_overlaps,
    ogr_geos_centroid ]

if __name__ == '__main__':

    gdaltest.setup_run( 'ogr_geos' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

