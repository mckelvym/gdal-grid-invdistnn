#!/usr/bin/env python
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  KML Driver testing.
# Author:   Matuesz Loskot <mateusz@loskot.net>
# 
###############################################################################
# Copyright (c) 2007, Matuesz Loskot <mateusz@loskot.net>
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

sys.path.append( '../pymod' )

import gdaltest
import ogrtest
import ogr
import osr
import gdal

###############################################################################
# Test basic open operation for KML datastore.
#
def ogr_kml_datastore():

    gdaltest.kml_ds = None

    try:
        gdaltest.kml_ds = ogr.Open( 'data/samples.kml' );
    except:
        gdaltest.kml_ds = None

    if gdaltest.kml_ds is None:
        gdaltest.have_kml = 0
    else:
        gdaltest.have_kml = 1

    if not gdaltest.have_kml:
        return 'skip'

    if gdaltest.kml_ds.GetLayerCount() != 6:
        gdaltest.post_reason( 'wrong number of layers' )
        return 'fail'

    return 'success'

###############################################################################
# Test reading attributes for first layer (point).
#
def ogr_kml_attributes_1():
    
    if not gdaltest.have_kml:
        return 'skip'

    if gdaltest.kml_ds is None:
        gdaltest.post_reason( 'kml_ds is none' )
        return 'faii'

    lyr = gdaltest.kml_ds.GetLayerByName('Placemarks')
    feat = lyr.GetNextFeature()

    if feat.GetField('Name') != 'Simple placemark':
        gdaltest.post_reason( 'Wrong name field value' )
        return 'fail'

    if feat.GetField('description')[:23] != 'Attached to the ground.':
        gdaltest.post_reason( 'Wrong description field value' )
        print 'got: ', feat.GetField('description')[:23]
        return 'fail'

    feat = lyr.GetNextFeature()
    if feat is None:
        gdaltest.post_reason( 'expected feature not found.' )
        return 'fail'

    if feat.GetField('Name') != 'Floating placemark':
        gdaltest.post_reason( 'Wrong name field value' )
        return 'fail'

    if feat.GetField('description')[:25] != 'Floats a defined distance':
        gdaltest.post_reason( 'Wrong description field value' )
        return 'fail'
    
    feat = lyr.GetNextFeature()
    if feat is None:
        gdaltest.post_reason( 'expected feature not found.' )
        return 'fail'

    if feat.GetField('Name') != 'Extruded placemark':
        gdaltest.post_reason( 'Wrong name field value' )
        return 'fail'

    if feat.GetField('description') != 'Tethered to the ground by a customizable \" tail \"':
        gdaltest.post_reason( 'Wrong description field value' )
        return 'fail'

    return 'success'

###############################################################################
# Test reading attributes for another layer (point).
#
def ogr_kml_attributes_2():
    
    if not gdaltest.have_kml:
        return 'skip'

    if gdaltest.kml_ds is None:
        gdaltest.post_reason( 'kml_ds is none' )
        return 'faii'

    lyr = gdaltest.kml_ds.GetLayerByName('Highlighted Icon')
    feat = lyr.GetNextFeature()

    if feat.GetField('Name') != 'Roll over this icon':
        gdaltest.post_reason( 'Wrong name field value' )
        return 'fail'

    if feat.GetField('description') != '':
        gdaltest.post_reason( 'Wrong description field value' )
        return 'fail'

    feat = lyr.GetNextFeature()
    if feat is not None:
        gdaltest.post_reason( 'unexpected feature found.' )
        return 'fail'

    return 'success'

###############################################################################
# Test reading attributes for another layer (linestring).
#
def ogr_kml_attributes_3():
    
    if not gdaltest.have_kml:
        return 'skip'

    if gdaltest.kml_ds is None:
        gdaltest.post_reason( 'kml_ds is none' )
        return 'faii'

    lyr = gdaltest.kml_ds.GetLayerByName('Paths')
    feat = lyr.GetNextFeature()

    if feat.GetField('Name') != 'Tessellated':
        gdaltest.post_reason( 'Wrong name field value' )
        return 'fail'

    if feat.GetField('description') != 'If the <tessellate> tag has a value of 1, the line will contour to the underlying terrain':
        gdaltest.post_reason( 'Wrong description field value' )
        return 'fail'

    feat = lyr.GetNextFeature()
    if feat is None:
        gdaltest.post_reason( 'expected feature not found.' )
        return 'fail'
    
    if feat.GetField('Name') != 'Untessellated':
        gdaltest.post_reason( 'Wrong name field value' )
        return 'fail'

    if feat.GetField('description') != 'If the <tessellate> tag has a value of 0, the line follow a simple straight-line path from point to point':
        gdaltest.post_reason( 'Wrong description field value' )
        return 'fail'
    
    feat = lyr.GetNextFeature()
    if feat is None:
        gdaltest.post_reason( 'expected feature not found.' )
        return 'fail'
 
    return 'success'

