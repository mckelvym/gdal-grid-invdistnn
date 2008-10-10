/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.28
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.gdal.ogr;

import org.gdal.osr.SpatialReference;
import org.gdal.osr.CoordinateTransformation;

class ogrJNI {
  public final static native int wkb25Bit_get();
  public final static native int wkbUnknown_get();
  public final static native int wkbPoint_get();
  public final static native int wkbLineString_get();
  public final static native int wkbPolygon_get();
  public final static native int wkbMultiPoint_get();
  public final static native int wkbMultiLineString_get();
  public final static native int wkbMultiPolygon_get();
  public final static native int wkbGeometryCollection_get();
  public final static native int wkbNone_get();
  public final static native int wkbLinearRing_get();
  public final static native int wkbPoint25D_get();
  public final static native int wkbLineString25D_get();
  public final static native int wkbPolygon25D_get();
  public final static native int wkbMultiPoint25D_get();
  public final static native int wkbMultiLineString25D_get();
  public final static native int wkbMultiPolygon25D_get();
  public final static native int wkbGeometryCollection25D_get();
  public final static native int OFTInteger_get();
  public final static native int OFTIntegerList_get();
  public final static native int OFTReal_get();
  public final static native int OFTRealList_get();
  public final static native int OFTString_get();
  public final static native int OFTStringList_get();
  public final static native int OFTWideString_get();
  public final static native int OFTWideStringList_get();
  public final static native int OFTBinary_get();
  public final static native int OFTDate_get();
  public final static native int OFTTime_get();
  public final static native int OFTDateTime_get();
  public final static native int OJUndefined_get();
  public final static native int OJLeft_get();
  public final static native int OJRight_get();
  public final static native int wkbXDR_get();
  public final static native int wkbNDR_get();
  public final static native String OLCRandomRead_get();
  public final static native String OLCSequentialWrite_get();
  public final static native String OLCRandomWrite_get();
  public final static native String OLCFastSpatialFilter_get();
  public final static native String OLCFastFeatureCount_get();
  public final static native String OLCFastGetExtent_get();
  public final static native String OLCCreateField_get();
  public final static native String OLCTransactions_get();
  public final static native String OLCDeleteFeature_get();
  public final static native String OLCFastSetNextByIndex_get();
  public final static native String ODsCCreateLayer_get();
  public final static native String ODsCDeleteLayer_get();
  public final static native String ODrCCreateDataSource_get();
  public final static native String ODrCDeleteDataSource_get();

  private static boolean available = false;

  static {
    try {
      System.loadLibrary("ogrjni");
      available = true;
    } catch (UnsatisfiedLinkError e) {
      available = false;
      System.err.println("Native library load failed.");
      System.err.println(e);
    }
  }
  
  public static boolean isAvailable() {
    return available;
  }

