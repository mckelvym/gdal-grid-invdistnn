/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.31
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

  public synchronized void delete() {
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

  public Geometry(int type, String wkt, int wkb, String wkb_buf, String gml) {
    this(ogrJNI.new_Geometry(type, wkt, wkb, wkb_buf, gml), true);
  }

  public int ExportToWkt(String[] argout) {
    return ogrJNI.Geometry_ExportToWkt(swigCPtr, this, argout);
  }

  public int ExportToWkb(char[][] nLen, int byte_order) {
    return ogrJNI.Geometry_ExportToWkb(swigCPtr, this, nLen, byte_order);
  }

  public String ExportToGML() {
    return ogrJNI.Geometry_ExportToGML(swigCPtr, this);
  }

  public void AddPoint(double x, double y, double z) {
    ogrJNI.Geometry_AddPoint(swigCPtr, this, x, y, z);
  }

  public int AddGeometryDirectly(Geometry other) {
    return ogrJNI.Geometry_AddGeometryDirectly(swigCPtr, this, Geometry.getCPtrAndDisown(other), other);
  }

  public int AddGeometry(Geometry other) {
    return ogrJNI.Geometry_AddGeometry(swigCPtr, this, Geometry.getCPtr(other), other);
  }

  public Geometry Clone() {
    long cPtr = ogrJNI.Geometry_Clone(swigCPtr, this);
    return (cPtr == 0) ? null : new Geometry(cPtr, true);
  }

  public int GetGeometryType() {
    return ogrJNI.Geometry_GetGeometryType(swigCPtr, this);
  }

  public String GetGeometryName() {
    return ogrJNI.Geometry_GetGeometryName(swigCPtr, this);
  }

  public double GetArea() {
    return ogrJNI.Geometry_GetArea(swigCPtr, this);
  }

  public int GetPointCount() {
    return ogrJNI.Geometry_GetPointCount(swigCPtr, this);
  }

  public double GetX(int point) {
    return ogrJNI.Geometry_GetX(swigCPtr, this, point);
  }

  public double GetY(int point) {
    return ogrJNI.Geometry_GetY(swigCPtr, this, point);
  }

  public double GetZ(int point) {
    return ogrJNI.Geometry_GetZ(swigCPtr, this, point);
  }

  public int GetGeometryCount() {
    return ogrJNI.Geometry_GetGeometryCount(swigCPtr, this);
  }

  public void SetPoint(int point, double x, double y, double z) {
    ogrJNI.Geometry_SetPoint(swigCPtr, this, point, x, y, z);
  }

  public Geometry GetGeometryRef(int geom) {
    long cPtr = ogrJNI.Geometry_GetGeometryRef(swigCPtr, this, geom);
    return (cPtr == 0) ? null : new Geometry(cPtr, false);
  }

  public Geometry GetBoundary() {
    long cPtr = ogrJNI.Geometry_GetBoundary(swigCPtr, this);
    return (cPtr == 0) ? null : new Geometry(cPtr, true);
  }

  public Geometry ConvexHull() {
    long cPtr = ogrJNI.Geometry_ConvexHull(swigCPtr, this);
    return (cPtr == 0) ? null : new Geometry(cPtr, true);
  }

  public Geometry Buffer(double distance, int quadsecs) {
    long cPtr = ogrJNI.Geometry_Buffer(swigCPtr, this, distance, quadsecs);
    return (cPtr == 0) ? null : new Geometry(cPtr, true);
  }

  public Geometry Intersection(Geometry other) {
    long cPtr = ogrJNI.Geometry_Intersection(swigCPtr, this, Geometry.getCPtr(other), other);
    return (cPtr == 0) ? null : new Geometry(cPtr, true);
  }

  public Geometry Union(Geometry other) {
    long cPtr = ogrJNI.Geometry_Union(swigCPtr, this, Geometry.getCPtr(other), other);
    return (cPtr == 0) ? null : new Geometry(cPtr, true);
  }

  public Geometry Difference(Geometry other) {
    long cPtr = ogrJNI.Geometry_Difference(swigCPtr, this, Geometry.getCPtr(other), other);
    return (cPtr == 0) ? null : new Geometry(cPtr, true);
  }

  public Geometry SymmetricDifference(Geometry other) {
    long cPtr = ogrJNI.Geometry_SymmetricDifference(swigCPtr, this, Geometry.getCPtr(other), other);
    return (cPtr == 0) ? null : new Geometry(cPtr, true);
  }

  public double Distance(Geometry other) {
    return ogrJNI.Geometry_Distance(swigCPtr, this, Geometry.getCPtr(other), other);
  }

  public void Empty() {
    ogrJNI.Geometry_Empty(swigCPtr, this);
  }

  public boolean Intersect(Geometry other) {
    return ogrJNI.Geometry_Intersect(swigCPtr, this, Geometry.getCPtr(other), other);
  }

  public boolean Equal(Geometry other) {
    return ogrJNI.Geometry_Equal(swigCPtr, this, Geometry.getCPtr(other), other);
  }

  public boolean Disjoint(Geometry other) {
    return ogrJNI.Geometry_Disjoint(swigCPtr, this, Geometry.getCPtr(other), other);
  }

  public boolean Touches(Geometry other) {
    return ogrJNI.Geometry_Touches(swigCPtr, this, Geometry.getCPtr(other), other);
  }

  public boolean Crosses(Geometry other) {
    return ogrJNI.Geometry_Crosses(swigCPtr, this, Geometry.getCPtr(other), other);
  }

  public boolean Within(Geometry other) {
    return ogrJNI.Geometry_Within(swigCPtr, this, Geometry.getCPtr(other), other);
  }

  public boolean Contains(Geometry other) {
    return ogrJNI.Geometry_Contains(swigCPtr, this, Geometry.getCPtr(other), other);
  }

  public boolean Overlaps(Geometry other) {
    return ogrJNI.Geometry_Overlaps(swigCPtr, this, Geometry.getCPtr(other), other);
  }

  public int TransformTo(SpatialReference reference) {
    return ogrJNI.Geometry_TransformTo(swigCPtr, this, SpatialReference.getCPtr(reference), reference);
  }

  public int Transform(CoordinateTransformation trans) {
    return ogrJNI.Geometry_Transform(swigCPtr, this, CoordinateTransformation.getCPtr(trans), trans);
  }

  public SpatialReference GetSpatialReference() {
    long cPtr = ogrJNI.Geometry_GetSpatialReference(swigCPtr, this);
    return (cPtr == 0) ? null : new SpatialReference(cPtr, false);
  }

  public void AssignSpatialReference(SpatialReference reference) {
    ogrJNI.Geometry_AssignSpatialReference(swigCPtr, this, SpatialReference.getCPtr(reference), reference);
  }

  public void CloseRings() {
    ogrJNI.Geometry_CloseRings(swigCPtr, this);
  }

  public void FlattenTo2D() {
    ogrJNI.Geometry_FlattenTo2D(swigCPtr, this);
  }

  public void GetEnvelope(double[] argout) {
    ogrJNI.Geometry_GetEnvelope(swigCPtr, this, argout);
  }

  public Geometry Centroid() {
    long cPtr = ogrJNI.Geometry_Centroid(swigCPtr, this);
    return (cPtr == 0) ? null : new Geometry(cPtr, true);
  }

  public int WkbSize() {
    return ogrJNI.Geometry_WkbSize(swigCPtr, this);
  }

  public int GetCoordinateDimension() {
    return ogrJNI.Geometry_GetCoordinateDimension(swigCPtr, this);
  }

  public void SetCoordinateDimension(int dimension) {
    ogrJNI.Geometry_SetCoordinateDimension(swigCPtr, this, dimension);
  }

  public int GetDimension() {
    return ogrJNI.Geometry_GetDimension(swigCPtr, this);
  }

}