###############################################################################
# Test reading attributes for another layer (polygon).
#
def ogr_kml_attributes_4():
    
    if not gdaltest.have_kml:
        return 'skip'

    if gdaltest.kml_ds is None:
        gdaltest.post_reason( 'kml_ds is none' )
        return 'faii'

    lyr = gdaltest.kml_ds.GetLayerByName('Google Campus')
    feat = lyr.GetNextFeature()

    i = 40
    while feat is not None:
        name = 'Building %d' % i
        if feat.GetField('Name') != name:
            gdaltest.post_reason( 'Wrong name field value' )
            print 'Got: "%s"' % feat.GetField('name')
            return 'fail'

        if feat.GetField('description') != '':
            gdaltest.post_reason( 'Wrong description field value' )
            return 'fail'

        i = i + 1
        feat = lyr.GetNextFeature()

    return 'success'
 
###############################################################################
# Test reading of KML point geometry
#
def ogr_kml_point_read():
    
    if not gdaltest.have_kml:
        return 'skip'

    if gdaltest.kml_ds is None:
        gdaltest.post_reason( 'kml_ds is none' )
        return 'faii'

    lyr = gdaltest.kml_ds.GetLayerByName('Placemarks')
    lyr.ResetReading()
    feat = lyr.GetNextFeature()

    wkt = 'POINT(-122.0822035425683 37.42228990140251)'
    
    if ogrtest.check_feature_geometry( feat, wkt):
        return 'fail'

    feat = lyr.GetNextFeature()
    if feat is None:
        gdaltest.post_reason( 'expected feature not found.' )
        return 'fail'
    
    wkt = 'POINT(-122.084075 37.4220033612141 50)'
    
    if ogrtest.check_feature_geometry( feat, wkt):
        return 'fail'
    
    feat = lyr.GetNextFeature()
    if feat is None:
        gdaltest.post_reason( 'expected feature not found.' )
        return 'fail'
    
    wkt = 'POINT(-122.0857667006183 37.42156927867553 50)'
    
    if ogrtest.check_feature_geometry( feat, wkt):
        return 'fail'

    return 'success'

###############################################################################
# Test reading of KML linestring geometry
#
def ogr_kml_linestring_read():
    
    if not gdaltest.have_kml:
        return 'skip'

    if gdaltest.kml_ds is None:
        gdaltest.post_reason( 'kml_ds is none' )
        return 'faii'

    lyr = gdaltest.kml_ds.GetLayerByName('Paths')
    lyr.ResetReading()
    feat = lyr.GetNextFeature()

    wkt = 'LINESTRING (-112.081423783034495 36.106778704771372 0, -112.087026775269294 36.0905099328766 0)'
    if ogrtest.check_feature_geometry(feat, wkt):
        return 'fail'
    
    feat = lyr.GetNextFeature()
    if feat is None:
        gdaltest.post_reason( 'expected feature not found.' )
        return 'fail'
 
    wkt = 'LINESTRING (-112.080622229594994 36.106734600079953 0,-112.085242575314993 36.090495986124218 0)'
    if ogrtest.check_feature_geometry(feat, wkt):
        return 'fail'
    
    feat = lyr.GetNextFeature()
    if feat is None:
        gdaltest.post_reason( 'expected feature not found.' % fid )
        return 'fail'
 
    wkt = 'LINESTRING (-112.265654928602004 36.094476726025462 2357,-112.266038452823807 36.093426088386707 2357,-112.266813901345301 36.092510587768807 2357,-112.267782683444494 36.091898273579957 2357,-112.268855751095202 36.091313794118697 2357,-112.269481071721899 36.090367720752099 2357,-112.269526855561097 36.089321714872852 2357,-112.269014456727604 36.088509160604723 2357,-112.268152881533894 36.087538135979557 2357,-112.2670588176031 36.086826852625677 2357,-112.265737458732104 36.086463123013033 2357)'
    if ogrtest.check_feature_geometry(feat, wkt):
        return 'fail'
    
    return 'success'

