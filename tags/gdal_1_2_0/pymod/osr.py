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
# Revision 1.37  2004/02/25 08:13:40  dron
# Added wrapper for OSRExportToUSGS() function.
#
# Revision 1.36  2004/02/22 10:25:12  dron
# Added wrapper for OSRImportFromUSGS().
#
# Revision 1.35  2004/02/05 17:08:57  dron
# Added wrapper for OSRSetHOM2PNO().
#
# Revision 1.34  2004/01/31 09:54:28  dron
# Fixed projection parameters number mismatch in PCI import/export functions.
#
# Revision 1.33  2004/01/30 09:58:32  dron
# Wrapper for OSRExportToPCI() function.
#
# Revision 1.32  2004/01/29 15:22:30  dron
# Added wrapper for OSRImportFromPCI().
#
# Revision 1.31  2003/11/03 16:55:18  dron
# Added missed 'scale' parameter to SetPS().
#
# Revision 1.30  2003/06/21 23:27:13  warmerda
# added TOWGS84 and PROJ.4 import
#
# Revision 1.29  2003/06/20 18:28:15  warmerda
# added all the projection specific Set methods
#
# Revision 1.28  2003/06/10 09:26:55  dron
# Added SetAngularUnits() and GetAngularUnits().
#
# Revision 1.27  2003/05/30 21:47:37  warmerda
# added OSRSetStatePlaneWithUnits
#
# Revision 1.26  2003/05/30 15:38:59  warmerda
# updated to use SetStatePlaneWithUnits
#
# Revision 1.25  2003/03/28 17:47:41  warmerda
# added lots of stuff for accessing projection parameters
#
# Revision 1.24  2003/03/21 22:23:27  warmerda
# added xml support
#
# Revision 1.23  2003/03/07 16:29:39  warmerda
# NULL fixes
#
# Revision 1.22  2003/02/25 04:57:37  warmerda
# added CopyGeogCSFrom()
#
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
from _gdal import ptrcreate, ptrfree, ptrvalue, ptrset, ptrcast, ptradd, ptrmap

SRS_PP_CENTRAL_MERIDIAN         = "central_meridian"
SRS_PP_SCALE_FACTOR             = "scale_factor"
SRS_PP_STANDARD_PARALLEL_1      = "standard_parallel_1"
SRS_PP_STANDARD_PARALLEL_2      = "standard_parallel_2"
SRS_PP_PSEUDO_STD_PARALLEL_1    = "pseudo_standard_parallel_1"
SRS_PP_LONGITUDE_OF_CENTER      = "longitude_of_center"
SRS_PP_LATITUDE_OF_CENTER       = "latitude_of_center"
SRS_PP_LONGITUDE_OF_ORIGIN      = "longitude_of_origin"
SRS_PP_LATITUDE_OF_ORIGIN       = "latitude_of_origin"
SRS_PP_FALSE_EASTING            = "false_easting"
SRS_PP_FALSE_NORTHING           = "false_northing"
SRS_PP_AZIMUTH                  = "azimuth"
SRS_PP_LONGITUDE_OF_POINT_1     = "longitude_of_point_1"
SRS_PP_LATITUDE_OF_POINT_1      = "latitude_of_point_1"
SRS_PP_LONGITUDE_OF_POINT_2     = "longitude_of_point_2"
SRS_PP_LATITUDE_OF_POINT_2      = "latitude_of_point_2"
SRS_PP_LONGITUDE_OF_POINT_3     = "longitude_of_point_3"
SRS_PP_LATITUDE_OF_POINT_3      = "latitude_of_point_3"
SRS_PP_RECTIFIED_GRID_ANGLE     = "rectified_grid_angle"
SRS_PP_LANDSAT_NUMBER           = "landsat_number"
SRS_PP_PATH_NUMBER              = "path_number"
SRS_PP_PERSPECTIVE_POINT_HEIGHT = "perspective_point_height"
SRS_PP_FIPSZONE                 = "fipszone"
SRS_PP_ZONE                     = "zone"

