#!/usr/bin/env python
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test some ESRI specific translation issues.
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

sys.path.append( '../pymod' )

import gdaltest
import osr

###############################################################################
# This test verifies that morphToESRI() translates ideosyncratic datum names
# from "EPSG" form to ESRI from when the exception list comes from the
# gdal_datum.csv file. 

def osr_esri_1():
    
    srs = osr.SpatialReference()
    srs.ImportFromEPSG( 4202 )

    if srs.GetAttrValue( 'DATUM' ) != 'Australian_Geodetic_Datum_1966':
        gdaltest.post_reason( 'Got wrong DATUM name (%s) after EPSG import.' %\
                              srs.GetAttrValue( 'DATUM' ) )
        return 'fail'
    
    srs.MorphToESRI()

    if srs.GetAttrValue( 'DATUM' ) != 'D_Australian_1966':
        gdaltest.post_reason( 'Got wrong DATUM name (%s) after ESRI morph.' %\
                              srs.GetAttrValue( 'DATUM' ) )
        return 'fail'
    
    srs.MorphFromESRI()

    if srs.GetAttrValue( 'DATUM' ) != 'Australian_Geodetic_Datum_1966':
        gdaltest.post_reason( 'Got wrong DATUM name (%s) after ESRI unmorph.' %\
                              srs.GetAttrValue( 'DATUM' ) )
        return 'fail'
    
    return 'success'

###############################################################################
# Verify that exact correct form of UTM names is established when
# translating certain GEOGCSes to ESRI format.

def osr_esri_2():
    
    srs = osr.SpatialReference()
    srs.SetFromUserInput( '+proj=utm +zone=11 +south +datum=WGS84' )

    srs.MorphToESRI()
    
    if srs.GetAttrValue( 'GEOGCS' ) != 'GCS_WGS_1984':
        gdaltest.post_reason( 'Got wrong GEOGCS name (%s) after ESRI morph.' %\
                              srs.GetAttrValue( 'GEOGCS' ) )
        return 'fail'
    
    if srs.GetAttrValue( 'PROJCS' ) != 'WGS_1984_UTM_Zone_11S':
        gdaltest.post_reason( 'Got wrong PROJCS name (%s) after ESRI morph.' %\
                              srs.GetAttrValue( 'PROJCS' ) )
        return 'fail'
    
    return 'success'

###############################################################################
# Verify that Unnamed is changed to Unknown in morphToESRI().

def osr_esri_3():
    
    srs = osr.SpatialReference()
    srs.SetFromUserInput( '+proj=mill +datum=WGS84' )

    srs.MorphToESRI()
    
    if srs.GetAttrValue( 'PROJCS' ) != 'Miller_Cylindrical':
        gdaltest.post_reason( 'Got wrong PROJCS name (%s) after ESRI morph.' %\
                              srs.GetAttrValue( 'PROJCS' ) )
        return 'fail'
    
    return 'success'

###############################################################################
# Verify Polar Stereographic translations work properly OGR to ESRI.

def osr_esri_4():
    
    srs = osr.SpatialReference()
    srs.SetFromUserInput( 'PROJCS["PS Test",GEOGCS["WGS 84",DATUM["WGS_1984",SPHEROID["WGS 84",6378137,298.2572235629972]],PRIMEM["Greenwich",0],UNIT["degree",0.0174532925199433]],PROJECTION["Polar_Stereographic"],PARAMETER["latitude_of_origin",-80.2333],PARAMETER["central_meridian",171],PARAMETER["scale_factor",0.9999],PARAMETER["false_easting",0],PARAMETER["false_northing",0],UNIT["metre",1]]' )

    srs.MorphToESRI()
    
    if srs.GetAttrValue( 'PROJECTION' ) != 'Stereographic_South_Pole':
        gdaltest.post_reason( 'Got wrong PROJECTION name (%s) after ESRI morph.' %\
                              srs.GetAttrValue( 'PROJECTION' ) )
        return 'fail'
    
    if srs.GetProjParm('standard_parallel_1') != -80.2333:
        gdaltest.post_reason( 'Got wrong parameter value (%g) after ESRI morph.' %\
                              srs.GetProjParm('standard_parallel_1') )
        return 'fail'
    
    return 'success'

###############################################################################
# Verify Polar Stereographic translations work properly ESRI to OGR.

