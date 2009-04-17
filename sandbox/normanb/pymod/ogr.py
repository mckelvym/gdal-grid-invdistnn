###############################################################################
#  $Id: ogr.py 11670 2007-06-19 15:44:59Z warmerdam $
# 
#  Project:  OpenGIS Simple Features Reference Implementation
#  Purpose:  OGR Python Shadow Class Implementations
#  Author:   Frank Warmerdam, warmerdam@pobox.com
# 
###############################################################################
#  Copyright (c) 2002, Frank Warmerdam <warmerdam@pobox.com>
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
###############################################################################

import _gdal
import gdal
import osr
import types
import sys

from _gdal import ptrcreate, ptrfree, ptrvalue, ptrset, ptrcast, ptradd, ptrmap, ptrptrcreate, ptrptrvalue, ptrptrset


class OGRError(Exception): pass

# OGRwkbGeometryType

wkb25Bit = -2147483648 # 0x80000000
wkbUnknown = 0
wkbPoint = 1  
wkbLineString = 2
wkbPolygon = 3
wkbMultiPoint = 4
wkbMultiLineString = 5
wkbMultiPolygon = 6
wkbGeometryCollection = 7
wkbNone = 100
wkbLinearRing = 101
wkbPoint25D =              wkbPoint              + wkb25Bit
wkbLineString25D =         wkbLineString         + wkb25Bit
wkbPolygon25D =            wkbPolygon            + wkb25Bit
wkbMultiPoint25D =         wkbMultiPoint         + wkb25Bit
wkbMultiLineString25D =    wkbMultiLineString    + wkb25Bit
wkbMultiPolygon25D =       wkbMultiPolygon       + wkb25Bit
wkbGeometryCollection25D = wkbGeometryCollection + wkb25Bit

# OGRFieldType

OFTInteger = 0
OFTIntegerList= 1
OFTReal = 2
OFTRealList = 3
OFTString = 4
OFTStringList = 5
OFTWideString = 6
OFTWideStringList = 7
OFTBinary = 8
OFTDate = 9
OFTTime = 10
OFTDateTime = 11

# OGRJustification

OJUndefined = 0
OJLeft = 1
OJRight = 2

wkbXDR = 0
wkbNDR = 1

###############################################################################
# Constants for testing Capabilities

# Layer
OLCRandomRead          = "RandomRead"
OLCSequentialWrite     = "SequentialWrite"
OLCRandomWrite         = "RandomWrite"
OLCFastSpatialFilter   = "FastSpatialFilter"
OLCFastFeatureCount    = "FastFeatureCount"
OLCFastGetExtent       = "FastGetExtent"
OLCCreateField         = "CreateField"
OLCTransactions        = "Transactions"
OLCDeleteFeature       = "DeleteFeature"
OLCFastSetNextByIndex  = "FastSetNextByIndex"

# DataSource
ODsCCreateLayer        = "CreateLayer"
ODsCDeleteLayer        = "DeleteLayer"

# Driver
ODrCCreateDataSource   = "CreateDataSource"
ODrCDeleteDataSource   = "DeleteDataSource"

###############################################################################
#     Do this on module instantiation.

_gdal.OGRRegisterAll()
    
###############################################################################
# Various free standing functions.

def Open( filename, update = 0 ):
    """Return an OGR DataSource
update=0,1 -- open it for update"""

    ds_o = _gdal.OGROpen( filename, update, 'NULL' )

    if ds_o is None or ds_o == 'NULL':
        raise OGRError, 'Unable to open: ' + filename
    else:
        ds = DataSource( ds_o )
        return ds

def OpenShared( filename, update = 0 ):
    
    drv_ptr = ptrptrcreate( 'void' )
    ptrptrset( drv_ptr, 'NULL' )

    ds_o = _gdal.OGROpenShared( filename, update, drv_ptr )
    
    driver_o = ptrcast(ptrptrvalue(drv_ptr),'OGRSFDriverH')
    ptrfree( drv_ptr )
    
    if ds_o is None or ds_o == 'NULL':
        raise OGRError, 'Unable to open: ' + filename
    else:
        ds = DataSource( ds_o )
        ds.driver_o = driver_o

        return ds

def GetDriverCount():
    return _gdal.OGRGetDriverCount()

def GetDriver( driver_index ):
    dr_o = _gdal.OGRGetDriver( driver_index )
    if dr_o is None or dr_o == 'NULL':
        raise OGRError, 'Unable to find ogr.Driver named at index "%s".' % driver_index
    else:
        return Driver( dr_o )

