/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.27
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.gdal.gdal;

public class Band extends MajorObject {
  private long swigCPtr;

  protected Band(long cPtr, boolean cMemoryOwn) {
    super(gdalJNI.SWIGBandUpcast(cPtr), cMemoryOwn);
    swigCPtr = cPtr;
  }

  protected static long getCPtr(Band obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  public void delete() {
    if(swigCPtr != 0 && swigCMemOwn) {
      swigCMemOwn = false;
      throw new UnsupportedOperationException("C++ destructor does not have public access");
    }
    swigCPtr = 0;
    super.delete();
  }

  public int getXSize() {
    return gdalJNI.Band_XSize_get(swigCPtr);
  }

  public int getYSize() {
    return gdalJNI.Band_YSize_get(swigCPtr);
  }

  public int getDataType() {
    return gdalJNI.Band_DataType_get(swigCPtr);
  }

  public int GetRasterColorInterpretation() {
    return gdalJNI.Band_GetRasterColorInterpretation(swigCPtr);
  }

  public int SetRasterColorInterpretation(int val) {
    return gdalJNI.Band_SetRasterColorInterpretation(swigCPtr, val);
  }

  public void GetNoDataValue(Double[] val) {
    gdalJNI.Band_GetNoDataValue(swigCPtr, val);
  }

  public int SetNoDataValue(double d) {
    return gdalJNI.Band_SetNoDataValue(swigCPtr, d);
  }

  public void GetMinimum(Double[] val) {
    gdalJNI.Band_GetMinimum(swigCPtr, val);
  }

  public void GetMaximum(Double[] val) {
    gdalJNI.Band_GetMaximum(swigCPtr, val);
  }

  public void GetOffset(Double[] val) {
    gdalJNI.Band_GetOffset(swigCPtr, val);
  }

  public void GetScale(Double[] val) {
    gdalJNI.Band_GetScale(swigCPtr, val);
  }

  public int GetOverviewCount() {
    return gdalJNI.Band_GetOverviewCount(swigCPtr);
  }

  public Band GetOverview(int i) {
    long cPtr = gdalJNI.Band_GetOverview(swigCPtr, i);
    return (cPtr == 0) ? null : new Band(cPtr, false);
  }

  public int Checksum(int xoff, int yoff, int[] xsize, int[] ysize) {
    return gdalJNI.Band_Checksum(swigCPtr, xoff, yoff, xsize, ysize);
  }

  public void ComputeRasterMinMax(double[] argout, int approx_ok) {
    gdalJNI.Band_ComputeRasterMinMax(swigCPtr, argout, approx_ok);
  }

  public int Fill(double real_fill, double imag_fill) {
    return gdalJNI.Band_Fill(swigCPtr, real_fill, imag_fill);
  }

  public int ReadRaster(int xoff, int yoff, int xsize, int ysize, char[][] buf_len, int[] buf_xsize, int[] buf_ysize, int[] buf_type) {
    return gdalJNI.Band_ReadRaster(swigCPtr, xoff, yoff, xsize, ysize, buf_len, buf_xsize, buf_ysize, buf_type);
  }

  public int WriteRaster(int xoff, int yoff, int xsize, int ysize, char[] buf_len, int[] buf_xsize, int[] buf_ysize, int[] buf_type) {
    return gdalJNI.Band_WriteRaster(swigCPtr, xoff, yoff, xsize, ysize, buf_len, buf_xsize, buf_ysize, buf_type);
  }

  public void FlushCache() {
    gdalJNI.Band_FlushCache(swigCPtr);
  }

  public ColorTable GetRasterColorTable() {
    long cPtr = gdalJNI.Band_GetRasterColorTable(swigCPtr);
    return (cPtr == 0) ? null : new ColorTable(cPtr, false);
  }

  public int SetRasterColorTable(ColorTable arg) {
    return gdalJNI.Band_SetRasterColorTable(swigCPtr, ColorTable.getCPtr(arg));
  }

  public int ReadRaster_Direct(int xoff, int yoff, int xsize, int ysize, int buf_xsize, int buf_ysize, int buf_type, java.nio.ByteBuffer buf) {
    return gdalJNI.Band_ReadRaster_Direct(swigCPtr, xoff, yoff, xsize, ysize, buf_xsize, buf_ysize, buf_type, buf);
  }

  public int WriteRaster_Direct(int xoff, int yoff, int xsize, int ysize, int buf_xsize, int buf_ysize, int buf_type, java.nio.ByteBuffer buf) {
    return gdalJNI.Band_WriteRaster_Direct(swigCPtr, xoff, yoff, xsize, ysize, buf_xsize, buf_ysize, buf_type, buf);
  }

}