def osr_esri_5():
    
    srs = osr.SpatialReference()
    srs.SetFromUserInput( 'PROJCS["PS Test",GEOGCS["GCS_WGS_1984",DATUM["D_WGS_1984",SPHEROID["WGS_1984",6378137,298.2572235629972]],PRIMEM["Greenwich",0],UNIT["Degree",0.017453292519943295]],PROJECTION["Stereographic_South_Pole"],PARAMETER["standard_parallel_1",-80.2333],PARAMETER["central_meridian",171],PARAMETER["scale_factor",0.9999],PARAMETER["false_easting",0],PARAMETER["false_northing",0],UNIT["Meter",1]]' )

    srs.MorphFromESRI()
    
    if srs.GetAttrValue( 'PROJECTION' ) != 'Polar_Stereographic':
        gdaltest.post_reason( 'Got wrong PROJECTION name (%s) after ESRI morph.' %\
                              srs.GetAttrValue( 'PROJECTION' ) )
        return 'fail'
    
    if srs.GetProjParm('latitude_of_origin') != -80.2333:
        gdaltest.post_reason( 'Got wrong parameter value (%g) after ESRI morph.' %\
                              srs.GetProjParm('latitude_of_origin') )
        return 'fail'
    
    return 'success'

###############################################################################
# Verify Lambert 2SP with a 1.0 scale factor still gets translated to 2SP
# per bug 187. 

def osr_esri_6():
    
    srs = osr.SpatialReference()
    srs.SetFromUserInput( 'PROJCS["Texas Centric Mapping System/Lambert Conformal",GEOGCS["GCS_North_American_1983",DATUM["D_North_American_1983",SPHEROID["GRS_1980",6378137.0,298.257222101]],PRIMEM["Greenwich",0.0],UNIT["Degree",0.0174532925199433]],PROJECTION["Lambert_Conformal_Conic"],PARAMETER["False_Easting",1500000.0],PARAMETER["False_Northing",5000000.0],PARAMETER["Central_Meridian",-100.0],PARAMETER["Standard_Parallel_1",27.5],PARAMETER["Standard_Parallel_2",35.0],PARAMETER["Scale_Factor",1.0],PARAMETER["Latitude_Of_Origin",18.0],UNIT["Meter",1.0]]' )

    srs.MorphFromESRI()
    
    if srs.GetAttrValue( 'PROJECTION' ) != 'Lambert_Conformal_Conic_2SP':
        gdaltest.post_reason( \
            'Got wrong PROJECTION name (%s) after ESRI morph, expected 2SP' %\
            srs.GetAttrValue( 'PROJECTION' ) )
        return 'fail'
    
    return 'success'

###############################################################################
# Verify that FEET is treated as US survey feet per bug #1533.

def osr_esri_7():
    
    prj = [ 'Projection    STATEPLANE',
            'Fipszone      903',
            'Datum         NAD83',
            'Spheroid      GRS80',
            'Units         FEET',
            'Zunits        NO',
            'Xshift        0.0',
            'Yshift        0.0',
            'Parameters    ',
            '' ]

    srs_prj = osr.SpatialReference()
    srs_prj.ImportFromESRI( prj )
    
    wkt = """PROJCS["NAD83 / Florida North",
    GEOGCS["NAD83",
        DATUM["North_American_Datum_1983",
            SPHEROID["GRS 1980",6378137,298.257222101,
                AUTHORITY["EPSG","7019"]],
            AUTHORITY["EPSG","6269"]],
        PRIMEM["Greenwich",0,
            AUTHORITY["EPSG","8901"]],
        UNIT["degree",0.01745329251994328,
            AUTHORITY["EPSG","9122"]],
        AUTHORITY["EPSG","4269"]],
    PROJECTION["Lambert_Conformal_Conic_2SP"],
    PARAMETER["standard_parallel_1",30.75],
    PARAMETER["standard_parallel_2",29.58333333333333],
    PARAMETER["latitude_of_origin",29],
    PARAMETER["central_meridian",-84.5],
    PARAMETER["false_easting",1968500.062007752],
    PARAMETER["false_northing",0],
    UNIT["U.S. Foot",0.3048006],
    AUTHORITY["EPSG","26960"]]"""

    srs_wkt = osr.SpatialReference(wkt = wkt)

    if not srs_prj.IsSame( srs_wkt ):
        gdaltest.post_reason( 'old style ESRI projection imported wrong, perhaps linear units?' )
        return 'fail'

    return 'success'

###############################################################################
# Verify that handling of numerically specified units (see bug #1533)

