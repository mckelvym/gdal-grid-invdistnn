/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.25
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */


using System;
using System.Runtime.InteropServices;

public class CoordinateTransformation : IDisposable {
  private HandleRef swigCPtr;
  protected bool swigCMemOwn;

  internal CoordinateTransformation(IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new HandleRef(this, cPtr);
  }

  internal static HandleRef getCPtr(CoordinateTransformation obj) {
    return (obj == null) ? new HandleRef(null, IntPtr.Zero) : obj.swigCPtr;
  }

  ~CoordinateTransformation() {
    Dispose();
  }

  public virtual void Dispose() {
    if(swigCPtr.Handle != IntPtr.Zero && swigCMemOwn) {
      swigCMemOwn = false;
      osrPINVOKE.delete_CoordinateTransformation(swigCPtr);
    }
    swigCPtr = new HandleRef(null, IntPtr.Zero);
    GC.SuppressFinalize(this);
  }

  public CoordinateTransformation(SpatialReference src, SpatialReference dst) : this(osrPINVOKE.new_CoordinateTransformation(SpatialReference.getCPtr(src), SpatialReference.getCPtr(dst)), true) {
  }

  public void TransformPoint(SWIGTYPE_p_double inout) {
    osrPINVOKE.CoordinateTransformation_TransformPoint__SWIG_0(swigCPtr, SWIGTYPE_p_double.getCPtr(inout));
  }

  public void TransformPoint(double x, double y, double z) {
    osrPINVOKE.CoordinateTransformation_TransformPoint__SWIG_1(swigCPtr, x, y, z);
  }

}