def GetDriverByName( name ):
    dr_o = _gdal.OGRGetDriverByName( name )
    if dr_o is None or dr_o == 'NULL':
        raise OGRError, 'Unable to find ogr.Driver named "%s".' % name
    else:
        return Driver( dr_o )

def GetOpenDSCount():
    return _gdal.OGRGetOpenDSCount()

def GetOpenDS( i ):
    _o = _gdal.OGRGetOpenDS( i )
    if _o is None or _o == 'NULL':
        return None
    else:
        return DataSource( obj = _o )

def SetGenerate_DB2_V72_BYTE_ORDER( flag ):
    return _gdal.OGRSetGenerate_DB2_V72_BYTE_ORDER( flag )

#############################################################################
# OGRSFDriver

class Driver:

    def __init__(self,obj=None):
        if obj is None:
            raise OGRError, 'OGRDriver may not be directly instantiated.'
        self._o = obj

    def GetName( self ):
        return _gdal.OGR_Dr_GetName( self._o )
    
    def TestCapability( self, cap ):
        """Test the capabilities of the Driver.
See the constants at the top of ogr.py"""
        return _gdal.OGR_Dr_TestCapability( self._o, cap )

    def Open( self, filename, update = 0 ):
        ds_o = _gdal.OGR_Dr_Open( self._o, filename, update )
        if ds_o is None or ds_o == 'NULL':
            return None
        else:
            return DataSource( ds_o )

    def CreateDataSource( self, filename, options = [] ):
        md_c = _gdal.ListToStringList( options )
        ds_o = _gdal.OGR_Dr_CreateDataSource( self._o, filename, md_c )
        _gdal.CSLDestroy(md_c)
        
        if ds_o is None or ds_o == 'NULL':
            raise OGRError, _gdal.CPLGetLastErrorMsg()
        else:
            return DataSource( ds_o )

    def CopyDataSource( self, src_ds, filename, options = [] ):
        md_c = _gdal.ListToStringList( options )
        ds_o = _gdal.OGR_Dr_CopyDataSource( self._o, src_ds._o, filename, md_c)
        _gdal.CSLDestroy(md_c)
        
        if ds_o is None or ds_o == 'NULL':
            raise OGRError, _gdal.CPLGetLastErrorMsg()
        else:
            return DataSource( ds_o )

    def DeleteDataSource( self, filename ):
        return _gdal.OGR_Dr_DeleteDataSource( self._o, filename )

#############################################################################
# OGRDataSource

