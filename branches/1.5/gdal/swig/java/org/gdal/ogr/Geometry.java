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

public class Geometry {
  private long swigCPtr;
  protected boolean swigCMemOwn;

  protected Geometry(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(Geometry obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public void delete() {
    if(swigCPtr != 0 && swigCMemOwn) {
      swigCMemOwn = false;
      ogrJNI.delete_Geometry(swigCPtr);
    }
    swigCPtr = 0;
  }

  protected static long getCPtrAndDisown(Geometry obj) {
    if (obj != null) obj.swigCMemOwn= false;
    return getCPtr(obj);
  }

  public int ExportToWkt(String[] argout) {
    return ogrJNI.Geometry_ExportToWkt(swigCPtr, argout);
  }

  public String ExportToGML() {
    return ogrJNI.Geometry_ExportToGML(swigCPtr);
  }

  public void AddPoint(double x, double y, double z) {
    ogrJNI.Geometry_AddPoint(swigCPtr, x, y, z);
  }

  public int AddGeometryDirectly(Geometry other) {
    return ogrJNI.Geometry_AddGeometryDirectly(swigCPtr, Geometry.getCPtrAndDisown(other));
  }

  public int AddGeometry(Geometry other) {
    return ogrJNI.Geometry_AddGeometry(swigCPtr, Geometry.getCPtr(other));
  }

  public Geometry Clone() {
    long cPtr = ogrJNI.Geometry_Clone(swigCPtr);
    return (cPtr == 0) ? null : new Geometry(cPtr, true);
  }

  public int GetGeometryType() {
    return ogrJNI.Geometry_GetGeometryType(swigCPtr);
  }

  public String GetGeometryName() {
    return ogrJNI.Geometry_GetGeometryName(swigCPtr);
  }

  public double GetArea() {
    return ogrJNI.Geometry_GetArea(swigCPtr);
  }

  public int GetPointCount() {
    return ogrJNI.Geometry_GetPointCount(swigCPtr);
  }

  public double GetX(int point) {
    return ogrJNI.Geometry_GetX(swigCPtr, point);
  }

  public double GetY(int point) {
    return ogrJNI.Geometry_GetY(swigCPtr, point);
  }

  public double GetZ(int point) {
    return ogrJNI.Geometry_GetZ(swigCPtr, point);
  }

  public int GetGeometryCount() {
    return ogrJNI.Geometry_GetGeometryCount(swigCPtr);
  }

  public void SetPoint(int point, double x, double y, double z) {
    ogrJNI.Geometry_SetPoint(swigCPtr, point, x, y, z);
  }

  public Geometry GetGeometryRef(int geom) {
    long cPtr = ogrJNI.Geometry_GetGeometryRef(swigCPtr, geom);
    return (cPtr == 0) ? null : new Geometry(cPtr, false);
  }

  public Geometry GetBoundary() {
    long cPtr = ogrJNI.Geometry_GetBoundary(swigCPtr);
    return (cPtr == 0) ? null : new Geometry(cPtr, true);
  }

  public Geometry ConvexHull() {
    long cPtr = ogrJNI.Geometry_ConvexHull(swigCPtr);
    return (cPtr == 0) ? null : new Geometry(cPtr, true);
  }

  public Geometry Buffer(double distance, int quadsecs) {
    long cPtr = ogrJNI.Geometry_Buffer(swigCPtr, distance, quadsecs);
    return (cPtr == 0) ? null : new Geometry(cPtr, true);
  }

  public Geometry Intersection(Geometry other) {
    long cPtr = ogrJNI.Geometry_Intersection(swigCPtr, Geometry.getCPtr(other));
    return (cPtr == 0) ? null : new Geometry(cPtr, true);
  }

  public Geometry Union(Geometry other) {
    long cPtr = ogrJNI.Geometry_Union(swigCPtr, Geometry.getCPtr(other));
    return (cPtr == 0) ? null : new Geometry(cPtr, true);
  }

  public Geometry Difference(Geometry other) {
    long cPtr = ogrJNI.Geometry_Difference(swigCPtr, Geometry.getCPtr(other));
    return (cPtr == 0) ? null : new Geometry(cPtr, true);
  }

  public Geometry SymmetricDifference(Geometry other) {
    long cPtr = ogrJNI.Geometry_SymmetricDifference(swigCPtr, Geometry.getCPtr(other));
    return (cPtr == 0) ? null : new Geometry(cPtr, true);
  }

  public double Distance(Geometry other) {
    return ogrJNI.Geometry_Distance(swigCPtr, Geometry.getCPtr(other));
  }

  public void Empty() {
    ogrJNI.Geometry_Empty(swigCPtr);
  }

  public boolean Intersect(Geometry other) {
    return ogrJNI.Geometry_Intersect(swigCPtr, Geometry.getCPtr(other));
  }

  public boolean Equal(Geometry other) {
    return ogrJNI.Geometry_Equal(swigCPtr, Geometry.getCPtr(other));
  }

  public boolean Disjoint(Geometry other) {
    return ogrJNI.Geometry_Disjoint(swigCPtr, Geometry.getCPtr(other));
  }

  public boolean Touches(Geometry other) {
    return ogrJNI.Geometry_Touches(swigCPtr, Geometry.getCPtr(other));
  }

  public boolean Crosses(Geometry other) {
    return ogrJNI.Geometry_Crosses(swigCPtr, Geometry.getCPtr(other));
  }

  public boolean Within(Geometry other) {
    return ogrJNI.Geometry_Within(swigCPtr, Geometry.getCPtr(other));
  }

  public boolean Contains(Geometry other) {
    return ogrJNI.Geometry_Contains(swigCPtr, Geometry.getCPtr(other));
  }

  public boolean Overlaps(Geometry other) {
    return ogrJNI.Geometry_Overlaps(swigCPtr, Geometry.getCPtr(other));
  }

  public int TransformTo(SpatialReference reference) {
    return ogrJNI.Geometry_TransformTo(swigCPtr, SpatialReference.getCPtr(reference));
  }

  public int Transform(CoordinateTransformation trans) {
    return ogrJNI.Geometry_Transform(swigCPtr, CoordinateTransformation.getCPtr(trans));
  }

  public SpatialReference GetSpatialReference() {
    long cPtr = ogrJNI.Geometry_GetSpatialReference(swigCPtr);
    return (cPtr == 0) ? null : new SpatialReference(cPtr, false);
  }

  public void AssignSpatialReference(SpatialReference reference) {
    ogrJNI.Geometry_AssignSpatialReference(swigCPtr, SpatialReference.getCPtr(reference));
  }

  public void CloseRings() {
    ogrJNI.Geometry_CloseRings(swigCPtr);
  }

  public void FlattenTo2D() {
    ogrJNI.Geometry_FlattenTo2D(swigCPtr);
  }

  public void GetEnvelope(double[] argout) {
    ogrJNI.Geometry_GetEnvelope(swigCPtr, argout);
  }

  public int WkbSize() {
    return ogrJNI.Geometry_WkbSize(swigCPtr);
  }

  public int GetCoordinateDimension() {
    return ogrJNI.Geometry_GetCoordinateDimension(swigCPtr);
  }

  public void SetCoordinateDimension(int dimension) {
    ogrJNI.Geometry_SetCoordinateDimension(swigCPtr, dimension);
  }

  public int GetDimension() {
    return ogrJNI.Geometry_GetDimension(swigCPtr);
  }

  public Geometry(int type, String wkt, byte[] nLen, String gml) {
    this(ogrJNI.new_Geometry(type, wkt, nLen, gml), true);
  }

  public int ExportToWkb(byte[] nLen, int byte_order) {
    return ogrJNI.Geometry_ExportToWkb(swigCPtr, nLen, byte_order);
  }

  public Geometry Centroid() {
    long cPtr = ogrJNI.Geometry_Centroid(swigCPtr);
    return (cPtr == 0) ? null : new Geometry(cPtr, true);
  }

}
