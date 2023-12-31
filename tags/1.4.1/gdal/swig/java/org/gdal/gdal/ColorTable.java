/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.31
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.gdal.gdal;

/* imports for getIndexColorModel */
import java.awt.image.IndexColorModel;
import java.awt.Color;

public class ColorTable {
  private long swigCPtr;
  protected boolean swigCMemOwn;

  protected ColorTable(long cPtr, boolean cMemoryOwn) {
    swigCMemOwn = cMemoryOwn;
    swigCPtr = cPtr;
  }

  protected static long getCPtr(ColorTable obj) {
    return (obj == null) ? 0 : obj.swigCPtr;
  }

  protected void finalize() {
    delete();
  }

  public synchronized void delete() {
    if(swigCPtr != 0 && swigCMemOwn) {
      swigCMemOwn = false;
      gdalJNI.delete_ColorTable(swigCPtr);
    }
    swigCPtr = 0;
  }

/* convienance method */
  public IndexColorModel getIndexColorModel(int bits) {
    int size = GetCount();
    byte[] reds = new byte[size];
    byte[] greens = new byte[size];
    byte[] blues = new byte[size];
    byte[] alphas = new byte[size];

    Color entry = null;
    for(int i = 0; i < size; i++) {
      entry = GetColorEntry(i);
      reds[i] = (byte)entry.getRed();
      greens[i] = (byte)entry.getGreen();
      blues[i] = (byte)entry.getBlue();
      byte alpha = (byte)entry.getAlpha();
      alphas[i] = (alpha != -1) ? alpha : 0;
    }
    return new IndexColorModel(bits, size, reds, greens, blues, alphas);
  }

  public ColorTable(int arg0) {
    this(gdalJNI.new_ColorTable(arg0), true);
  }

  public ColorTable Clone() {
    long cPtr = gdalJNI.ColorTable_Clone(swigCPtr, this);
    return (cPtr == 0) ? null : new ColorTable(cPtr, false);
  }

  public int GetPaletteInterpretation() {
    return gdalJNI.ColorTable_GetPaletteInterpretation(swigCPtr, this);
  }

  public int GetCount() {
    return gdalJNI.ColorTable_GetCount(swigCPtr, this);
  }

  public java.awt.Color GetColorEntry(int arg0) {
    return gdalJNI.ColorTable_GetColorEntry(swigCPtr, this, arg0);
  }

  public int GetColorEntryAsRGB(int arg0, java.awt.Color arg1) {
    return gdalJNI.ColorTable_GetColorEntryAsRGB(swigCPtr, this, arg0, arg1);
  }

  public void SetColorEntry(int arg0, java.awt.Color arg1) {
    gdalJNI.ColorTable_SetColorEntry(swigCPtr, this, arg0, arg1);
  }

}