class DataSource:
    def __init__(self,obj=None):
        if obj is None:
            raise OGRError, 'OGRDataSource may not be directly instantiated.'
        self._o = obj

    def __len__(self):
        """Returns the number of layers on the datasource"""
        return self.GetLayerCount()

    def __getitem__(self, value):
        """Support dictionary, list, and slice -like access to the datasource.
ds[0] would return the first layer on the datasource.
ds['aname'] would return the layer named "aname".
ds[0:4] would return a list of the first four layers."""
        if isinstance(value, types.SliceType):
            output = []
            for i in xrange(value.start,value.stop,step=value.step):
                try:
                    output.append(self.GetLayer(i))
                except OGRError: #we're done because we're off the end
                    return output
            return output
        if isinstance(value, types.IntType):
            if value > len(self)-1:
                raise IndexError
            return self.GetLayer(value)
        elif isinstance(value,types.StringType):
            return self.GetLayer(value)
        else:
            raise TypeError,"Input %s is not of String or Int type" % type(value)

    def Destroy(self):
        _gdal.OGR_DS_Destroy( self._o )
        self._o = None

    def Release(self):
        _gdal.OGRReleaseDataSource( self._o )
        self._o = None

    def Reference(self):
        return _gdal.OGR_DS_Reference(self._o)
    
    def Dereference(self):
        return _gdal.OGR_DS_Dereference(self._o)

    def GetRefCount(self):
        return _gdal.OGR_DS_GetRefCount(self._o)

    def GetSummaryRefCount(self):
        return _gdal.OGR_DS_GetSummaryRefCount(self._o)

    def GetName(self):
        """Returns the name of the datasource"""
        return _gdal.OGR_DS_GetName( self._o )
    
    def GetLayerCount(self):
        """Returns the number of layers on the datasource"""
        return _gdal.OGR_DS_GetLayerCount( self._o )

    def GetLayer(self,iLayer=0):
        """Return the layer given an index or a name"""
        if isinstance(iLayer, types.StringType):
            l_obj = _gdal.OGR_DS_GetLayerByName( self._o, iLayer)
        elif isinstance(iLayer, types.IntType):
            l_obj = _gdal.OGR_DS_GetLayer( self._o, iLayer)
        else:
            raise TypeError, "Input %s is not of String or Int type" % type(iLayer)
        
        if l_obj is not None and l_obj != 'NULL':
            return Layer( l_obj )
        else:
            raise OGRError, 'No layer %d on datasource' % iLayer

    def GetLayerByName(self,name):
        # If given a layer name, scan for it. 
        l_obj = _gdal.OGR_DS_GetLayerByName( self._o, name)
        if l_obj is not None and l_obj != 'NULL':
            return Layer( l_obj )
        else:
            raise OGRError, 'No layer %s on datasource' % name 

    def DeleteLayer( self, iLayer ):
        return _gdal.OGR_DS_DeleteLayer( self._o, iLayer )

    def CreateLayer(self, name, srs = None, geom_type = wkbUnknown,
                    options = [] ):
        if srs is None:
            srs_o = 'NULL'
        else:
            srs_o = srs._o
            
        md_c = _gdal.ListToStringList( options )
        obj = _gdal.OGR_DS_CreateLayer( self._o, name, srs_o, geom_type, md_c)
        _gdal.CSLDestroy(md_c)
        if obj is None or obj == 'NULL':
            raise OGRError, gdal.GetLastErrorMsg()
        else:
            return Layer( obj = obj )

    def CopyLayer(self, src_layer, new_name, options = [] ):
        md_c = _gdal.ListToStringList( options )
        obj = _gdal.OGR_DS_CopyLayer( self._o, src_layer._o, new_name, md_c)
        _gdal.CSLDestroy(md_c)
        if obj is None and obj != 'NULL':
            raise OGRError, gdal.GetLastErrorMsg()
        else:
            return Layer( obj = obj )

    def TestCapability( self, cap ):
        """Test the capabilities of the DataSource.
See the constants at the top of ogr.py"""
        return _gdal.OGR_DS_TestCapability( self._o, cap )

    def ExecuteSQL( self, statement, region = 'NULL', dialect = "" ):
        l_obj = _gdal.OGR_DS_ExecuteSQL( self._o, statement, region, dialect )
        if l_obj is not None and l_obj != 'NULL':
            return Layer( l_obj )
        else:
            return None

    def ReleaseResultSet( self, layer ):
        _gdal.OGR_DS_ReleaseResultSet( self._o, layer._o )

    def GetDriver( self ):
        """Returns the driver of the datasource"""
        x = _gdal.OGR_DS_GetDriver( self._o )
        if x is None or x == 'NULL':
            return None
        else:
            return Driver( x )
    
#############################################################################
# OGRLayer