def osr_esri_8():
    
    prj = [ 'Projection    STATEPLANE',
            'Fipszone      903',
            'Datum         NAD83',
            'Spheroid      GRS80',
            'Units         3.280839895013123',
            'Zunits        NO',
            'Xshift        0.0',
            'Yshift        0.0',
            'Parameters    ',
            '' ]

    srs_prj = osr.SpatialReference()
    srs_prj.ImportFromESRI( prj )
    
    wkt = """PROJCS["NAD83 / Florida North",
    GEOGCS["NAD83",
        DATUM["North_American_Datum_1983",
            SPHEROID["GRS 1980",6378137,298.257222101,
                AUTHORITY["EPSG","7019"]],
            AUTHORITY["EPSG","6269"]],
        PRIMEM["Greenwich",0,
            AUTHORITY["EPSG","8901"]],
        UNIT["degree",0.01745329251994328,
            AUTHORITY["EPSG","9122"]],
        AUTHORITY["EPSG","4269"]],
    PROJECTION["Lambert_Conformal_Conic_2SP"],
    PARAMETER["standard_parallel_1",30.75],
    PARAMETER["standard_parallel_2",29.58333333333333],
    PARAMETER["latitude_of_origin",29],
    PARAMETER["central_meridian",-84.5],
    PARAMETER["false_easting",1968503.937007874],
    PARAMETER["false_northing",0],
    UNIT["user-defined",0.3048],
    AUTHORITY["EPSG","26960"]]"""

    srs_wkt = osr.SpatialReference(wkt = wkt)

    if not srs_prj.IsSame( srs_wkt ):
        gdaltest.post_reason( 'old style ESRI projection imported wrong, perhaps linear units?' )
        return 'fail'

    return 'success'

###############################################################################
# Verify Equidistant Conic handling.

def osr_esri_9():
    
    srs = osr.SpatialReference()
    srs.SetFromUserInput( 'PROJCS["edc",GEOGCS["GCS_North_American_1983",DATUM["D_North_American_1983",SPHEROID["GRS_1980",6378137.0,298.257222101]],PRIMEM["Greenwich",0.0],UNIT["Degree",0.0174532925199433]],PROJECTION["Equidistant_Conic"],PARAMETER["False_Easting",0.0],PARAMETER["False_Northing",0.0],PARAMETER["Central_Meridian",-96.0],PARAMETER["Standard_Parallel_1",29.5],PARAMETER["Standard_Parallel_2",45.5],PARAMETER["Latitude_Of_Origin",37.5],UNIT["Meter",1.0]]' )

    expected = 'PROJCS["edc",GEOGCS["GCS_North_American_1983",DATUM["North_American_Datum_1983",SPHEROID["GRS_1980",6378137.0,298.257222101]],PRIMEM["Greenwich",0.0],UNIT["Degree",0.0174532925199433]],PROJECTION["Equidistant_Conic"],PARAMETER["False_Easting",0.0],PARAMETER["False_Northing",0.0],PARAMETER["longitude_of_center",-96.0],PARAMETER["Standard_Parallel_1",29.5],PARAMETER["Standard_Parallel_2",45.5],PARAMETER["latitude_of_center",37.5],UNIT["Meter",1.0]]'
    
    srs.MorphFromESRI()
    wkt = srs.ExportToWkt()
    if wkt != expected:
        print
        print 'Got:      ', wkt
        print 'Expected: ', expected
        gdaltest.post_reason( 'Did not get expected Equidistant Conic SRS after morphFromESRI' )
        return 'fail'

    expected = 'PROJCS["edc",GEOGCS["GCS_North_American_1983",DATUM["D_North_American_1983",SPHEROID["GRS_1980",6378137.0,298.257222101]],PRIMEM["Greenwich",0.0],UNIT["Degree",0.017453292519943295]],PROJECTION["Equidistant_Conic"],PARAMETER["False_Easting",0.0],PARAMETER["False_Northing",0.0],PARAMETER["central_meridian",-96.0],PARAMETER["Standard_Parallel_1",29.5],PARAMETER["Standard_Parallel_2",45.5],PARAMETER["latitude_of_origin",37.5],UNIT["Meter",1.0]]'
    
    srs.MorphToESRI()
    wkt = srs.ExportToWkt()
    if wkt != expected:
        print
        print 'Got:      ', wkt
        print 'Expected: ', expected
        gdaltest.post_reason( 'Did not get expected Equidistant Conic SRS after morphToESRI' )
        return 'fail'

    return 'success'

###############################################################################
# Verify Plate_Carree handling.

