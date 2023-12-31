/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.31
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.gdal.osr;

public class CoordinateTransformation {
  private long swigCPtr;
  protected boolean swigCMemOwn;

  public CoordinateTransformation(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }
  
  public static long getCPtr(CoordinateTransformation obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if(swigCPtr != 0 && swigCMemOwn) {
      swigCMemOwn = false;
      osrJNI.delete_CoordinateTransformation(swigCPtr);
    }
    swigCPtr = 0;
  }

  protected static long getCPtrAndDisown(CoordinateTransformation obj) {
    if (obj != null) obj.swigCMemOwn= false;
    return getCPtr(obj);
  }

  public CoordinateTransformation(SpatialReference src, SpatialReference dst) {
    this(osrJNI.new_CoordinateTransformation(SpatialReference.getCPtr(src), src, SpatialReference.getCPtr(dst), dst), true);
  }

  public void TransformPoint(double[] inout) {
    osrJNI.CoordinateTransformation_TransformPoint__SWIG_0(swigCPtr, this, inout);
  }

  public void TransformPoint(double[] argout, double x, double y, double z) {
    osrJNI.CoordinateTransformation_TransformPoint__SWIG_1(swigCPtr, this, argout, x, y, z);
  }

}
