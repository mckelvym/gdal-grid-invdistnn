/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.28
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.gdal.gdal;

public class Dataset extends MajorObject {
  private long swigCPtr;

  protected Dataset(long cPtr, boolean cMemoryOwn) {
    super(gdalJNI.SWIGDatasetUpcast(cPtr), cMemoryOwn);
    swigCPtr = cPtr;
  }

  protected static long getCPtr(Dataset obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public void delete() {
    if(swigCPtr != 0 && swigCMemOwn) {
      swigCMemOwn = false;
      gdalJNI.delete_Dataset(swigCPtr);
    }
    swigCPtr = 0;
    super.delete();
  }

  protected static long getCPtrAndDisown(Dataset obj) {
    if (obj != null) obj.swigCMemOwn= false;
    return getCPtr(obj);
  }

  public int getRasterXSize() {
    return gdalJNI.Dataset_RasterXSize_get(swigCPtr);
  }

  public int getRasterYSize() {
    return gdalJNI.Dataset_RasterYSize_get(swigCPtr);
  }

  public int getRasterCount() {
    return gdalJNI.Dataset_RasterCount_get(swigCPtr);
  }

  public Driver GetDriver() {
    long cPtr = gdalJNI.Dataset_GetDriver(swigCPtr);
    return (cPtr == 0) ? null : new Driver(cPtr, false);
  }

  public Band GetRasterBand(int nBand) {
    long cPtr = gdalJNI.Dataset_GetRasterBand(swigCPtr, nBand);
    return (cPtr == 0) ? null : new Band(cPtr, false);
  }

  public String GetProjection() {
    return gdalJNI.Dataset_GetProjection(swigCPtr);
  }

  public String GetProjectionRef() {
    return gdalJNI.Dataset_GetProjectionRef(swigCPtr);
  }

  public int SetProjection(String prj) {
    return gdalJNI.Dataset_SetProjection(swigCPtr, prj);
  }

  public void GetGeoTransform(double[] argout) {
    gdalJNI.Dataset_GetGeoTransform(swigCPtr, argout);
  }

  public int SetGeoTransform(double[] argin) {
    return gdalJNI.Dataset_SetGeoTransform(swigCPtr, argin);
  }

  public int BuildOverviews(String resampling, int[] overviewlist) {
    return gdalJNI.Dataset_BuildOverviews(swigCPtr, resampling, overviewlist);
  }

  public int GetGCPCount() {
    return gdalJNI.Dataset_GetGCPCount(swigCPtr);
  }

  public String GetGCPProjection() {
    return gdalJNI.Dataset_GetGCPProjection(swigCPtr);
  }

  public void GetGCPs(java.util.Vector nGCPs) {
    gdalJNI.Dataset_GetGCPs(swigCPtr, nGCPs);
  }

  public int SetGCPs(int nGCPs, GCP pGCPs, String pszGCPProjection) {
    return gdalJNI.Dataset_SetGCPs(swigCPtr, nGCPs, GCP.getCPtr(pGCPs), pszGCPProjection);
  }

  public void FlushCache() {
    gdalJNI.Dataset_FlushCache(swigCPtr);
  }

  public int AddBand(int datatype, java.util.Vector options) {
    return gdalJNI.Dataset_AddBand(swigCPtr, datatype, options);
  }

  public int WriteRaster(int xoff, int yoff, int xsize, int ysize, char[] buf_len, int[] buf_xsize, int[] buf_ysize, int[] buf_type, int[] band_list) {
    return gdalJNI.Dataset_WriteRaster(swigCPtr, xoff, yoff, xsize, ysize, buf_len, buf_xsize, buf_ysize, buf_type, band_list);
  }

  public int ReadRaster(int xoff, int yoff, int xsize, int ysize, char[][] buf_len, int[] buf_xsize, int[] buf_ysize, int[] buf_type, int[] band_list) {
    return gdalJNI.Dataset_ReadRaster(swigCPtr, xoff, yoff, xsize, ysize, buf_len, buf_xsize, buf_ysize, buf_type, band_list);
  }

}
