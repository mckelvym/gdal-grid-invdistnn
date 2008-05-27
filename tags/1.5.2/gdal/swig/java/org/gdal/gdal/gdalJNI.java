/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.28
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */

package org.gdal.gdal;

class gdalJNI {

  private static boolean available = false;

  static {
    try {
      System.loadLibrary("gdaljni");
      available = true;
    } catch (UnsatisfiedLinkError e) {
      available = false;
      System.err.println("Native library load failed.");
      System.err.println(e);
    }
  }
  
  public static boolean isAvailable() {
    return available;
  }

  public final static native void Debug(String jarg1, String jarg2);
  public final static native void Error(int jarg1, int jarg2, String jarg3);
  public final static native int PushErrorHandler__SWIG_0(String jarg1);
  public final static native void PushErrorHandler__SWIG_1(long jarg1);
  public final static native void PopErrorHandler();
  public final static native void ErrorReset();
  public final static native int GetLastErrorNo();
  public final static native int GetLastErrorType();
  public final static native String GetLastErrorMsg();
  public final static native void PushFinderLocation(String jarg1);
  public final static native void PopFinderLocation();
  public final static native void FinderClean();
  public final static native String FindFile(String jarg1, String jarg2);
  public final static native void SetConfigOption(String jarg1, String jarg2);
  public final static native String GetConfigOption(String jarg1, String jarg2);
  public final static native String CPLBinaryToHex(int jarg1, long jarg2);
  public final static native long CPLHexToBinary(String jarg1, long jarg2);
  public final static native String MajorObject_GetDescription(long jarg1);
  public final static native void MajorObject_SetDescription(long jarg1, String jarg2);
  public final static native java.util.Hashtable MajorObject_GetMetadata_Dict(long jarg1, String jarg2);
  public final static native java.util.Vector MajorObject_GetMetadata_List(long jarg1, String jarg2);
  public final static native int MajorObject_SetMetadata__SWIG_0(long jarg1, java.util.Hashtable jarg2, String jarg3);
  public final static native int MajorObject_SetMetadata__SWIG_1(long jarg1, String jarg2, String jarg3);
  public final static native String Driver_ShortName_get(long jarg1);
  public final static native String Driver_LongName_get(long jarg1);
  public final static native String Driver_HelpTopic_get(long jarg1);
  public final static native long Driver_Create(long jarg1, String jarg2, int jarg3, int jarg4, int jarg5, int jarg6, java.util.Vector jarg7);
  public final static native long Driver_CreateCopy(long jarg1, String jarg2, long jarg3, int jarg4, java.util.Vector jarg5);
  public final static native int Driver_Delete(long jarg1, String jarg2);
  public final static native int Driver_Register(long jarg1);
  public final static native void Driver_Deregister(long jarg1);
  public final static native void GCP_GCPX_set(long jarg1, double jarg2);
  public final static native double GCP_GCPX_get(long jarg1);
  public final static native void GCP_GCPY_set(long jarg1, double jarg2);
  public final static native double GCP_GCPY_get(long jarg1);
  public final static native void GCP_GCPZ_set(long jarg1, double jarg2);
  public final static native double GCP_GCPZ_get(long jarg1);
  public final static native void GCP_GCPPixel_set(long jarg1, double jarg2);
  public final static native double GCP_GCPPixel_get(long jarg1);
  public final static native void GCP_GCPLine_set(long jarg1, double jarg2);
  public final static native double GCP_GCPLine_get(long jarg1);
  public final static native void GCP_Info_set(long jarg1, String jarg2);
  public final static native String GCP_Info_get(long jarg1);
  public final static native void GCP_Id_set(long jarg1, String jarg2);
  public final static native String GCP_Id_get(long jarg1);
  public final static native long new_GCP(double jarg1, double jarg2, double jarg3, double jarg4, double jarg5, String jarg6, String jarg7);
  public final static native void delete_GCP(long jarg1);
  public final static native double GDAL_GCP_GCPX_get(long jarg1);
  public final static native void GDAL_GCP_GCPX_set(long jarg1, double jarg2);
  public final static native double GDAL_GCP_GCPY_get(long jarg1);
  public final static native void GDAL_GCP_GCPY_set(long jarg1, double jarg2);
  public final static native double GDAL_GCP_GCPZ_get(long jarg1);
  public final static native void GDAL_GCP_GCPZ_set(long jarg1, double jarg2);
  public final static native double GDAL_GCP_GCPPixel_get(long jarg1);
  public final static native void GDAL_GCP_GCPPixel_set(long jarg1, double jarg2);
  public final static native double GDAL_GCP_GCPLine_get(long jarg1);
  public final static native void GDAL_GCP_GCPLine_set(long jarg1, double jarg2);
  public final static native String GDAL_GCP_Info_get(long jarg1);
  public final static native void GDAL_GCP_Info_set(long jarg1, String jarg2);
  public final static native String GDAL_GCP_Id_get(long jarg1);
  public final static native void GDAL_GCP_Id_set(long jarg1, String jarg2);
  public final static native double GDAL_GCP_get_GCPX(long jarg1);
  public final static native void GDAL_GCP_set_GCPX(long jarg1, double jarg2);
  public final static native double GDAL_GCP_get_GCPY(long jarg1);
  public final static native void GDAL_GCP_set_GCPY(long jarg1, double jarg2);
  public final static native double GDAL_GCP_get_GCPZ(long jarg1);
  public final static native void GDAL_GCP_set_GCPZ(long jarg1, double jarg2);
  public final static native double GDAL_GCP_get_GCPPixel(long jarg1);
  public final static native void GDAL_GCP_set_GCPPixel(long jarg1, double jarg2);
  public final static native double GDAL_GCP_get_GCPLine(long jarg1);
  public final static native void GDAL_GCP_set_GCPLine(long jarg1, double jarg2);
  public final static native String GDAL_GCP_get_Info(long jarg1);
  public final static native void GDAL_GCP_set_Info(long jarg1, String jarg2);
  public final static native String GDAL_GCP_get_Id(long jarg1);
  public final static native void GDAL_GCP_set_Id(long jarg1, String jarg2);
  public final static native long GCPsToGeoTransform(int jarg1, long jarg2, double[] jarg3, int jarg4);
  public final static native int Dataset_RasterXSize_get(long jarg1);
  public final static native int Dataset_RasterYSize_get(long jarg1);
  public final static native int Dataset_RasterCount_get(long jarg1);
  public final static native void delete_Dataset(long jarg1);
  public final static native long Dataset_GetDriver(long jarg1);
  public final static native long Dataset_GetRasterBand(long jarg1, int jarg2);
  public final static native String Dataset_GetProjection(long jarg1);
  public final static native String Dataset_GetProjectionRef(long jarg1);
  public final static native int Dataset_SetProjection(long jarg1, String jarg2);
  public final static native void Dataset_GetGeoTransform(long jarg1, double[] jarg2);
  public final static native int Dataset_SetGeoTransform(long jarg1, double[] jarg2);
  public final static native int Dataset_BuildOverviews(long jarg1, String jarg2, int[] jarg3);
  public final static native int Dataset_GetGCPCount(long jarg1);
  public final static native String Dataset_GetGCPProjection(long jarg1);
  public final static native void Dataset_GetGCPs(long jarg1, java.util.Vector jarg2);
  public final static native int Dataset_SetGCPs(long jarg1, int jarg2, long jarg3, String jarg4);
  public final static native void Dataset_FlushCache(long jarg1);
  public final static native int Dataset_AddBand(long jarg1, int jarg2, java.util.Vector jarg3);
  public final static native int Dataset_WriteRaster(long jarg1, int jarg2, int jarg3, int jarg4, int jarg5, char[] jarg6, int[] jarg8, int[] jarg9, int[] jarg10, int[] jarg11);
  public final static native int Dataset_ReadRaster(long jarg1, int jarg2, int jarg3, int jarg4, int jarg5, char[][] jarg6, int[] jarg8, int[] jarg9, int[] jarg10, int[] jarg11);
  public final static native int Band_XSize_get(long jarg1);
  public final static native int Band_YSize_get(long jarg1);
  public final static native int Band_DataType_get(long jarg1);
  public final static native void Band_GetBlockSize(long jarg1, int[] jarg2, int[] jarg3);
  public final static native int Band_GetRasterColorInterpretation(long jarg1);
  public final static native int Band_SetRasterColorInterpretation(long jarg1, int jarg2);
  public final static native void Band_GetNoDataValue(long jarg1, Double[] jarg2);
  public final static native int Band_SetNoDataValue(long jarg1, double jarg2);
  public final static native void Band_GetMinimum(long jarg1, Double[] jarg2);
  public final static native void Band_GetMaximum(long jarg1, Double[] jarg2);
  public final static native void Band_GetOffset(long jarg1, Double[] jarg2);
  public final static native void Band_GetScale(long jarg1, Double[] jarg2);
  public final static native int Band_GetStatistics(long jarg1, int jarg2, int jarg3, double[] jarg4, double[] jarg5, double[] jarg6, double[] jarg7);
  public final static native int Band_SetStatistics(long jarg1, double jarg2, double jarg3, double jarg4, double jarg5);
  public final static native int Band_GetOverviewCount(long jarg1);
  public final static native long Band_GetOverview(long jarg1, int jarg2);
  public final static native int Band_Checksum(long jarg1, int jarg2, int jarg3, int[] jarg4, int[] jarg5);
  public final static native void Band_ComputeRasterMinMax(long jarg1, double[] jarg2, int jarg3);
  public final static native void Band_ComputeBandStats(long jarg1, double[] jarg2, int jarg3);
  public final static native int Band_Fill(long jarg1, double jarg2, double jarg3);
  public final static native int Band_ReadRaster(long jarg1, int jarg2, int jarg3, int jarg4, int jarg5, char[][] jarg6, int[] jarg8, int[] jarg9, int[] jarg10);
  public final static native int Band_WriteRaster(long jarg1, int jarg2, int jarg3, int jarg4, int jarg5, char[] jarg6, int[] jarg8, int[] jarg9, int[] jarg10);
  public final static native void Band_FlushCache(long jarg1);
  public final static native long Band_GetRasterColorTable(long jarg1);
  public final static native int Band_SetRasterColorTable(long jarg1, long jarg2);
  public final static native int Band_ReadRaster_Direct(long jarg1, int jarg2, int jarg3, int jarg4, int jarg5, int jarg6, int jarg7, int jarg8, java.nio.ByteBuffer jarg9);
  public final static native int Band_WriteRaster_Direct(long jarg1, int jarg2, int jarg3, int jarg4, int jarg5, int jarg6, int jarg7, int jarg8, java.nio.ByteBuffer jarg9);
  public final static native long new_ColorTable(int jarg1);
  public final static native void delete_ColorTable(long jarg1);
  public final static native long ColorTable_Clone(long jarg1);
  public final static native int ColorTable_GetPaletteInterpretation(long jarg1);
  public final static native int ColorTable_GetCount(long jarg1);
  public final static native java.awt.Color ColorTable_GetColorEntry(long jarg1, int jarg2);
  public final static native int ColorTable_GetColorEntryAsRGB(long jarg1, int jarg2, java.awt.Color jarg3);
  public final static native void ColorTable_SetColorEntry(long jarg1, int jarg2, java.awt.Color jarg3);
  public final static native void AllRegister();
  public final static native int GetCacheMax();
  public final static native void SetCacheMax(int jarg1);
  public final static native int GetCacheUsed();
  public final static native int GetDataTypeSize(int jarg1);
  public final static native int DataTypeIsComplex(int jarg1);
  public final static native String GetDataTypeName(int jarg1);
  public final static native int GetDataTypeByName(String jarg1);
  public final static native String GetColorInterpretationName(int jarg1);
  public final static native String GetPaletteInterpretationName(int jarg1);
  public final static native String DecToDMS(double jarg1, String jarg2, int jarg3);
  public final static native double PackedDMSToDec(double jarg1);
  public final static native double DecToPackedDMS(double jarg1);
  public final static native long ParseXMLString(String jarg1);
  public final static native String SerializeXMLTree(long jarg1);
  public final static native int GetDriverCount();
  public final static native long GetDriverByName(String jarg1);
  public final static native long GetDriver(int jarg1);
  public final static native long Open(String jarg1, int jarg2);
  public final static native long OpenShared(String jarg1, int jarg2);
  public final static native long AutoCreateWarpedVRT(long jarg1, String jarg2, String jarg3, int jarg4, double jarg5);
  public final static native java.util.Vector GeneralCmdLineProcessor(java.util.Vector jarg1, int jarg2);
  public final static native long SWIGDriverUpcast(long jarg1);
  public final static native long SWIGDatasetUpcast(long jarg1);
  public final static native long SWIGBandUpcast(long jarg1);
}
