/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.25
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */


using System;
using System.Runtime.InteropServices;

public class FieldDefn : IDisposable {
  private HandleRef swigCPtr;
  protected bool swigCMemOwn;

  internal FieldDefn(IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = new HandleRef(this, cPtr);
  }

  internal static HandleRef getCPtr(FieldDefn obj) {
    return (obj == null) ? new HandleRef(null, IntPtr.Zero) : obj.swigCPtr;
  }

  public virtual void Dispose() {
    if(swigCPtr.Handle != IntPtr.Zero && swigCMemOwn) {
      swigCMemOwn = false;
      throw new MethodAccessException("C++ destructor does not have public access");
    }
    swigCPtr = new HandleRef(null, IntPtr.Zero);
    GC.SuppressFinalize(this);
  }

  public FieldDefn(string name, int field_type) : this(ogrPINVOKE.new_FieldDefn(name, field_type), true) {
  }

  public void Destroy() {
    ogrPINVOKE.FieldDefn_Destroy(swigCPtr);
  }

  public string GetName() {
    string ret = ogrPINVOKE.FieldDefn_GetName(swigCPtr);
    return ret;
  }

  public string GetNameRef() {
    string ret = ogrPINVOKE.FieldDefn_GetNameRef(swigCPtr);
    return ret;
  }

  public void SetName(string name) {
    ogrPINVOKE.FieldDefn_SetName(swigCPtr, name);
  }

  public int GetFieldType() {
    int ret = ogrPINVOKE.FieldDefn_GetFieldType(swigCPtr);
    return ret;
  }

  public void SetType(int type) {
    ogrPINVOKE.FieldDefn_SetType(swigCPtr, type);
  }

  public int GetJustify() {
    int ret = ogrPINVOKE.FieldDefn_GetJustify(swigCPtr);
    return ret;
  }

  public void SetJustify(int justify) {
    ogrPINVOKE.FieldDefn_SetJustify(swigCPtr, justify);
  }

  public int GetWidth() {
    int ret = ogrPINVOKE.FieldDefn_GetWidth(swigCPtr);
    return ret;
  }

  public void SetWidth(int width) {
    ogrPINVOKE.FieldDefn_SetWidth(swigCPtr, width);
  }

  public int GetPrecision() {
    int ret = ogrPINVOKE.FieldDefn_GetPrecision(swigCPtr);
    return ret;
  }

  public void SetPrecision(int precision) {
    ogrPINVOKE.FieldDefn_SetPrecision(swigCPtr, precision);
  }

}
