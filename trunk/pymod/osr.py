#******************************************************************************
#  $Id$
# 
#  Project:  OSR (OGRSpatialReference/CoordinateTransform) Python Interface
#  Purpose:  OSR Shadow Class Implementations
#  Author:   Frank Warmerdam, warmerdam@pobox.com
# 
#******************************************************************************
#  Copyright (c) 2000, Frank Warmerdam <warmerdam@pobox.com>
# 
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  the rights to use, copy, modify, merge, publish, distribute, sublicense,
#  and/or sell copies of the Software, and to permit persons to whom the
#  Software is furnished to do so, subject to the following conditions:
# 
#  The above copyright notice and this permission notice shall be included
#  in all copies or substantial portions of the Software.
# 
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#  DEALINGS IN THE SOFTWARE.
#******************************************************************************
# 
# $Log$
# Revision 1.21  2003/02/20 04:14:04  warmerda
# Updated email.
#
# Revision 1.20  2003/02/06 05:53:56  warmerda
# Blow an exception when the CoordinateTransformation constructor fails.
#
# Revision 1.19  2003/02/06 05:49:28  warmerda
# Translate 'NULL' into None for CoordinateTransformation class constructor.
#
# Revision 1.18  2003/02/06 04:50:57  warmerda
# added the Fixup() method on OGRSpatialReference
#
# Revision 1.17  2003/01/24 19:45:17  warmerda
# fixed handling of CloneGeogCS() failing
#
# Revision 1.16  2003/01/08 18:17:33  warmerda
# added FixupOrdering() and StripCTParms
#
# Revision 1.15  2002/11/30 20:53:51  warmerda
# added SetFromUserInput
#
# Revision 1.14  2002/11/25 16:11:39  warmerda
# added GetAuthorityCode/Name
#
# Revision 1.13  2001/10/23 18:52:43  warmerda
# modify initializer for Peppers
#
# Revision 1.12  2001/10/19 14:46:16  warmerda
# added SetGeogCS() and __str__
#
# Revision 1.11  2001/10/10 20:47:49  warmerda
# added some OSR methods
#
# Revision 1.10  2001/03/14 21:00:41  warmerda
# Fixed bug in Get/SetAttrValue().
#
# Revision 1.9  2000/11/17 17:16:13  warmerda
# added ImportFromESRI()
#
# Revision 1.8  2000/10/20 04:20:59  warmerda
# added SetStatePlane
#
# Revision 1.7  2000/09/14 21:06:49  warmerda
# added GetWellKnownGeogCSAsWKT
#
# Revision 1.6  2000/08/30 20:31:25  warmerda
# added some more methods
#
# Revision 1.5  2000/08/30 20:06:14  warmerda
# added projection method list functions
#
# Revision 1.4  2000/07/13 17:37:32  warmerda
# added CloneGeogCS
#
# Revision 1.3  2000/07/11 01:02:06  warmerda
# added ExportToProj4()
#
# Revision 1.2  2000/07/09 20:56:38  warmerda
# added exportToPrettyWkt
#
# Revision 1.1  2000/03/22 01:10:49  warmerda
# New
#
#

import _gdal
import gdal

def GetProjectionMethods():
    return _gdal.OPTGetProjectionMethods()

def GetWellKnownGeogCSAsWKT( name ):
    srs = SpatialReference()
    srs.SetWellKnownGeogCS( name )
    return srs.ExportToWkt()

