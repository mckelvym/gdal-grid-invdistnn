#!/usr/bin/env python
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test Misc. OGRGeometry operations.
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
# Test Area calculation for a MultiPolygon (which excersises lower level
# get_Area() methods as well).

def ogr_geom_area():

    geom_wkt = 'MULTIPOLYGON( ((0 0,1 1,1 0,0 0)),((0 0,10 0, 10 10, 0 10),(1 1,1 2,2 2,2 1)) )'
    geom = ogr.CreateGeometryFromWkt( geom_wkt )

    area = geom.GetArea()
    if abs(area-99.5) > 0.00000000001:
        gdaltest.post_reason( 'Area result wrong, got %g.' % area )
        return 'fail'

    geom.Destroy()
    
    return 'success'

###############################################################################
# Test Area calculation for a LinearRing (which excersises special case of
# getGeometryType value).

def ogr_geom_area_linearring():

    geom = ogr.Geometry( type = ogr.wkbLinearRing )
    geom.AddPoint_2D( 0, 0)
    geom.AddPoint_2D( 10, 0)
    geom.AddPoint_2D( 10, 10)
    geom.AddPoint_2D( 0, 10)
    geom.AddPoint_2D( 0, 0)

    area = geom.GetArea()
    if abs(area - 100.0) > 0.00000000001:
        gdaltest.post_reason( 'Area result wrong, got %g.' % area )
        return 'fail'

    geom.Destroy()
    
    return 'success'

def ogr_geom_empty():
    try:
        ogr.Geometry.IsEmpty
    except:
        return 'skip'

    # We know this fails for POINT EMPTY.  Add test for points when this
    # is fixed.
    
    geom_wkt = 'LINESTRING EMPTY'
    geom = ogr.CreateGeometryFromWkt(geom_wkt)

    if (geom.IsEmpty() == False):
        geom.Destroy()
        gdaltest.post_reason ("IsEmpty returning false for an empty geometry")
        return 'fail'
    geom.Destroy()
    
    geom_wkt = 'POINT( 1 2 )'

    geom = ogr.CreateGeometryFromWkt(geom_wkt)
    if not geom:
        gdaltest.post_reason ("A geometry could not be created from wkt: %s"%wkt)
        return 'fail'

    if (geom.IsEmpty() == True):
        gdaltest.post_reason ("IsEmpty returning true for a non-empty geometry")
        return 'fail'
    geom.Destroy()
    return 'success'

def ogr_geom_pickle():
    try:
        ogr.Geometry.IsEmpty  #IsEmpty is only in the ng bindings
    except:
        return 'skip'
    
    import pickle
    geom_wkt = 'MULTIPOLYGON( ((0 0,1 1,1 0,0 0)),((0 0,10 0, 10 10, 0 10),(1 1,1 2,2 2,2 1)) )'
    geom = ogr.CreateGeometryFromWkt(geom_wkt)
    p = pickle.dumps(geom)
    
    g = pickle.loads(p)
    
    if not geom.Equal(g):
        gdaltest.post_reason ("pickled geometries were not equal")
        return 'fail'
    geom.Destroy()
    return 'success'

###############################################################################
# Test OGRGeometry::getBoundary() result for point.

def ogr_geom_boundary_point():
    
    geom_wkt = 'POINT(1 1)'
    geom = ogr.CreateGeometryFromWkt(geom_wkt)
    tmp = ogr.CreateGeometryFromWkt(geom_wkt)

    # Detect GEOS support
    try:
        result = geom.Union(tmp)
    except:
        result = None

    tmp.Destroy()

    if result is None:
        gdaltest.have_geos = 0
        return 'skip'

    gdaltest.have_geos = 1

    bnd = geom.GetBoundary()
    if bnd.GetGeometryType() is not ogr.wkbGeometryCollection:
        gdaltest.post_reason( 'Boundary not reported as GEOMETRYCOLLECTION EMPTY' )
        return 'fail'

    bnd.Destroy()
    geom.Destroy()
    
    return 'success'

###############################################################################
# Test OGRGeometry::getBoundary() result for multipoint.

def ogr_geom_boundary_multipoint():
    
    if gdaltest.have_geos == 0:
        return 'skip'

    geom_wkt = 'MULTIPOINT((0 0),(1 1))'
    geom = ogr.CreateGeometryFromWkt(geom_wkt)

    bnd = geom.GetBoundary()
    if bnd.GetGeometryType() is not ogr.wkbGeometryCollection:
        gdaltest.post_reason( 'Boundary not reported as GEOMETRYCOLLECTION EMPTY' )
        return 'fail'

    bnd.Destroy()
    geom.Destroy()
    
    return 'success'

###############################################################################
# Test OGRGeometry::getBoundary() result for linestring.

def ogr_geom_boundary_linestring():
    
    if gdaltest.have_geos == 0:
        return 'skip'

    geom_wkt = 'LINESTRING(0 0, 1 1, 2 2, 3 2, 4 2)'
    geom = ogr.CreateGeometryFromWkt(geom_wkt)

    bnd = geom.GetBoundary()
    if bnd.GetGeometryType() is not ogr.wkbMultiPoint:
        gdaltest.post_reason( 'Boundary not reported as MULTIPOINT' )
        return 'fail'

    if bnd.GetGeometryCount() != 2:
        gdaltest.post_reason( 'Boundary not reported as MULTIPOINT consisting of 2 points' )
        return 'fail'

    bnd.Destroy()
    geom.Destroy()
 
    geom_wkt = 'LINESTRING(0 0, 1 0, 1 1, 0 1, 0 0)'
    geom = ogr.CreateGeometryFromWkt(geom_wkt)

    bnd = geom.GetBoundary()
    if bnd.GetGeometryType() is not ogr.wkbMultiPoint:
        gdaltest.post_reason( 'Boundary not reported as MULTIPOINT' )
        return 'fail'

    if bnd.GetGeometryCount() != 0:
        gdaltest.post_reason( 'Boundary not reported as MULTIPOINT EMPTY' )
        return 'fail'

   
    bnd.Destroy()
    geom.Destroy()
 
    return 'success'

###############################################################################
# Test OGRGeometry::getBoundary() result for polygon.

def ogr_geom_boundary_polygon():
    
    if gdaltest.have_geos == 0:
        return 'skip'

    geom_wkt = 'POLYGON((0 0,1 1,1 0,0 0))'
    geom = ogr.CreateGeometryFromWkt(geom_wkt)

    bnd = geom.GetBoundary()
    if bnd.GetGeometryType() is not ogr.wkbLineString:
        gdaltest.post_reason( 'Boundary not reported as non-empty LINESTRING' )
        return 'fail'

    bnd.Destroy()
    geom.Destroy()
    
    return 'success'

###############################################################################
# cleanup

def ogr_geom_cleanup():
    return 'success'

gdaltest_list = [ 
    ogr_geom_area,
    ogr_geom_area_linearring,
    ogr_geom_empty,
    ogr_geom_pickle,
    ogr_geom_boundary_point,
    ogr_geom_boundary_multipoint,
    ogr_geom_boundary_linestring,
    ogr_geom_boundary_polygon,
    ogr_geom_cleanup ]

if __name__ == '__main__':

    gdaltest.setup_run( 'ogr_geom' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

