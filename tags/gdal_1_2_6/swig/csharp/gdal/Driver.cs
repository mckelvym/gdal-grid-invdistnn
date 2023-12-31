/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.24
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

namespace GDAL {

using System;

public class Driver : IDisposable {
  private IntPtr swigCPtr;
  protected bool swigCMemOwn;

  internal Driver(IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  internal static IntPtr getCPtr(Driver obj) {
    return (obj == null) ? IntPtr.Zero : obj.swigCPtr;
  }

  protected Driver() : this(IntPtr.Zero, false) {
  }

  public virtual void Dispose() {
    if(swigCPtr != IntPtr.Zero && swigCMemOwn) {
      swigCMemOwn = false;
      throw new MethodAccessException("C++ destructor does not have public access");
    }
    swigCPtr = IntPtr.Zero;
    GC.SuppressFinalize(this);
  }

  public string ShortName {
    get {
      return gdalPINVOKE.get_Driver_ShortName(swigCPtr);
    } 
  }

  public string LongName {
    get {
      return gdalPINVOKE.get_Driver_LongName(swigCPtr);
    } 
  }

  public string HelpTopic {
    get {
      return gdalPINVOKE.get_Driver_HelpTopic(swigCPtr);
    } 
  }

  public Dataset Create(string name, int xsize, int ysize, int bands, int eType, SWIGTYPE_p_p_char options) {
    IntPtr cPtr = gdalPINVOKE.Driver_Create(swigCPtr, name, xsize, ysize, bands, eType, SWIGTYPE_p_p_char.getCPtr(options));
    return (cPtr == IntPtr.Zero) ? null : new Dataset(cPtr, true);
  }

  public Dataset CreateCopy(string name, Dataset src, int strict, SWIGTYPE_p_p_char options) {
    IntPtr cPtr = gdalPINVOKE.Driver_CreateCopy(swigCPtr, name, Dataset.getCPtr(src), strict, SWIGTYPE_p_p_char.getCPtr(options));
    return (cPtr == IntPtr.Zero) ? null : new Dataset(cPtr, true);
  }

  public int Delete(string name) {
    return gdalPINVOKE.Driver_Delete(swigCPtr, name);
  }

  public SWIGTYPE_p_p_char GetMetadata(string pszDomain) {
    IntPtr cPtr = gdalPINVOKE.Driver_GetMetadata(swigCPtr, pszDomain);
    return (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_p_char(cPtr, false);
  }

}

}