def osr_esri_10():
    
    srs = osr.SpatialReference()
    srs.SetFromUserInput( 'PROJCS["Sphere_Plate_Carree",GEOGCS["GCS_Sphere",DATUM["D_Sphere",SPHEROID["Sphere",6371000.0,0.0]],PRIMEM["Greenwich",0.0],UNIT["Degree",0.0174532925199433]],PROJECTION["Plate_Carree"],PARAMETER["False_Easting",0.0],PARAMETER["False_Northing",0.0],PARAMETER["Central_Meridian",0.0],UNIT["Meter",1.0]]' )

    expected = 'PROJCS["Sphere_Plate_Carree",GEOGCS["GCS_Sphere",DATUM["Not_specified_based_on_Authalic_Sphere",SPHEROID["Sphere",6371000.0,0.0]],PRIMEM["Greenwich",0.0],UNIT["Degree",0.0174532925199433]],PROJECTION["Equirectangular"],PARAMETER["False_Easting",0.0],PARAMETER["False_Northing",0.0],PARAMETER["Central_Meridian",0.0],UNIT["Meter",1.0]]'
    
    srs.MorphFromESRI()
    wkt = srs.ExportToWkt()
    if wkt != expected:
        print
        print 'Got:      ', wkt
        print 'Expected: ', expected
        gdaltest.post_reason( 'Did not get expected Equirectangular SRS after morphFromESRI' )
        return 'fail'

    expected = 'PROJCS["Sphere_Plate_Carree",GEOGCS["GCS_Sphere",DATUM["D_Sphere",SPHEROID["Sphere",6371000.0,0.0]],PRIMEM["Greenwich",0.0],UNIT["Degree",0.017453292519943295]],PROJECTION["Plate_Carree"],PARAMETER["False_Easting",0.0],PARAMETER["False_Northing",0.0],PARAMETER["Central_Meridian",0.0],UNIT["Meter",1.0]]'
    
    srs.MorphToESRI()
    wkt = srs.ExportToWkt()
    if wkt != expected:
        print
        print 'Got:      ', wkt
        print 'Expected: ', expected
        gdaltest.post_reason( 'Did not get expected Plate_Caree SRS after morphToESRI' )
        return 'fail'

    return 'success'

###############################################################################
# Verify arc/info style TM handling.

def osr_esri_11():
    
    srs = osr.SpatialReference()
    srs.ImportFromESRI( [ 'Projection    TRANSVERSE',
                          'Datum         NAD27',
                          'Spheroid      CLARKE1866',
                          'Units         METERS',
                          'Zunits        NO',
                          'Xshift        0.0',
                          'Yshift        0.0',
                          'Parameters   ',
                          '1.0 /* scale factor at central meridian',
                          '-106 56  0.5 /* longitude of central meridian',
                          '  39 33 30 /* latitude of origin',
                          '0.0 /* false easting (meters)',
                          '0.0 /* false northing (meters)' ] )
    
    expected = 'PROJCS["unnamed",GEOGCS["NAD27",DATUM["North_American_Datum_1927",SPHEROID["Clarke 1866",6378206.4,294.978698213898,AUTHORITY["EPSG","7008"]],TOWGS84[-3,142,183,0,0,0,0],AUTHORITY["EPSG","6267"]],PRIMEM["Greenwich",0,AUTHORITY["EPSG","8901"]],UNIT["degree",0.0174532925199433,AUTHORITY["EPSG","9108"]],AXIS["Lat",NORTH],AXIS["Long",EAST],AUTHORITY["EPSG","4267"]],PROJECTION["Transverse_Mercator"],PARAMETER["latitude_of_origin",39.55833333333333],PARAMETER["central_meridian",-106.9334722222222],PARAMETER["scale_factor",1],PARAMETER["false_easting",0],PARAMETER["false_northing",0],UNIT["METERS",1]]'
    
    srs.MorphFromESRI()
    wkt = srs.ExportToWkt()
    if wkt != expected:
        print
        print 'Got:      ', wkt
        print 'Expected: ', expected
        gdaltest.post_reason( 'Did not get expected TM SRS after morphFromESRI' )
        return 'fail'

    return 'success'

###############################################################################
# Test automatic morphing of ESRI-style LCC WKT prefixed with 'ESRI::'