SRS_WGS84_SEMIMAJOR             = 6378137.0
SRS_WGS84_INVFLATTENING         = 298.257223563


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

    def Reference( self ):
        return _gdal.OSRReference(self._o)

    def Dereference( self ):
        return _gdal.OSRDereference(self._o)

    def ImportFromWkt( self, wkt ):
        return _gdal.OSRImportFromWkt( self._o, wkt )

    def ImportFromProj4( self, proj4 ):
        return _gdal.OSRImportFromProj4( self._o, proj4 )

    def ImportFromESRI( self, prj_lines ):
        return _gdal.OSRImportFromESRI( self._o, prj_lines )

    def ImportFromPCI( self, proj, units = "METRE", proj_parms = None ):
	"""Imports PCI styled projection definition.
	
	proj --- string	representing projection definition in PCI format,
	units --- grid units code ("DEGREE" or "METRE", later is default),
	proj_parms --- tuple of 17 coordinate system parameters (double
	precition floating point values). None by default (i.e., all values
	zeroed). See OGRSpatialReference::importFromPCI() C++ function for
	details.
	"""
	if proj_parms is None:
	    return _gdal.OSRImportFromPCI( self._o, proj, units )
	else:
	    return _gdal.OSRImportFromPCI( self._o, proj, units, proj_parms )

    def ImportFromUSGS( self, proj_code, zone=0, \
		        proj_parms=None, datum_code=0 ):
	"""Imports USGS styled projection definition.
	
	proj_code --- USGS projection system code,
	zone --- zone number (for UTM and State Plane projection systems only),
	proj_parms --- tuple of 15 USGS coordinate system parameters (double
	precition floating point values in packed DMS format). None by default
	(i.e., all values zeroed).
	datum_code --- USGS ellipsoid code.
	See OGRSpatialReference::importFromUSGS() C++ function for details.
	"""
	return _gdal.OSRImportFromUSGS( self._o, proj_code, zone, \
					proj_parms, datum_code )

    def ImportFromXML( self, xml ):
        return _gdal.OSRImportFromXML( self._o, xml )

    def ExportToWkt(self):
        return _gdal.OSRExportToWkt( self._o )

    def ExportToPrettyWkt(self,simplify=0):
        return _gdal.OSRExportToPrettyWkt( self._o, simplify )

    def ExportToProj4(self):
        return _gdal.OSRExportToProj4( self._o )

    def ExportToPCI(self):
	"""Return PCI styled projection definition.
	
	Returns tuple (proj, units, (proj_parms)) where proj is a string
	representing projection definition in PCI format, units is a string
	containing grid units code ("METRE" or "DEGREE") and (proj_parms)
	is a tuple of 17 double precition floating point values with
	projection parameters. See description of
	OGRSpatialReference::exportToPCI() C++ function for details."""
        return _gdal.OSRExportToPCI( self._o )

    def ExportToUSGS(self):
	"""Return USGS styled projection definition.
	
	Returns tuple (proj_code, zone, (proj_parms), datum_code) where
	proj_code is an USGS projection system code, zone is a zone number
	for UTM and State Plane projection systems only), (proj_parms)
	is a tuple of 15 double precition floating point values with
	USGS projection parameters and datum_code is an USGS ellipsoid code.
	See description of OGRSpatialReference::exportToUSGS() C++ function
	for details."""
        return _gdal.OSRExportToUSGS( self._o )

    def ExportToXML( self, dialect = '' ):
        return _gdal.OSRExportToXML( self._o, dialect )

    def CloneGeogCS(self):
        o = _gdal.OSRCloneGeogCS( self._o )
        if o is None or o == 'NULL':
            return None
        else:
            return SpatialReference(obj=o)
    
    def Clone(self):
        o = SpatialReference(obj=_gdal.OSRClone( self._o ))
        if o is None or o == 'NULL':
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

    def CopyGeogCSFrom( self, src_srs ):
        return _gdal.OSRCopyGeogCSFrom( self._o, src_srs._o )

    def SetTOWGS84( self, p1, p2, p3, p4=0.0, p5=0.0, p6=0.0, p7=0.0 ):
        return _gdal.OSRSetTOWGS84( self._o, p1, p2, p3, p4, p5, p6, p7 )

    def GetTOWGS84( self ):
        pd = ptrcreate( 'double', 0.0, 7 )
        err = _gdal.OSRGetTOWGS84( self._o, pd, 7 )
        if err != 0:
            print err
            raise ValueError, ('GetTOWGS84() failed, err = %d.' % err)

        result = ( ptrvalue(pd,0),
                   ptrvalue(pd,1), 
                   ptrvalue(pd,2), 
                   ptrvalue(pd,3), 
                   ptrvalue(pd,4), 
                   ptrvalue(pd,5), 
                   ptrvalue(pd,6) )
        ptrfree( pd )
        return result
    
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

    def SetAngularUnits(self, units_name, to_radians ):
        return _gdal.OSRSetAngularUnits( self._o, units_name, to_radians )

    def GetAngularUnits( self ):
        return _gdal.OSRGetAngularUnits( self._o, 'NULL' )

    def SetLinearUnits(self, units_name, to_meters ):
        return _gdal.OSRSetLinearUnits( self._o, units_name, to_meters )

    def GetLinearUnits( self ):
        return _gdal.OSRGetLinearUnits( self._o, 'NULL' )

    def GetLinearUnitsName( self ):
        name = None
        if self.IsProjected():
            name = self.GetAttrValue( 'PROJCS|UNIT', 0 )
        elif self.IsLocal():
            name = self.GetAttrValue( 'LOCAL_CS|UNIT', 0 )

        if name is None:
            return 'Meter'
        else:
            return name

    def SetAuthority( self, target_key, authority_name, authority_code ):
        return _gdal.OSRSetAuthority( self._o, target_key, authority_name,
                                      int(authority_code) )

    def GetAuthorityCode( self, target_key ):
        return _gdal.OSRGetAuthorityCode( self._o, target_key )
    
    def GetAuthorityName( self, target_key ):
        return _gdal.OSRGetAuthorityName( self._o, target_key )
    
    def SetUTM(self, zone, is_north = 1):
        return _gdal.OSRSetUTM(self._o, zone, is_north )

    def SetStatePlane(self, zone, is_nad83 = 1, overrideunitsname='', 
                      overrideunits = 0.0 ):
        return _gdal.OSRSetStatePlaneWithUnits(self._o, zone, is_nad83,
                                               overrideunitsname, 
                                               overrideunits )

    def SetAttrValue( self, node_path, value ):
        return _gdal.OSRSetAttrValue( self._o, node_path, value )
    
    def SetProjParm( self, name, value ):
        return _gdal.OSRSetProjParm( self._o, name, value )

    def GetProjParm( self, name, default_val = 0.0 ):
        return _gdal.OSRGetProjParm( self._o, name, default_val, 'NULL' )

    def SetNormProjParm( self, name, value ):
        return _gdal.OSRSetNormProjParm( self._o, name, value )

    def GetNormProjParm( self, name, default_val = 0.0 ):
        return _gdal.OSRGetNormProjParm( self._o, name, default_val, 'NULL' )

    def __str__( self ):
        return self.ExportToPrettyWkt()

    def SetACEA( self, stdp1, stdp2, clat, clong, fe, fn ):
	return _gdal.OSRSetACEA( self._o, stdp1, stdp2, clat, clong, fe, fn )

    def SetAE( self, clat, clong, fe, fn ):
	return _gdal.OSRSetAE( self._o, clat, clong, fe, fn )

    def SetCEA( self, stdp1, cm, fe, fn ):
	return _gdal.OSRSetCEA( self._o, stdp1, cm, fe, fn )

    def SetCS( self, clat, clong, fe, fn ):
	return _gdal.OSRSetCS( self._o, clat, clong, fe, fn )

    def SetEC( self, stdp1, stdp2, clat, clong, fe, fn ):
	return _gdal.OSRSetEC( self._o, stdp1, stdp2, clat, clong, fe, fn )

    def SetEckertIV( self, cm, fe, fn ):
	return _gdal.OSRSetEckertIV( self._o, cm, fe, fn )

    def SetEckertVI( self, cm, fe, fn ):
	return _gdal.OSRSetEckertVI( self._o, cm, fe, fn )

    def SetEquirectangular( self, clat, clong, fe, fn ):
	return _gdal.OSRSetEquirectangular( self._o, clat, clong, fe, fn )

    def SetGS( self, cm, fe, fn ):
	return _gdal.OSRSetGS( self._o, cm, fe, fn )

    def SetGnomonic( self, clat, clong, fe, fn ):
	return _gdal.OSRSetGnomic( self._o, clat, clong, fe, fn )

    def SetHOM( self, clat, clong, azi, recttoskew, scale, fe, fn ):
	"""Set a Hotine Oblique Mercator projection using azimuth angle.
	
	clat --- Latitude of the projection origin.
	clong --- Longitude of the projection origin.
	azi --- Azimuth, measured clockwise from North, of the projection
	centerline.
	recttoskew --- ?
	scale --- Scale factor applies to the projection origin.
	fe --- False easting.
	fn --- False northing."""
	return _gdal.OSRSetHOM( self._o, \
	    clat, clong, azi, recttoskew, scale, fe, fn )

    def SetHOM2PNO( self, clat, lat1, long1, lat2, long2, scale, fe, fn ):
	"""Set a Hotine Oblique Mercator projection using two points
	on projection centerline.
	
	clat --- Latitude of the projection origin.
	lat1 --- Latitude of the first point on center line.
	long1 --- Longitude of the first point on center line.
	lat2 --- Latitude of the second point on center line.
	long2 --- Longitude of the second point on center line.
	scale --- Scale factor applies to the projection origin.
	fe --- False easting.
	fn --- False northing."""
	return _gdal.OSRSetHOM2PNO( self._o, \
	    clat, lat1, long1, lat2, long2, scale, fe, fn )

    def SetKrovak( self, clat, clong, azi, pstdparlat, scale, fe, fn ):
	return _gdal.OSRSetKrovak( self._o, clat, clong, azi, pstdparlat, scale, fe, fn )

    def SetLAEA( self, clat, clong, fe, fn ):
	return _gdal.OSRSetLAEA( self._o, clat, clong, fe, fn )

    def SetLCC( self, stdp1, stdp2, clat, clong, fe, fn ):
	return _gdal.OSRSetLCC( self._o, stdp1, stdp2, clat, clong, fe, fn )

    def SetLCCB( self, stdp1, stdp2, clat, clong, fe, fn ):
	return _gdal.OSRSetLCCB( self._o, stdp1, stdp2, clat, clong, fe, fn )

    def SetLCC1SP( self, clat, clong, scale, fe, fn ):
	return _gdal.OSRSetLCC1SP( self._o, clat, clong, scale, fe, fn )

    def SetMC( self, clat, clong, fe, fn ):
	return _gdal.OSRSetMC( self._o, clat, clong, fe, fn )

    def SetMercator( self, clat, clong, scale, fe, fn ):
	return _gdal.OSRSetMercator( self._o, clat, clong, scale, fe, fn )

    def SetMollweide( self, cm, fe, fn ):
	return _gdal.OSRSetMollweide( self._o, cm, fe, fn )

    def SetNZMG( self, clat, clong, fe, fn ):
	return _gdal.OSRSetNZMG( self._o, clat, clong, fe, fn )

    def SetOS( self, olat, cm, fe, fn ):
	return _gdal.OSRSetOS( self._o, olat, cm, fe, fn )

    def SetOrthographic( self, clat, clong, fe, fn ):
	return _gdal.OSRSetOrthographic( self._o, clat, clong, fe, fn )

    def SetPolyconic( self, clat, clong, fe, fn ):
	return _gdal.OSRSetPolyconic( self._o, clat, clong, fe, fn )

    def SetPS( self, clat, clong, scale, fe, fn ):
	return _gdal.OSRSetPS( self._o, clat, clong, scale, fe, fn )

    def SetRobinson( self, clong, fe, fn ):
	return _gdal.OSRSetRobinson( self._o, clong, fe, fn )

    def SetSinusoidal( self, clong, fe, fn ):
	return _gdal.OSRSetSinusoidal( self._o, clong, fe, fn )

    def SetStereographic( self, clat, clong, scale, fe, fn ):
	return _gdal.OSRSetStereographic( self._o, clat, clong, scale, fe, fn )

    def SetSOC( self, lato, cm, fe, fn ):
	return _gdal.OSRSetSOC( self._o, lato, cm, fe, fn )

    def SetTM( self, clat, clong, scale, fe, fn ):
	return _gdal.OSRSetTM( self._o, clat, clong, scale, fe, fn )

    def SetTMSO( self, clat, clong, scale, fe, fn ):
	return _gdal.OSRSetTMSO( self._o, clat, clong, scale, fe, fn )

    def SetTMG( self, clat, clong, fe, fn ):
	return _gdal.OSRSetTMG( self._o, clat, clong, fe, fn )

    def SetVDG( self, clong, fe, fn ):
	return _gdal.OSRSetVDG( self._o, clong, fe, fn )

    
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
