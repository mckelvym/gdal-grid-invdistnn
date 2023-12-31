/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.24
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

namespace OGR {

using System;

public class DataSource : IDisposable {
  private IntPtr swigCPtr;
  protected bool swigCMemOwn;

  internal DataSource(IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  internal static IntPtr getCPtr(DataSource obj) {
    return (obj == null) ? IntPtr.Zero : obj.swigCPtr;
  }

  protected DataSource() : this(IntPtr.Zero, false) {
  }

  public virtual void Dispose() {
    if(swigCPtr != IntPtr.Zero && swigCMemOwn) {
      swigCMemOwn = false;
      throw new MethodAccessException("C++ destructor does not have public access");
    }
    swigCPtr = IntPtr.Zero;
    GC.SuppressFinalize(this);
  }

  public string name {
    get {
      return ogrPINVOKE.get_DataSource_name(swigCPtr);
    } 
  }

  public void Destroy() {
    ogrPINVOKE.DataSource_Destroy(swigCPtr);
  }

  public void Release() {
    ogrPINVOKE.DataSource_Release(swigCPtr);
  }

  public int Reference() {
    return ogrPINVOKE.DataSource_Reference(swigCPtr);
  }

  public int Dereference() {
    return ogrPINVOKE.DataSource_Dereference(swigCPtr);
  }

  public int GetRefCount() {
    return ogrPINVOKE.DataSource_GetRefCount(swigCPtr);
  }

  public int GetSummaryRefCount() {
    return ogrPINVOKE.DataSource_GetSummaryRefCount(swigCPtr);
  }

  public int GetLayerCount() {
    return ogrPINVOKE.DataSource_GetLayerCount(swigCPtr);
  }

  public string GetName() {
    return ogrPINVOKE.DataSource_GetName(swigCPtr);
  }

  public int DeleteLayer(int index) {
    return ogrPINVOKE.DataSource_DeleteLayer(swigCPtr, index);
  }

  public Layer CreateLayer(string name, SWIGTYPE_p_OSRSpatialReferenceShadow reference, int geom_type, SWIGTYPE_p_p_char options) {
    IntPtr cPtr = ogrPINVOKE.DataSource_CreateLayer(swigCPtr, name, SWIGTYPE_p_OSRSpatialReferenceShadow.getCPtr(reference), geom_type, SWIGTYPE_p_p_char.getCPtr(options));
    return (cPtr == IntPtr.Zero) ? null : new Layer(cPtr, false);
  }

  public Layer CopyLayer(Layer src_layer, string new_name, SWIGTYPE_p_p_char options) {
    IntPtr cPtr = ogrPINVOKE.DataSource_CopyLayer(swigCPtr, Layer.getCPtr(src_layer), new_name, SWIGTYPE_p_p_char.getCPtr(options));
    return (cPtr == IntPtr.Zero) ? null : new Layer(cPtr, false);
  }

  public Layer GetLayerByIndex(int index) {
    IntPtr cPtr = ogrPINVOKE.DataSource_GetLayerByIndex(swigCPtr, index);
    return (cPtr == IntPtr.Zero) ? null : new Layer(cPtr, false);
  }

  public Layer GetLayerByName(string layer_name) {
    IntPtr cPtr = ogrPINVOKE.DataSource_GetLayerByName(swigCPtr, layer_name);
    return (cPtr == IntPtr.Zero) ? null : new Layer(cPtr, false);
  }

  public int TestCapability(string cap) {
    return ogrPINVOKE.DataSource_TestCapability(swigCPtr, cap);
  }

  public Layer ExecuteSQL(string statement, Geometry geom, string dialect) {
    IntPtr cPtr = ogrPINVOKE.DataSource_ExecuteSQL(swigCPtr, statement, Geometry.getCPtr(geom), dialect);
    return (cPtr == IntPtr.Zero) ? null : new Layer(cPtr, true);
  }

  public void ReleaseResultSet(Layer layer) {
    ogrPINVOKE.DataSource_ReleaseResultSet(swigCPtr, Layer.getCPtr(layer));
  }

}

}