def osr_esri_12():
    
    srs = osr.SpatialReference()
    srs.SetFromUserInput( 'ESRI::PROJCS["Lambert Conformal Conic",GEOGCS["grs80",DATUM["D_North_American_1983",SPHEROID["Geodetic_Reference_System_1980",6378137,298.257222101]],PRIMEM["Greenwich",0],UNIT["Degree",0.017453292519943295]],PROJECTION["Lambert_Conformal_Conic"],PARAMETER["standard_parallel_1",34.33333333333334],PARAMETER["standard_parallel_2",36.16666666666666],PARAMETER["latitude_of_origin",33.75],PARAMETER["central_meridian",-79],PARAMETER["false_easting",609601.22],PARAMETER["false_northing",0],UNIT["Meter",1]]' )

    # No MorphFromESRI() is required

    if srs.GetAttrValue( 'PROJECTION' ) != 'Lambert_Conformal_Conic_2SP':
        gdaltest.post_reason( 'Got wrong PROJECTION name (%s) after ESRI morph.' % \
                              srs.GetAttrValue( 'PROJECTION' ) )
        return 'fail'
    
    if abs( srs.GetProjParm('standard_parallel_1') - 34.333333333 ) > 0.00001:
        gdaltest.post_reason( 'Got wrong parameter value (%g) after ESRI morph.' % \
                              srs.GetProjParm('standard_parallel_1') )
        return 'fail'
    
    if srs.GetAttrValue( 'DATUM' ) != 'North_American_Datum_1983':
        gdaltest.post_reason( 'Got wrong DATUM name (%s) after ESRI morph.' % \
                              srs.GetAttrValue( 'DATUM' ) )

    if srs.GetAttrValue( 'UNIT' ) != 'Meter':
        gdaltest.post_reason( 'Got wrong UNIT name (%s) after ESRI morph.' % \
                              srs.GetAttrValue( 'UNIT' ) )
        return 'fail'
    
    return 'success'

###############################################################################
# Test automatic morphing of ESRI-style LCC WKT prefixed with 'ESRI::'
# but read directly from file.

def osr_esri_13():
    
    srs = osr.SpatialReference()
    srs.SetFromUserInput( 'data/lcc_esri.prj' )

    # No MorphFromESRI() is required

    if srs.GetAttrValue( 'PROJECTION' ) != 'Lambert_Conformal_Conic_2SP':
        gdaltest.post_reason( 'Got wrong PROJECTION name (%s) after ESRI morph.' % \
                              srs.GetAttrValue( 'PROJECTION' ) )
        return 'fail'
    
    if abs( srs.GetProjParm('standard_parallel_1') - 34.333333333 ) > 0.00001:
        gdaltest.post_reason( 'Got wrong parameter value (%g) after ESRI morph.' % \
                              srs.GetProjParm('standard_parallel_1') )
        return 'fail'
    
    if srs.GetAttrValue( 'DATUM' ) != 'North_American_Datum_1983':
        gdaltest.post_reason( 'Got wrong DATUM name (%s) after ESRI morph.' % \
                              srs.GetAttrValue( 'DATUM' ) )

    if srs.GetAttrValue( 'UNIT' ) != 'Meter':
        gdaltest.post_reason( 'Got wrong UNIT name (%s) after ESRI morph.' % \
                              srs.GetAttrValue( 'UNIT' ) )
        return 'fail'
    
    return 'success'


###############################################################################
# Verify that state plane epsg authority values are not applied if the
# linear units are changed for old style .prj files (bug #1697)

def osr_esri_14():
    
    srs = osr.SpatialReference()
    srs.ImportFromESRI( [ 'PROJECTION STATEPLANE',
                          'UNITS feet',
                          'FIPSZONE 2600',
                          'DATUM NAD83',
                          'PARAMETERS' ] )
    if srs.GetAuthorityCode( 'PROJCS' ) != None:
        print srs.GetAuthorityCode( 'PROJCS' )
        gdaltest.post_reason( 'Get epsg authority code inappropriately.' )
        return 'fail'
    
    srs = osr.SpatialReference()
    srs.ImportFromESRI( [ 'PROJECTION STATEPLANE',
                          'UNITS meter',
                          'FIPSZONE 2600',
                          'DATUM NAD83',
                          'PARAMETERS' ] )
    if srs.GetAuthorityCode( 'PROJCS' ) != '32104':
        print srs.GetAuthorityCode( 'PROJCS' )
        gdaltest.post_reason( 'Did not get epsg authority code when expected.')
        return 'fail'

    return 'success'


###############################################################################

gdaltest_list = [ 
    osr_esri_1,
    osr_esri_2,
    osr_esri_3,
    osr_esri_4,
    osr_esri_5,
    osr_esri_6,
    osr_esri_7,
    osr_esri_8,
    osr_esri_9,
    osr_esri_10,
    osr_esri_11,
    osr_esri_12,
    osr_esri_13,
    osr_esri_14,
    None ]

if __name__ == '__main__':

    gdaltest.setup_run( 'osr_esri' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

