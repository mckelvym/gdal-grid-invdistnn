/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.31
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

  public synchronized void delete() {
    if(swigCPtr != 0 && swigCMemOwn) {
      swigCMemOwn = false;
      throw new UnsupportedOperationException("C++ destructor does not have public access");
    }
    swigCPtr = 0;
    super.delete();
  }

  protected static long getCPtrAndDisown(Band obj) {
    if (obj != null) obj.swigCMemOwn= false;
    return getCPtr(obj);
  }

  public int getXSize() {
    return gdalJNI.Band_XSize_get(swigCPtr, this);
  }

  public int getYSize() {
    return gdalJNI.Band_YSize_get(swigCPtr, this);
  }

  public int getDataType() {
    return gdalJNI.Band_DataType_get(swigCPtr, this);
  }

  public void GetBlockSize(int[] pnBlockXSize, int[] pnBlockYSize) {
    gdalJNI.Band_GetBlockSize(swigCPtr, this, pnBlockXSize, pnBlockYSize);
  }

  public int GetRasterColorInterpretation() {
    return gdalJNI.Band_GetRasterColorInterpretation(swigCPtr, this);
  }

  public int SetRasterColorInterpretation(int val) {
    return gdalJNI.Band_SetRasterColorInterpretation(swigCPtr, this, val);
  }

  public void GetNoDataValue(Double[] val) {
    gdalJNI.Band_GetNoDataValue(swigCPtr, this, val);
  }

  public int SetNoDataValue(double d) {
    return gdalJNI.Band_SetNoDataValue(swigCPtr, this, d);
  }

  public void GetMinimum(Double[] val) {
    gdalJNI.Band_GetMinimum(swigCPtr, this, val);
  }

  public void GetMaximum(Double[] val) {
    gdalJNI.Band_GetMaximum(swigCPtr, this, val);
  }

  public void GetOffset(Double[] val) {
    gdalJNI.Band_GetOffset(swigCPtr, this, val);
  }

  public void GetScale(Double[] val) {
    gdalJNI.Band_GetScale(swigCPtr, this, val);
  }

  public int GetStatistics(int approx_ok, int force, double[] min, double[] max, double[] mean, double[] stddev) {
    return gdalJNI.Band_GetStatistics(swigCPtr, this, approx_ok, force, min, max, mean, stddev);
  }

  public int SetStatistics(double min, double max, double mean, double stddev) {
    return gdalJNI.Band_SetStatistics(swigCPtr, this, min, max, mean, stddev);
  }

  public int GetOverviewCount() {
    return gdalJNI.Band_GetOverviewCount(swigCPtr, this);
  }

  public Band GetOverview(int i) {
    long cPtr = gdalJNI.Band_GetOverview(swigCPtr, this, i);
    return (cPtr == 0) ? null : new Band(cPtr, false);
  }

  public int Checksum(int xoff, int yoff, int[] xsize, int[] ysize) {
    return gdalJNI.Band_Checksum(swigCPtr, this, xoff, yoff, xsize, ysize);
  }

  public void ComputeRasterMinMax(double[] argout, int approx_ok) {
    gdalJNI.Band_ComputeRasterMinMax(swigCPtr, this, argout, approx_ok);
  }

  public void ComputeBandStats(double[] argout, int samplestep) {
    gdalJNI.Band_ComputeBandStats(swigCPtr, this, argout, samplestep);
  }

  public int Fill(double real_fill, double imag_fill) {
    return gdalJNI.Band_Fill(swigCPtr, this, real_fill, imag_fill);
  }

  public int ReadRaster(int xoff, int yoff, int xsize, int ysize, char[][] buf_len, int[] buf_xsize, int[] buf_ysize, int[] buf_type) {
    return gdalJNI.Band_ReadRaster(swigCPtr, this, xoff, yoff, xsize, ysize, buf_len, buf_xsize, buf_ysize, buf_type);
  }

  public int WriteRaster(int xoff, int yoff, int xsize, int ysize, char[] buf_len, int[] buf_xsize, int[] buf_ysize, int[] buf_type) {
    return gdalJNI.Band_WriteRaster(swigCPtr, this, xoff, yoff, xsize, ysize, buf_len, buf_xsize, buf_ysize, buf_type);
  }

  public void FlushCache() {
    gdalJNI.Band_FlushCache(swigCPtr, this);
  }

  public ColorTable GetRasterColorTable() {
    long cPtr = gdalJNI.Band_GetRasterColorTable(swigCPtr, this);
    return (cPtr == 0) ? null : new ColorTable(cPtr, false);
  }

  public int SetRasterColorTable(ColorTable arg) {
    return gdalJNI.Band_SetRasterColorTable(swigCPtr, this, ColorTable.getCPtr(arg), arg);
  }

  public int ReadRaster_Direct(int xoff, int yoff, int xsize, int ysize, int buf_xsize, int buf_ysize, int buf_type, java.nio.ByteBuffer buf) {
    return gdalJNI.Band_ReadRaster_Direct(swigCPtr, this, xoff, yoff, xsize, ysize, buf_xsize, buf_ysize, buf_type, buf);
  }

  public int WriteRaster_Direct(int xoff, int yoff, int xsize, int ysize, int buf_xsize, int buf_ysize, int buf_type, java.nio.ByteBuffer buf) {
    return gdalJNI.Band_WriteRaster_Direct(swigCPtr, this, xoff, yoff, xsize, ysize, buf_xsize, buf_ysize, buf_type, buf);
  }

}