class SpatialReference:

    def __init__(self,obj=None):
        if obj is None:
            self._o = _gdal.OSRNewSpatialReference( "" )
        else:
            self._o = obj
            _gdal.OSRReference( self._o )

    def __del__(self):
        if _gdal.OSRDereference( self._o ) == 0:
            _gdal.OSRDestroySpatialReference( self._o )

    def ImportFromWkt( self, wkt ):
        return _gdal.OSRImportFromWkt( self._o, wkt )

    def ImportFromESRI( self, prj_lines ):
        return _gdal.OSRImportFromESRI( self._o, prj_lines )

    def ExportToWkt(self):
        return _gdal.OSRExportToWkt( self._o )

    def ExportToPrettyWkt(self,simplify=0):
        return _gdal.OSRExportToPrettyWkt( self._o, simplify )

    def ExportToProj4(self):
        return _gdal.OSRExportToProj4( self._o )

    def CloneGeogCS(self):
        o = _gdal.OSRCloneGeogCS( self._o )
        if o is None:
            return None
        else:
            return SpatialReference(obj=o)
    
    def Clone(self):
        o = SpatialReference(obj=_gdal.OSRClone( self._o ))
        if o is None:
            return None
        else:
            return SpatialReference(obj=o)
    
    def Validate(self):
        return _gdal.OSRValidate( self._o )
    
    def StripCTParms(self):
        return _gdal.OSRStripCTParms( self._o )
    
    def FixupOrdering(self):
        return _gdal.OSRFixupOrdering( self._o )
    
    def Fixup(self):
        return _gdal.OSRFixup( self._o )
    
    def MorphToESRI(self):
        return _gdal.OSRMorphToESRI( self._o )
    
    def MorphFromESRI(self):
        return _gdal.OSRMorphFromESRI( self._o )
    
    def ImportFromEPSG(self,code):
        return _gdal.OSRImportFromEPSG( self._o, code )

    def IsGeographic(self):
        return _gdal.OSRIsGeographic( self._o )
    
    def IsProjected(self):
        return _gdal.OSRIsProjected( self._o )

    def GetAttrValue(self, name, child = 0):
        return _gdal.OSRGetAttrValue(self._o, name, child)
    
    def SetAttrValue(self, name, value):
        return _gdal.OSRSetAttrValue(self._o, name, value)

    def SetWellKnownGeogCS(self, name):
        return _gdal.OSRSetWellKnownGeogCS(self._o, name)

    def SetFromUserInput(self, name):
        return _gdal.OSRSetFromUserInput(self._o, name)

    def SetGeogCS( self, geog_name, datum_name, ellipsoid_name,
                   semi_major, inv_flattening,
                   pm_name = 'Greenwich', pm_offset = 0.0,
                   units = 'degree', conv_to_radian = 0.0174532925199433 ):
        return _gdal.OSRSetGeogCS( self._o, geog_name, datum_name,
                                   ellipsoid_name, semi_major, inv_flattening,
                                   pm_name, pm_offset,
                                   units, conv_to_radian )
    
    def SetProjCS(self, name = "unnamed" ):
        return _gdal.OSRSetProjCS(self._o, name)
    
    def IsSameGeogCS(self, other):
        return _gdal.OSRIsSameGeogCS(self._o, other._o)

    def IsSame(self, other):
        return _gdal.OSRIsSame(self._o, other._o)

    def SetLinearUnits(self, units_name, to_meters ):
        return _gdal.OSRSetLinearUnits( self._o, units_name, to_meters )

    def SetAuthority( self, target_key, authority_name, authority_code ):
        return _gdal.OSRSetAuthority( self._o, target_key, authority_name,
                                      int(authority_code) )

    def GetAuthorityCode( self, target_key ):
        return _gdal.OSRGetAuthorityCode( self._o, target_key )
    
    def GetAuthorityName( self, target_key ):
        return _gdal.OSRGetAuthorityName( self._o, target_key )
    
    def SetUTM(self, zone, is_north = 1):
        return _gdal.OSRSetUTM(self._o, zone, is_north )

    def SetStatePlane(self, zone, is_nad83 = 1 ):
        return _gdal.OSRSetStatePlane(self._o, zone, is_nad83 )

    def SetAttrValue( self, node_path, value ):
        return _gdal.OSRSetAttrValue( self._o, node_path, value )
    
    def SetProjParm( self, name, value ):
        return _gdal.OSRSetProjParm( self._o, name, value )

    def __str__( self ):
        return self.ExportToPrettyWkt()

    
class CoordinateTransformation:
    
    def __init__(self,source,target = None):
        """
        Initialize coordinate transform.

        source -- source osr.SpatialReference coordinate system.
        target -- destination osr.SpatialReference coordinate system.
        """
        #
        # NOTE: A special requirement of the Atlantis Peppers system is
        # that it needs to be able to instantiate a coordinate tranform
        # by just passing in a tuple with two SpatialReference objects.  We
        # assume this is the case if target is None.

        self._o = None
        if target is None:
            target = source[1]
            source = source[0]

        gdal.ErrorReset()
        self._o = _gdal.OCTNewCoordinateTransformation( source._o, target._o )
        if self._o is None or self._o == 'NULL':
            if len(gdal.GetLastErrorMsg()) > 0:
                raise ValueError, gdal.GetLastErrorMsg()
            else:
                raise ValueError, 'Failed to create coordinate transformation.'

    def __del__(self):
        if self._o: 
            _gdal.OCTDestroyCoordinateTransformation( self._o )

    def TransformPoint(self, x, y, z = 0):
        points = [(x,y,z),]
        points_ret = self.TransformPoints(points)
        return points_ret[0]

    def TransformPoints(self, points):
        return _gdal.OCTTransform(self._o, points)