  public final static native String Driver_name_get(long jarg1);
  public final static native long Driver_CreateDataSource(long jarg1, String jarg2, java.util.Vector jarg3);
  public final static native long Driver_CopyDataSource(long jarg1, long jarg2, String jarg3, java.util.Vector jarg4);
  public final static native long Driver_Open(long jarg1, String jarg2, int jarg3);
  public final static native int Driver_DeleteDataSource(long jarg1, String jarg2);
  public final static native boolean Driver_TestCapability(long jarg1, String jarg2);
  public final static native String Driver_GetName(long jarg1);
  public final static native String DataSource_name_get(long jarg1);
  public final static native void delete_DataSource(long jarg1);
  public final static native int DataSource_GetRefCount(long jarg1);
  public final static native int DataSource_GetSummaryRefCount(long jarg1);
  public final static native int DataSource_GetLayerCount(long jarg1);
  public final static native long DataSource_GetDriver(long jarg1);
  public final static native String DataSource_GetName(long jarg1);
  public final static native int DataSource_DeleteLayer(long jarg1, int jarg2);
  public final static native long DataSource_CreateLayer(long jarg1, String jarg2, long jarg3, int jarg4, java.util.Vector jarg5);
  public final static native long DataSource_CopyLayer(long jarg1, long jarg2, String jarg3, java.util.Vector jarg4);
  public final static native long DataSource_GetLayerByIndex(long jarg1, int jarg2);
  public final static native long DataSource_GetLayerByName(long jarg1, String jarg2);
  public final static native boolean DataSource_TestCapability(long jarg1, String jarg2);
  public final static native long DataSource_ExecuteSQL(long jarg1, String jarg2, long jarg3, String jarg4);
  public final static native void DataSource_ReleaseResultSet(long jarg1, long jarg2);
  public final static native int Layer_GetRefCount(long jarg1);
  public final static native void Layer_SetSpatialFilter(long jarg1, long jarg2);
  public final static native void Layer_SetSpatialFilterRect(long jarg1, double jarg2, double jarg3, double jarg4, double jarg5);
  public final static native long Layer_GetSpatialFilter(long jarg1);
  public final static native int Layer_SetAttributeFilter(long jarg1, String jarg2);
  public final static native void Layer_ResetReading(long jarg1);
  public final static native String Layer_GetName(long jarg1);
  public final static native long Layer_GetFeature(long jarg1, int jarg2);
  public final static native long Layer_GetNextFeature(long jarg1);
  public final static native int Layer_SetNextByIndex(long jarg1, int jarg2);
  public final static native int Layer_SetFeature(long jarg1, long jarg2);
  public final static native int Layer_CreateFeature(long jarg1, long jarg2);
  public final static native int Layer_DeleteFeature(long jarg1, int jarg2);
  public final static native int Layer_SyncToDisk(long jarg1);
  public final static native long Layer_GetLayerDefn(long jarg1);
  public final static native int Layer_GetFeatureCount(long jarg1, int jarg2);
  public final static native int Layer_GetExtent(long jarg1, double[] jarg2, int jarg3);
  public final static native boolean Layer_TestCapability(long jarg1, String jarg2);
  public final static native int Layer_CreateField(long jarg1, long jarg2, int jarg3);
  public final static native int Layer_StartTransaction(long jarg1);
  public final static native int Layer_CommitTransaction(long jarg1);
  public final static native int Layer_RollbackTransaction(long jarg1);
  public final static native long Layer_GetSpatialRef(long jarg1);
  public final static native long Layer_GetFeatureRead(long jarg1);
  public final static native void delete_Feature(long jarg1);
  public final static native long new_Feature(long jarg1);
  public final static native long Feature_GetDefnRef(long jarg1);
  public final static native int Feature_SetGeometry(long jarg1, long jarg2);
  public final static native int Feature_SetGeometryDirectly(long jarg1, long jarg2);
  public final static native long Feature_GetGeometryRef(long jarg1);
  public final static native long Feature_Clone(long jarg1);
  public final static native boolean Feature_Equal(long jarg1, long jarg2);
  public final static native int Feature_GetFieldCount(long jarg1);
  public final static native long Feature_GetFieldDefnRef__SWIG_0(long jarg1, int jarg2);
  public final static native long Feature_GetFieldDefnRef__SWIG_1(long jarg1, String jarg2);
  public final static native String Feature_GetFieldAsString__SWIG_0(long jarg1, int jarg2);
  public final static native String Feature_GetFieldAsString__SWIG_1(long jarg1, String jarg2);
  public final static native int Feature_GetFieldAsInteger__SWIG_0(long jarg1, int jarg2);
  public final static native int Feature_GetFieldAsInteger__SWIG_1(long jarg1, String jarg2);
  public final static native double Feature_GetFieldAsDouble__SWIG_0(long jarg1, int jarg2);
  public final static native double Feature_GetFieldAsDouble__SWIG_1(long jarg1, String jarg2);
  public final static native boolean Feature_IsFieldSet__SWIG_0(long jarg1, int jarg2);
  public final static native boolean Feature_IsFieldSet__SWIG_1(long jarg1, String jarg2);
  public final static native int Feature_GetFieldIndex(long jarg1, String jarg2);
  public final static native int Feature_GetFID(long jarg1);
  public final static native int Feature_SetFID(long jarg1, int jarg2);
  public final static native void Feature_DumpReadable(long jarg1);
  public final static native void Feature_UnsetField__SWIG_0(long jarg1, int jarg2);
  public final static native void Feature_UnsetField__SWIG_1(long jarg1, String jarg2);
  public final static native void Feature_SetField__SWIG_0(long jarg1, int jarg2, String jarg3);
  public final static native void Feature_SetField__SWIG_1(long jarg1, String jarg2, String jarg3);
  public final static native void Feature_SetField__SWIG_2(long jarg1, int jarg2, int jarg3);
  public final static native void Feature_SetField__SWIG_3(long jarg1, String jarg2, int jarg3);
  public final static native void Feature_SetField__SWIG_4(long jarg1, int jarg2, double jarg3);
  public final static native void Feature_SetField__SWIG_5(long jarg1, String jarg2, double jarg3);
  public final static native void Feature_SetField__SWIG_6(long jarg1, int jarg2, int jarg3, int jarg4, int jarg5, int jarg6, int jarg7, int jarg8, int jarg9);
  public final static native void Feature_SetField__SWIG_7(long jarg1, String jarg2, int jarg3, int jarg4, int jarg5, int jarg6, int jarg7, int jarg8, int jarg9);
  public final static native int Feature_SetFrom(long jarg1, long jarg2, int jarg3);
  public final static native String Feature_GetStyleString(long jarg1);
  public final static native void Feature_SetStyleString(long jarg1, String jarg2);
  public final static native int Feature_GetFieldType__SWIG_0(long jarg1, int jarg2);
  public final static native int Feature_GetFieldType__SWIG_1(long jarg1, String jarg2);
  public final static native void delete_FeatureDefn(long jarg1);
  public final static native long new_FeatureDefn(String jarg1);
  public final static native String FeatureDefn_GetName(long jarg1);
  public final static native int FeatureDefn_GetFieldCount(long jarg1);
  public final static native long FeatureDefn_GetFieldDefn(long jarg1, int jarg2);
  public final static native int FeatureDefn_GetFieldIndex(long jarg1, String jarg2);
  public final static native void FeatureDefn_AddFieldDefn(long jarg1, long jarg2);
  public final static native int FeatureDefn_GetGeomType(long jarg1);
  public final static native void FeatureDefn_SetGeomType(long jarg1, int jarg2);
  public final static native int FeatureDefn_GetReferenceCount(long jarg1);
  public final static native void delete_FieldDefn(long jarg1);
  public final static native long new_FieldDefn(String jarg1, int jarg2);
  public final static native String FieldDefn_GetName(long jarg1);
  public final static native String FieldDefn_GetNameRef(long jarg1);
  public final static native void FieldDefn_SetName(long jarg1, String jarg2);
  public final static native int FieldDefn_GetFieldType(long jarg1);
  public final static native void FieldDefn_SetType(long jarg1, int jarg2);
  public final static native int FieldDefn_GetJustify(long jarg1);
  public final static native void FieldDefn_SetJustify(long jarg1, int jarg2);
  public final static native int FieldDefn_GetWidth(long jarg1);
  public final static native void FieldDefn_SetWidth(long jarg1, int jarg2);
  public final static native int FieldDefn_GetPrecision(long jarg1);
  public final static native void FieldDefn_SetPrecision(long jarg1, int jarg2);
  public final static native String FieldDefn_GetFieldTypeName(long jarg1, int jarg2);
  public final static native long CreateGeometryFromWkb(byte[] jarg1, long jarg3);
  public final static native long CreateGeometryFromWkt(String jarg1, long jarg2);
  public final static native long CreateGeometryFromGML(String jarg1);
  public final static native void delete_Geometry(long jarg1);
  public final static native int Geometry_ExportToWkt(long jarg1, String[] jarg2);
  public final static native String Geometry_ExportToGML(long jarg1);
  public final static native void Geometry_AddPoint(long jarg1, double jarg2, double jarg3, double jarg4);
  public final static native int Geometry_AddGeometryDirectly(long jarg1, long jarg2);
  public final static native int Geometry_AddGeometry(long jarg1, long jarg2);
  public final static native long Geometry_Clone(long jarg1);
  public final static native int Geometry_GetGeometryType(long jarg1);
  public final static native String Geometry_GetGeometryName(long jarg1);
  public final static native double Geometry_GetArea(long jarg1);
  public final static native int Geometry_GetPointCount(long jarg1);
  public final static native double Geometry_GetX(long jarg1, int jarg2);
  public final static native double Geometry_GetY(long jarg1, int jarg2);
  public final static native double Geometry_GetZ(long jarg1, int jarg2);
  public final static native int Geometry_GetGeometryCount(long jarg1);
  public final static native void Geometry_SetPoint(long jarg1, int jarg2, double jarg3, double jarg4, double jarg5);
  public final static native long Geometry_GetGeometryRef(long jarg1, int jarg2);
  public final static native long Geometry_GetBoundary(long jarg1);
  public final static native long Geometry_ConvexHull(long jarg1);
  public final static native long Geometry_Buffer(long jarg1, double jarg2, int jarg3);
  public final static native long Geometry_Intersection(long jarg1, long jarg2);
  public final static native long Geometry_Union(long jarg1, long jarg2);
  public final static native long Geometry_Difference(long jarg1, long jarg2);
  public final static native long Geometry_SymmetricDifference(long jarg1, long jarg2);
  public final static native double Geometry_Distance(long jarg1, long jarg2);
  public final static native void Geometry_Empty(long jarg1);
  public final static native boolean Geometry_Intersect(long jarg1, long jarg2);
  public final static native boolean Geometry_Equal(long jarg1, long jarg2);
  public final static native boolean Geometry_Disjoint(long jarg1, long jarg2);
  public final static native boolean Geometry_Touches(long jarg1, long jarg2);
  public final static native boolean Geometry_Crosses(long jarg1, long jarg2);
  public final static native boolean Geometry_Within(long jarg1, long jarg2);
  public final static native boolean Geometry_Contains(long jarg1, long jarg2);
  public final static native boolean Geometry_Overlaps(long jarg1, long jarg2);
  public final static native int Geometry_TransformTo(long jarg1, long jarg2);
  public final static native int Geometry_Transform(long jarg1, long jarg2);
  public final static native long Geometry_GetSpatialReference(long jarg1);
  public final static native void Geometry_AssignSpatialReference(long jarg1, long jarg2);
  public final static native void Geometry_CloseRings(long jarg1);
  public final static native void Geometry_FlattenTo2D(long jarg1);
  public final static native void Geometry_GetEnvelope(long jarg1, double[] jarg2);
  public final static native int Geometry_WkbSize(long jarg1);
  public final static native int Geometry_GetCoordinateDimension(long jarg1);
  public final static native void Geometry_SetCoordinateDimension(long jarg1, int jarg2);
  public final static native int Geometry_GetDimension(long jarg1);
  public final static native long new_Geometry(int jarg1, String jarg2, byte[] jarg3, String jarg5);
  public final static native int Geometry_ExportToWkb(long jarg1, byte[] jarg2, int jarg4);
  public final static native long Geometry_Centroid(long jarg1);
  public final static native int GetDriverCount();
  public final static native int GetOpenDSCount();
  public final static native int SetGenerate_DB2_V72_BYTE_ORDER(int jarg1);
  public final static native void RegisterAll();
  public final static native long GetOpenDS(int jarg1);
  public final static native long Open(String jarg1, int jarg2);
  public final static native long OpenShared(String jarg1, int jarg2);
  public final static native long GetDriverByName(String jarg1);
  public final static native long GetDriver(int jarg1);
  public final static native int sum(int jarg1, int jarg2);
}
