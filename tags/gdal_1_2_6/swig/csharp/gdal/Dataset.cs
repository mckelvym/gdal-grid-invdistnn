/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.24
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

namespace GDAL {

using System;

public class Dataset : IDisposable {
  private IntPtr swigCPtr;
  protected bool swigCMemOwn;

  internal Dataset(IntPtr cPtr, bool cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  internal static IntPtr getCPtr(Dataset obj) {
    return (obj == null) ? IntPtr.Zero : obj.swigCPtr;
  }

  protected Dataset() : this(IntPtr.Zero, false) {
  }

  ~Dataset() {
    Dispose();
  }

  public virtual void Dispose() {
    if(swigCPtr != IntPtr.Zero && swigCMemOwn) {
      swigCMemOwn = false;
      gdalPINVOKE.delete_Dataset(swigCPtr);
    }
    swigCPtr = IntPtr.Zero;
    GC.SuppressFinalize(this);
  }

  public int RasterXSize {
    get {
      return gdalPINVOKE.get_Dataset_RasterXSize(swigCPtr);
    } 
  }

  public int RasterYSize {
    get {
      return gdalPINVOKE.get_Dataset_RasterYSize(swigCPtr);
    } 
  }

  public int RasterCount {
    get {
      return gdalPINVOKE.get_Dataset_RasterCount(swigCPtr);
    } 
  }

  public Driver GetDriver() {
    IntPtr cPtr = gdalPINVOKE.Dataset_GetDriver(swigCPtr);
    return (cPtr == IntPtr.Zero) ? null : new Driver(cPtr, false);
  }

  public Band GetRasterBand(int nBand) {
    IntPtr cPtr = gdalPINVOKE.Dataset_GetRasterBand(swigCPtr, nBand);
    return (cPtr == IntPtr.Zero) ? null : new Band(cPtr, false);
  }

  public string GetProjection() {
    return gdalPINVOKE.Dataset_GetProjection(swigCPtr);
  }

  public string GetProjectionRef() {
    return gdalPINVOKE.Dataset_GetProjectionRef(swigCPtr);
  }

  public int SetProjection(string prj) {
    return gdalPINVOKE.Dataset_SetProjection(swigCPtr, prj);
  }

  public void GetGeoTransform(SWIGTYPE_p_double_6 argout) {
    gdalPINVOKE.Dataset_GetGeoTransform(swigCPtr, SWIGTYPE_p_double_6.getCPtr(argout));
  }

  public int SetGeoTransform(SWIGTYPE_p_double_6 argin) {
    return gdalPINVOKE.Dataset_SetGeoTransform(swigCPtr, SWIGTYPE_p_double_6.getCPtr(argin));
  }

  public SWIGTYPE_p_p_char GetMetadata(string pszDomain) {
    IntPtr cPtr = gdalPINVOKE.Dataset_GetMetadata(swigCPtr, pszDomain);
    return (cPtr == IntPtr.Zero) ? null : new SWIGTYPE_p_p_char(cPtr, false);
  }

  public int SetMetadata(SWIGTYPE_p_p_char papszMetadata, string pszDomain) {
    return gdalPINVOKE.Dataset_SetMetadata(swigCPtr, SWIGTYPE_p_p_char.getCPtr(papszMetadata), pszDomain);
  }

  public int BuildOverviews(string resampling, int overviewlist) {
    return gdalPINVOKE.Dataset_BuildOverviews(swigCPtr, resampling, overviewlist);
  }

  public int GetGCPCount() {
    return gdalPINVOKE.Dataset_GetGCPCount(swigCPtr);
  }

  public string GetGCPProjection() {
    return gdalPINVOKE.Dataset_GetGCPProjection(swigCPtr);
  }

  public void GetGCPs(SWIGTYPE_p_int nGCPs, SWIGTYPE_p_p_GDAL_GCP pGCPs) {
    gdalPINVOKE.Dataset_GetGCPs(swigCPtr, SWIGTYPE_p_int.getCPtr(nGCPs), SWIGTYPE_p_p_GDAL_GCP.getCPtr(pGCPs));
  }

  public int SetGCPs(int nGCPs, GCP pGCPs, string pszGCPProjection) {
    return gdalPINVOKE.Dataset_SetGCPs(swigCPtr, nGCPs, GCP.getCPtr(pGCPs), pszGCPProjection);
  }

  public void FlushCache() {
    gdalPINVOKE.Dataset_FlushCache(swigCPtr);
  }

  public int AddBand(int datatype, SWIGTYPE_p_p_char options) {
    return gdalPINVOKE.Dataset_AddBand(swigCPtr, datatype, SWIGTYPE_p_p_char.getCPtr(options));
  }

  public int WriteRaster(int xoff, int yoff, int xsize, int ysize, int buf_len, SWIGTYPE_p_int buf_xsize, SWIGTYPE_p_int buf_ysize, SWIGTYPE_p_int buf_type, int band_list) {
    return gdalPINVOKE.Dataset_WriteRaster(swigCPtr, xoff, yoff, xsize, ysize, buf_len, SWIGTYPE_p_int.getCPtr(buf_xsize), SWIGTYPE_p_int.getCPtr(buf_ysize), SWIGTYPE_p_int.getCPtr(buf_type), band_list);
  }

}

}