###############################################################################
# Test reading of KML polygon geometry
#
def ogr_kml_polygon_read():
    
    if not gdaltest.have_kml:
        return 'skip'

    if gdaltest.kml_ds is None:
        gdaltest.post_reason( 'kml_ds is none' )
        return 'faii'

    lyr = gdaltest.kml_ds.GetLayerByName('Google Campus')
    lyr.ResetReading()
    feat = lyr.GetNextFeature()

    wkt = 'POLYGON ((-122.084893845961204 37.422571240447859 17,-122.084958097919795 37.422119226268563 17,-122.084746957304702 37.42207183952619 17,-122.084572538096197 37.422090067296757 17,-122.084595488672306 37.422159327008949 17,-122.0838521118269 37.422272785643713 17,-122.083792243334997 37.422035391120843 17,-122.0835076656616 37.422090069571063 17,-122.083470946415204 37.422009873951609 17,-122.083122108574798 37.422104649494599 17,-122.082924737457205 37.422265039903863 17,-122.082933916938501 37.422312428430942 17,-122.083383735973698 37.422250460876178 17,-122.083360785424802 37.422341592287452 17,-122.083420455164202 37.42237075460644 17,-122.083659133885007 37.422512920110009 17,-122.083975843895203 37.422658730937812 17,-122.084237474333094 37.422651439725207 17,-122.0845036949503 37.422651438643499 17,-122.0848020460801 37.422611339163147 17,-122.084788275051494 37.422563950551208 17,-122.084893845961204 37.422571240447859 17))'
    if ogrtest.check_feature_geometry( feat, wkt):
        return 'fail'
    
    feat = lyr.GetNextFeature()
    if feat is None:
        gdaltest.post_reason( 'expected feature not found.' % fid )
        return 'fail'
 
    wkt = 'POLYGON ((-122.085741277148301 37.422270331552568 17,-122.085816976848093 37.422314088323461 17,-122.085852582875006 37.422303374697442 17,-122.085879994563896 37.422256861387893 17,-122.085886010140896 37.422231107613797 17,-122.085806915728796 37.422202501738553 17,-122.085837954265301 37.42214027058678 17,-122.085673264051906 37.422086902144081 17,-122.085602292640701 37.42214885429042 17,-122.085590277843593 37.422128290487002 17,-122.085584167223701 37.422081719672462 17,-122.085485206574106 37.42210455874995 17,-122.085506726435199 37.422142679498243 17,-122.085443071291493 37.422127838461719 17,-122.085099071490404 37.42251282407603 17,-122.085676981863202 37.422818153236513 17,-122.086016227378295 37.422449188587223 17,-122.085726032700407 37.422292396042529 17,-122.085741277148301 37.422270331552568 17))'
    if ogrtest.check_feature_geometry(feat, wkt):
        return 'fail'
    
    feat = lyr.GetNextFeature()
    if feat is None:
        gdaltest.post_reason( 'expected feature not found.' % fid )
        return 'fail'
 
    wkt = 'POLYGON ((-122.085786228724203 37.421362088869692 25,-122.085731299060299 37.421369359894811 25,-122.085731299291794 37.421409349109027 25,-122.085607707367899 37.421383901665649 25,-122.085580242651602 37.42137299550869 25,-122.085218622197104 37.421372995043157 25,-122.085227776563897 37.421616565082651 25,-122.085259818934702 37.421605658944031 25,-122.085259818549901 37.421682001560001 25,-122.085236931147804 37.421700178603459 25,-122.085264395782801 37.421761979825753 25,-122.085323903274599 37.421761980139067 25,-122.085355945432397 37.421852864451999 25,-122.085410875246296 37.421889218237339 25,-122.085479537935697 37.42189285337048 25,-122.085543622981902 37.421889217975462 25,-122.085626017804202 37.421860134999257 25,-122.085937287963006 37.421860134536047 25,-122.085942871866607 37.42160898590042 25,-122.085965546986102 37.421579927591438 25,-122.085864046234093 37.421471150029568 25,-122.0858548911215 37.421405713261841 25,-122.085809116276806 37.4214057134039 25,-122.085786228724203 37.421362088869692 25))'
    if ogrtest.check_feature_geometry(feat, wkt):
        return 'fail'
    
    feat = lyr.GetNextFeature()
    if feat is None:
        gdaltest.post_reason( 'expected feature not found.' % fid )
        return 'fail'
 
    wkt = 'POLYGON ((-122.084437112828397 37.421772530030907 19,-122.084511885574599 37.421911115428962 19,-122.0850470999805 37.421787551215353 19,-122.085071991339106 37.421436630231611 19,-122.084916406231997 37.421372378221157 19,-122.084219386816699 37.421372378016258 19,-122.084219386589993 37.421476171614962 19,-122.083808641999099 37.4214613409357 19,-122.083789972856394 37.421313064107963 19,-122.083279653469802 37.421293288405927 19,-122.083260981920702 37.421392139442979 19,-122.082937362173695 37.421372363998763 19,-122.082906242566693 37.421515697788713 19,-122.082850226966499 37.421762825764652 19,-122.082943578863507 37.421767769696352 19,-122.083217411188002 37.421792485526858 19,-122.0835970430103 37.421748007445601 19,-122.083945555677104 37.421693642376027 19,-122.084007789463698 37.421762838158529 19,-122.084113587521003 37.421748011043917 19,-122.084076247378405 37.421713412923751 19,-122.084144704773905 37.421678815345693 19,-122.084144704222993 37.421817206601972 19,-122.084250333307395 37.421817070044597 19,-122.084437112828397 37.421772530030907 19))'
    if ogrtest.check_feature_geometry(feat, wkt):
        return 'fail'
 
    return 'success'

###############################################################################
#  Cleanup

def ogr_kml_cleanup():
    if not gdaltest.have_kml:
        return 'skip'

    if gdaltest.kml_ds is not None:
        gdaltest.kml_ds.Destroy()

    return 'success'

###############################################################################
# Build tests runner

gdaltest_list = [ 
    ogr_kml_datastore,
    ogr_kml_attributes_1,
    ogr_kml_attributes_2,
    ogr_kml_attributes_3,
    ogr_kml_attributes_4,
    ogr_kml_point_read,
    ogr_kml_linestring_read,
    ogr_kml_polygon_read,
    ogr_kml_cleanup ]

if __name__ == '__main__':

    gdaltest.setup_run( 'ogr_kml' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