class Layer:
    def __init__(self,obj=None):
        if obj is None:
            raise OGRError, 'OGRLayer may not be directly instantiated.'
        self._o = obj

    def __len__(self):
        """Returns the number of features in the layer"""
        return self.GetFeatureCount()

    def Reference(self):
        return _gdal.OGR_L_Reference(self._o)
    
    def Dereference(self):
        return _gdal.OGR_L_Dereference(self._o)

    def GetRefCount(self):
        return _gdal.OGR_L_GetRefCount(self._o)

    def SetSpatialFilter( self, geom ):
        """Sets an ogr.Geometry as a spatial filter"""
        if geom is None:
            geom_o = 'NULL'
        else:
            geom_o = geom._o
        _gdal.OGR_L_SetSpatialFilter( self._o, geom_o )

    def SetSpatialFilterRect( self, minx, miny, maxx, maxy ):
        """Sets a four-tuple extent as a spatial filter"""
        _gdal.OGR_L_SetSpatialFilterRect( self._o, minx, miny, maxx, maxy )

    def GetSpatialFilter( self ):
        """Returns the Spatial filter of the Layer as a Geometry"""
        geom_o = _gdal.OGR_L_GetSpatialFilter( self._o )
        if geom_o is None or geom_o == 'NULL':
            return None
        else:
            return Geometry( obj = geom_o, thisown = 0 )

    def SetAttributeFilter( self, where_clause = None ):
        filter = gdal.ToNULLableString( where_clause )
        result = _gdal.OGR_L_SetAttributeFilter( self._o, filter )
        gdal.FreeNULLableString(filter)
        return result
        
    def ResetReading( self ):
        _gdal.OGR_L_ResetReading( self._o )

    def GetName( self ):
        """Returns the name of the layer"""
        return _gdal.OGR_FD_GetName( _gdal.OGR_L_GetLayerDefn( self._o ) )
    
    def GetFeature( self, fid ):
        """Returns a feature with a given feature id (fid)"""
        f_o = _gdal.OGR_L_GetFeature( self._o, fid )
        if f_o is None or f_o == 'NULL':
            return None
        else:
            newfeat = Feature( obj = f_o )
            newfeat.thisown = 1
            return newfeat

    def GetNextFeature( self ):
        """Iterator that returns the next feature in the sequence"""
        f_o = _gdal.OGR_L_GetNextFeature( self._o )
        if f_o is None or f_o == 'NULL':
            return None
        else:
            newfeat = Feature( obj = f_o )
            newfeat.thisown = 1
            return newfeat

    def SetNextByIndex( self, new_index ):
        return _gdal.OGR_L_SetNextByIndex( self._o, new_index )
        
    def SetFeature( self, feat ):
        return _gdal.OGR_L_SetFeature( self._o, feat._o )

    def CreateFeature( self, feat ):
        return _gdal.OGR_L_CreateFeature( self._o, feat._o )

    def DeleteFeature( self, fid ):
        return _gdal.OGR_L_DeleteFeature( self._o, fid )

    def SyncToDisk( self ):
        """Flushes the layer to disk"""
        return _gdal.OGR_L_SyncToDisk( self._o )

    def GetLayerDefn( self ):
        return FeatureDefn( obj = _gdal.OGR_L_GetLayerDefn( self._o ) )

    def GetFeatureCount( self, force = 1 ):
        return _gdal.OGR_L_GetFeatureCount( self._o, force )

    def GetExtent( self, force = 1 ):
        """Returns the extent of the layer as a four-tuple"""
        extents = _gdal.ptrcreate( 'double', 0.0, 4 )
        res = _gdal.OGR_L_GetExtent( self._o,
                  _gdal.ptrcast(extents,'OGREnvelope_p'), force )
        if res != 0:
            ret_extents = None
        else:
            ret_extents = ( _gdal.ptrvalue( extents, 0 ),
                            _gdal.ptrvalue( extents, 1 ),
                            _gdal.ptrvalue( extents, 2 ),
                            _gdal.ptrvalue( extents, 3 ) )
        _gdal.ptrfree( extents )
        
        return ret_extents
    
    def TestCapability( self, cap ):
        """Test the capabilities of the Layer.
See the constants at the top of ogr.py"""
        return _gdal.OGR_L_TestCapability( self._o, cap )

    def CreateField( self, field_def, approx_ok = 1 ):
        """Creates a new field on the layer given a FieldDef"""
        return _gdal.OGR_L_CreateField( self._o, field_def._o, approx_ok )

    def CreateFeature( self, feature ):
        """Creates a feature on the layer given a Feature"""
        return _gdal.OGR_L_CreateFeature( self._o, feature._o )

    def StartTransaction( self ):
        return _gdal.OGR_L_StartTransaction( self._o )

    def CommitTransaction( self ):
        return _gdal.OGR_L_CommitTransaction( self._o )

    def RollbackTransaction( self ):
        return _gdal.OGR_L_RollbackTransaction( self._o )

    def GetSpatialRef( self ):
        """Returns the spatial reference of the layer"""
        srs_o = _gdal.OGR_L_GetSpatialRef( self._o )
        if srs_o is not None and srs_o != 'NULL':
            return osr.SpatialReference( srs_o )
        else:
            return None

    def GetFeaturesRead( self ):
        return _gdal.OGR_L_GetFeaturesRead( self._o )

#############################################################################
# OGRFeature

class Feature:
    def __init__(self,feature_def=None,obj=None):
        if feature_def is None and obj is None:
            raise OGRError, 'ogr.Feature() needs an ogr.FeatureDefn.'
        if feature_def is not None and obj is not None:
            raise OGRError, 'ogr.Feature() cannot receive obj and feature_def.'
        if obj is not None:
            self._o = obj
            self.thisown = 0
        else:
            self._o = _gdal.OGR_F_Create( feature_def._o )
            self.thisown = 1

    def __del__(self):
        if self.thisown:
            self.Destroy()

    def __cmp__(self, other):
        """Compares a feature to another for equality"""
        return _gdal.OGR_F_Equal( self._o, other._o )

    def __copy__(self):
        return self.Clone()

    def __getattr__(self, name):
        """Returns the values of fields by the given name"""
        try:
            names = []
            for i in range(self.GetFieldCount()):
                fieldname = self.GetFieldDefnRef(i).GetName()
                names.append(fieldname)
            if name in names:
                return self.GetField(name)
            else:
                raise
        except:
            raise AttributeError, name
    def __setattr__(self, name, value):
        """Sets the values of a specified field by the given name"""
        special_names = ['_o','thisown']
        if name in special_names:
            self.__dict__[name] = value
        if name not in special_names or name not in dir(self):
            try:
                self.__getattr__(name)
                self.SetField(name, value)
            except:
                pass

    def Destroy( self ):
        if self._o is not None and self.thisown:
            _gdal.OGR_F_Destroy( self._o )
        self._o = None
        self.thisown = 0

    def GetDefnRef( self ):
        return FeatureDefn( obj = _gdal.OGR_F_GetDefnRef( self._o ) )

    def SetGeometry( self, geom ):
        if geom is None:
            return _gdal.OGR_F_SetGeometry( self._o, None )
        else:
            return _gdal.OGR_F_SetGeometry( self._o, geom._o )

    def SetGeometryDirectly( self, geom ):
        if geom is None:
            return _gdal.OGR_F_SetGeometryDirectly( self._o, None )
        else:
            if not geom.thisown:
                print 'SetGeometryDirectly() with unowned geometry!'
            geom.thisown = 0
            return _gdal.OGR_F_SetGeometryDirectly( self._o, geom._o )

    def GetGeometryRef( self ):
        geom_o = _gdal.OGR_F_GetGeometryRef( self._o )

        if geom_o is None or geom_o == 'NULL':
            return None
        else:
            return Geometry( obj = geom_o, thisown = 0 )

    def Clone( self ):
        newfeat = Feature( obj = _gdal.OGR_F_Clone( self._o ) )
        newfeat.thisown = 1
        return newfeat

    def Equal( self, other_geom ):
        return _gdal.OGR_F_Equal( self._o, other_geom._o )
    
    def GetFieldCount( self ):
        """Returns the number of fields in the feature"""
        return _gdal.OGR_F_GetFieldCount( self._o )

    def GetFieldDefnRef( self, fld_index ):
        if type(fld_index).__name__ == 'str':
            fld_index = self.GetFieldIndex(fld_index)
        return FieldDefn( obj = _gdal.OGR_F_GetFieldDefnRef( self._o,
                                                             fld_index ) )

    def GetFieldIndex( self, name ):
        return _gdal.OGR_F_GetFieldIndex( self._o, name )

    def IsFieldSet( self, fld_index ):
        if isinstance(fld_index, types.StringType):
            fld_index = self.GetFieldIndex(fld_index)
        return _gdal.OGR_F_IsFieldSet( self._o, fld_index )

    def UnsetField( self, fld_index ):
        if isinstance(fld_index, types.StringType):
            fld_index = self.GetFieldIndex(fld_index)
        _gdal.OGR_F_UnsetField( self._o, fld_index )

    def SetField( self, fld_index, value ):
        if isinstance(fld_index, types.StringType):
            fld_index = self.GetFieldIndex(fld_index)
        _gdal.OGR_F_SetFieldString( self._o, fld_index, str(value) )

    def GetFieldAsString( self, fld_index ):
        if isinstance(fld_index, types.StringType):
            fld_index = self.GetFieldIndex(fld_index)
        return _gdal.OGR_F_GetFieldAsString( self._o, fld_index )

    def GetFieldAsInteger( self, fld_index ):
        if isinstance(fld_index, types.StringType):
            fld_index = self.GetFieldIndex(fld_index)
        return _gdal.OGR_F_GetFieldAsInteger( self._o, fld_index )

    def GetFieldAsDouble( self, fld_index ):
        if isinstance(fld_index, types.StringType):
            fld_index = self.GetFieldIndex(fld_index)
        return _gdal.OGR_F_GetFieldAsDouble( self._o, fld_index )

    def GetField( self, fld_index ):
        if isinstance(fld_index, types.StringType):
            fld_index = self.GetFieldIndex(fld_index)
        return _gdal.OGR_F_GetField( self._o, fld_index )

    def GetFID( self ):
        return _gdal.OGR_F_GetFID( self._o )
    
    def SetFID( self, fid ):
        return _gdal.OGR_F_SetFID( self._o, fid )

    def DumpReadable(self):
        _gdal.OGR_F_DumpReadable( self._o, 'NULL' )
        
    def SetFrom( self, other, be_forgiving = 1 ):
        return _gdal.OGR_F_SetFrom( self._o, other._o, be_forgiving )

    def GetStyleString( self ):
        return _gdal.OGR_F_GetStyleString( self._o )

    def SetStyleString( self, style ):
        _gdal.OGR_F_SetStyleString( self._o, style )
    
class FeatureDefn:

    def __init__(self,obj=None,name='unnamed'):
        if obj is None:
            self._o = _gdal.OGR_FD_Create( name )
        else:
            self._o = obj

    def Destroy( self ):
        _gdal.OGR_FD_Destroy( self._o )

    def GetName( self ):
        return _gdal.OGR_FD_GetName( self._o )

    def GetFieldCount( self ):
        return _gdal.OGR_FD_GetFieldCount( self._o )

    def GetFieldDefn( self, i ):
        obj = _gdal.OGR_FD_GetFieldDefn( self._o, i )
        if obj is not None:
            return FieldDefn( obj = obj )
        else:
            return None

    def GetFieldIndex( self, name ):
        return _gdal.OGR_FD_GetFieldIndex( self._o, name )

    def AddFieldDefn( self, field_defn ):
        _gdal.OGR_FD_AddFieldDefn( self._o, field_defn._o )

    def GetGeomType( self ):
        return _gdal.OGR_FD_GetGeomType( self._o )

    def SetGeomType( self, geom_type ):
        _gdal.OGR_FD_SetGeomType( self._o, geom_type )
        
    def Reference( self ):
        return _gdal.OGR_FD_Reference( self._o )
        
    def Dereference( self ):
        return _gdal.OGR_FD_Dereference( self._o )
        
    def GetReferenceCount( self ):
        return _gdal.OGR_FD_GetReferenceCount( self._o )
        
#############################################################################
# OGRFieldDefn

class FieldDefn:
    
    def __init__(self,name='unnamed',field_type=OFTString, obj=None):
        if obj is None:
            self._o = _gdal.OGR_Fld_Create( name, field_type )
        else:
            self._o = obj

    def Destroy( self ):
        _gdal.OGR_Fld_Destroy( self._o )

    def GetName( self ):
        return _gdal.OGR_Fld_GetNameRef( self._o )

    def GetNameRef( self ):
        return _gdal.OGR_Fld_GetNameRef( self._o )

    def SetName( self, name ):
        _gdal.OGR_Fld_SetName( self._o, name )

    def GetType( self ):
        return _gdal.OGR_Fld_GetType( self._o )

    def SetType( self, type ):
        _gdal.OGR_Fld_SetType( self._o, type )

    def GetJustify( self ):
        return _gdal.OGR_Fld_GetJustify( self._o )
    
    def SetJustify( self, justification ):
        _gdal.OGR_Fld_SetJustify( self._o, justification )
    
    def GetWidth( self ):
        return _gdal.OGR_Fld_GetWidth( self._o )
    
    def SetWidth( self, width ):
        _gdal.OGR_Fld_SetWidth( self._o, width )
    
    def GetPrecision( self ):
        return _gdal.OGR_Fld_GetPrecision( self._o )
    
    def SetPrecision( self, precision ):
        _gdal.OGR_Fld_SetPrecision( self._o, precision )
    
#############################################################################
# OGRGeometry

def CreateGeometryFromWkb( bin_string, srs = None ):
    if srs is not None:
        srs_o = srs._o
    else:
        srs_o = ''
    _obj = _gdal.OGR_G_CreateFromWkb( bin_string, srs_o )
    if _obj is not None and _obj != 'NULL':
        result = Geometry( obj = _obj )
        result.thisown = 1
        return result
    elif len(_gdal.CPLGetLastErrorMsg()) == 0:
        raise ValueError, 'Failed to parse WKB in ogr.CreateGeometryFromWkb()'
    else:
        raise ValueError, _gdal.CPLGetLastErrorMsg()

def CreateGeometryFromWkt( string, srs = None ):
    if srs is not None:
        srs_o = srs._o
    else:
        srs_o = ''
    _obj = _gdal.OGR_G_CreateFromWkt( string, srs_o )
    if _obj is not None and _obj != 'NULL':
        result = Geometry( obj = _obj )
        result.thisown = 1
        return result
    elif len(_gdal.CPLGetLastErrorMsg()) == 0:
        raise ValueError, 'Failed to parse WKT in ogr.CreateGeometryFromWkt()'
    else:
        raise ValueError, _gdal.CPLGetLastErrorMsg()

def CreateGeometryFromGML( string ):
    _obj = _gdal.OGR_G_CreateFromGML( string )
    if _obj is not None and _obj != 'NULL':
        result = Geometry( obj = _obj )
        result.thisown = 1
        return result
    elif len(_gdal.CPLGetLastErrorMsg()) == 0:
        raise ValueError, 'Failed to parse GML in ogr.CreateGeometryFromGML()'
    else:
        raise ValueError, _gdal.CPLGetLastErrorMsg()

class Geometry:
    def __init__(self, type=None, obj=None,
                 wkt=None,
                 thisown = None,
                 wkb=None,
                 gml=None,
                 srs=None):
        if obj is not None:
            self._o = obj
            if thisown is not None:
                self.thisown = thisown
            else:
                self.thisown = 0
        elif type is not None:
            self._o = _gdal.OGR_G_CreateGeometry( type )
            self.thisown = 1
        elif wkt:
            if srs:
                return CreateGeometryFromWkt(wkt, srs)
            else:
                return CreateGeometryFromWkt(wkt)
        elif wkb:
            if srs:
                return CreateGeometryFromWkb(wkb, srs)
            else:
                return CreateGeometryFromWkb(wkb)
        elif gml:
            return CreateGeometryFromGML(gml)
        else:
            raise OGRError, 'OGRGeometry may not be directly instantiated.'

    def __del__(self):
        if self.thisown:
            self.Destroy()
            
    def __str__(self):
        return self.ExportToWkt()

    def __copy__(self):
        return self.Clone()
    
    def Destroy( self ):
        if not self.thisown:
            print 'Destroy invoked on unowned geometry.' 
            
        _gdal.OGR_G_DestroyGeometry( self._o )
        self._o = None
        self.thisown = 0

    def ExportToWkb( self, byte_order = None ):
        if byte_order is None:
            byte_order = wkbXDR
        return _gdal.OGR_G_ExportToWkb( self._o, byte_order )

    def ExportToWkt( self):
        return _gdal.OGR_G_ExportToWkt( self._o )

    def ExportToGML( self):
        return _gdal.OGR_G_ExportToGML( self._o )

    def GetDimension( self ):
        return _gdal.OGR_G_GetDimension( self._o )
    
    def GetCoordinateDimension( self ):
        return _gdal.OGR_G_GetCoordinateDimension( self._o )
    
    def SetCoordinateDimension( self, new_dimension ):
        return _gdal.OGR_G_SetCoordinateDimension( self._o, new_dimension )
    
    def WkbSize( self ):
        return _gdal.OGR_G_WkbSize( self._o )

    def Clone( self ):
        _obj = _gdal.OGR_G_Clone( self._o )
        if _obj is not None:
            return Geometry( obj = _obj, thisown = 1 )
        else:
            return None

    def GetGeometryType( self ):
        return _gdal.OGR_G_GetGeometryType( self._o )
    
    def GetGeometryName( self ):
        return _gdal.OGR_G_GetGeometryName( self._o )
    
    def GetEnvelope( self ):
        extents = _gdal.ptrcreate( 'double', 0.0, 4 )
        _gdal.OGR_G_GetEnvelope( self._o, _gdal.ptrcast(extents,'OGREnvelope_p') )
        ret_extents = ( _gdal.ptrvalue( extents, 0 ),
                        _gdal.ptrvalue( extents, 1 ),
                        _gdal.ptrvalue( extents, 2 ),
                        _gdal.ptrvalue( extents, 3 ) )
        _gdal.ptrfree( extents )
        return ret_extents

    def FlattenTo2D( self ):
        _gdal.OGR_G_FlattenTo2D( self._o )
    
    def CloseRings( self ):
        _gdal.OGR_G_CloseRings( self._o )
    
    def AssignSpatialReference( self, srs ):
        _gdal.OGR_G_AssignSpatialReference( self._o, srs._o )
    
    def GetSpatialReference( self ):
        srs_o = _gdal.OGR_G_GetSpatialReference( self._o )
        if srs_o is not None and srs_o != 'NULL':
            return osr.SpatialReference( srs_o )
        else:
            return None

    def Transform( self, coord_tran ):
        return _gdal.OGR_G_Transform( self._o, coord_tran._o )
    
    def TransformTo( self, srs_out ):
        return _gdal.OGR_G_TransformTo( self._o, srs_out._o )
    
    def Intersect( self, other_geom ):
        return _gdal.OGR_G_Intersect( self._o, other_geom._o )

    def Equal( self, other_geom ):
        return _gdal.OGR_G_Equal( self._o, other_geom._o )

    def Disjoint( self, other_geom ):
        return _gdal.OGR_G_Disjoint( self._o, other_geom._o )

    def Touches( self, other_geom ):
        return _gdal.OGR_G_Touches( self._o, other_geom._o )

    def Crosses( self, other_geom ):
        return _gdal.OGR_G_Crosses( self._o, other_geom._o )

    def Within( self, other_geom ):
        return _gdal.OGR_G_Within( self._o, other_geom._o )

    def Contains( self, other_geom ):
        return _gdal.OGR_G_Contains( self._o, other_geom._o )

    def Overlaps( self, other_geom ):
        return _gdal.OGR_G_Overlaps( self._o, other_geom._o )

    def Empty( self ):
        return _gdal.OGR_G_Empty( self._o )

    def GetArea( self ):
        return _gdal.OGR_G_GetArea( self._o )

    def Centroid( self, pnt_geom = None ):

        if pnt_geom is None:
            pnt_geom = Geometry( type = wkbPoint )
            
        err_code = _gdal.OGR_G_Centroid( self._o, pnt_geom._o )
        if err_code != 0:
            raise OGRError, 'Error in Centroid operation.  ' + _gdal.CPLGetLastErrorMsg()

        return pnt_geom

    def GetPointCount( self):
        return _gdal.OGR_G_GetPointCount( self._o )

    def GetX( self, i = 0):
        return _gdal.OGR_G_GetX( self._o, i )

    def GetY( self, i = 0):
        return _gdal.OGR_G_GetY( self._o, i )

    def GetZ( self, i = 0 ):
        return _gdal.OGR_G_GetZ( self._o, i )

    def SetPoint( self, i, x, y, z = 0):
        return _gdal.OGR_G_SetPoint( self._o, i, x, y, z )

    def SetPoint_2D( self, i, x, y):
        return _gdal.OGR_G_SetPoint_2D( self._o, i, x, y )

    def AddPoint( self, x, y, z=0 ):
        return _gdal.OGR_G_AddPoint( self._o, x, y, z )

    def AddPoint_2D( self, x, y ):
        return _gdal.OGR_G_AddPoint_2D( self._o, x, y )

    def GetGeometryCount( self ):
        return _gdal.OGR_G_GetGeometryCount( self._o )

    def GetGeometryRef( self, i ):
        geom = _gdal.OGR_G_GetGeometryRef( self._o, i )
        if geom is not None:
            return Geometry( obj = geom )
        else:
            return None
        
    def AddGeometry( self, subgeom ):
        return _gdal.OGR_G_AddGeometry( self._o, subgeom._o )

    def AddGeometryDirectly( self, subgeom ):
        if not subgeom.thisown:
            print 'AddGeometryDirectly() with unowned geometry!'
        subgeom.thisown = 0
        return _gdal.OGR_G_AddGeometryDirectly( self._o, subgeom._o )

    def GetBoundary( self ):
        geom = _gdal.OGR_G_GetBoundary( self._o )
        if geom is not None and geom != 'NULL':
            return Geometry( obj = geom, thisown = 1 )
        else:
            return None
        
    def ConvexHull( self ):
        geom = _gdal.OGR_G_ConvexHull( self._o )
        if geom is not None and geom != 'NULL':
            return Geometry( obj = geom, thisown = 1 )
        else:
            return None
        
    def Buffer( self, distance, quadsects = 30 ):
        geom = _gdal.OGR_G_Buffer( self._o, distance, quadsects )
        if geom is not None and geom != 'NULL':
            return Geometry( obj = geom, thisown = 1 )
        else:
            return None
        
    def Intersection( self, other ):
        geom = _gdal.OGR_G_Intersection( self._o, other._o )
        if geom is not None and geom != 'NULL':
            return Geometry( obj = geom, thisown = 1 )
        else:
            return None
        
    def Union( self, other ):
        geom = _gdal.OGR_G_Union( self._o, other._o )
        if geom is not None and geom != 'NULL':
            return Geometry( obj = geom, thisown = 1 )
        else:
            return None
        
    def Difference( self, other ):
        geom = _gdal.OGR_G_Difference( self._o, other._o )
        if geom is not None and geom != 'NULL':
            return Geometry( obj = geom, thisown = 1 )
        else:
            return None
        
    def SymmetricDifference( self, other ):
        geom = _gdal.OGR_G_SymmetricDifference( self._o, other._o )
        if geom is not None and geom != 'NULL':
            return Geometry( obj = geom, thisown = 1 )
        else:
            return None
        
    def Distance( self, other ):
        return _gdal.OGR_G_Distance( self._o, other._o )

def BuildPolygonFromEdges( edges, bBestEffort=0, bAutoClose=0, Tolerance=0 ):
    _o = _gdal.OGRBuildPolygonFromEdges( edges._o, bBestEffort, bAutoClose,
                                         Tolerance )
    if _o is not None and _o != 'NULL':
        return Geometry( obj = _o, thisown = 1 )
    else:
        return None;
