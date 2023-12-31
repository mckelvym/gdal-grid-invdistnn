/* ----------------------------------------------------------------------------
 * This file was automatically generated by SWIG (http://www.swig.org).
 * Version 1.3.25
 *
 * Do not make changes to this file unless you know what you are doing--modify
 * the SWIG interface file instead.
 * ----------------------------------------------------------------------------- */


using System;
using System.Runtime.InteropServices;

class ogrPINVOKE {

  protected class SWIGExceptionHelper {

    public delegate void ExceptionDelegate(string message);
    public delegate void ExceptionArgumentDelegate(string message, string paramName);

    static ExceptionDelegate applicationDelegate = new ExceptionDelegate(SetPendingApplicationException);
    static ExceptionDelegate arithmeticDelegate = new ExceptionDelegate(SetPendingArithmeticException);
    static ExceptionDelegate divideByZeroDelegate = new ExceptionDelegate(SetPendingDivideByZeroException);
    static ExceptionDelegate indexOutOfRangeDelegate = new ExceptionDelegate(SetPendingIndexOutOfRangeException);
    static ExceptionDelegate invalidOperationDelegate = new ExceptionDelegate(SetPendingInvalidOperationException);
    static ExceptionDelegate ioDelegate = new ExceptionDelegate(SetPendingIOException);
    static ExceptionDelegate nullReferenceDelegate = new ExceptionDelegate(SetPendingNullReferenceException);
    static ExceptionDelegate outOfMemoryDelegate = new ExceptionDelegate(SetPendingOutOfMemoryException);
    static ExceptionDelegate overflowDelegate = new ExceptionDelegate(SetPendingOverflowException);
    static ExceptionDelegate systemDelegate = new ExceptionDelegate(SetPendingSystemException);

    static ExceptionArgumentDelegate argumentDelegate = new ExceptionArgumentDelegate(SetPendingArgumentException);
    static ExceptionArgumentDelegate argumentNullDelegate = new ExceptionArgumentDelegate(SetPendingArgumentNullException);
    static ExceptionArgumentDelegate argumentOutOfRangeDelegate = new ExceptionArgumentDelegate(SetPendingArgumentOutOfRangeException);

    [DllImport("ogr", EntryPoint="SWIGRegisterExceptionCallbacks_ogr")]
    public static extern void SWIGRegisterExceptionCallbacks_ogr(
                                ExceptionDelegate applicationDelegate,
                                ExceptionDelegate arithmeticDelegate,
                                ExceptionDelegate divideByZeroDelegate, 
                                ExceptionDelegate indexOutOfRangeDelegate, 
                                ExceptionDelegate invalidOperationDelegate,
                                ExceptionDelegate ioDelegate,
                                ExceptionDelegate nullReferenceDelegate,
                                ExceptionDelegate outOfMemoryDelegate, 
                                ExceptionDelegate overflowDelegate, 
                                ExceptionDelegate systemExceptionDelegate);

    [DllImport("ogr", EntryPoint="SWIGRegisterExceptionArgumentCallbacks_ogr")]
    public static extern void SWIGRegisterExceptionCallbacksArgument_ogr(
                                ExceptionArgumentDelegate argumentDelegate,
                                ExceptionArgumentDelegate argumentNullDelegate,
                                ExceptionArgumentDelegate argumentOutOfRangeDelegate);

    static void SetPendingApplicationException(string message) {
      SWIGPendingException.Set(new System.ApplicationException(message));
    }
    static void SetPendingArithmeticException(string message) {
      SWIGPendingException.Set(new System.ArithmeticException(message));
    }
    static void SetPendingDivideByZeroException(string message) {
      SWIGPendingException.Set(new System.DivideByZeroException(message));
    }
    static void SetPendingIndexOutOfRangeException(string message) {
      SWIGPendingException.Set(new System.IndexOutOfRangeException(message));
    }
    static void SetPendingInvalidOperationException(string message) {
      SWIGPendingException.Set(new System.InvalidOperationException(message));
    }
    static void SetPendingIOException(string message) {
      SWIGPendingException.Set(new System.IO.IOException(message));
    }
    static void SetPendingNullReferenceException(string message) {
      SWIGPendingException.Set(new System.NullReferenceException(message));
    }
    static void SetPendingOutOfMemoryException(string message) {
      SWIGPendingException.Set(new System.OutOfMemoryException(message));
    }
    static void SetPendingOverflowException(string message) {
      SWIGPendingException.Set(new System.OverflowException(message));
    }
    static void SetPendingSystemException(string message) {
      SWIGPendingException.Set(new System.SystemException(message));
    }

    static void SetPendingArgumentException(string message, string paramName) {
      SWIGPendingException.Set(new System.ArgumentException(message, paramName));
    }
    static void SetPendingArgumentNullException(string message, string paramName) {
      SWIGPendingException.Set(new System.ArgumentNullException(paramName, message));
    }
    static void SetPendingArgumentOutOfRangeException(string message, string paramName) {
      SWIGPendingException.Set(new System.ArgumentOutOfRangeException(paramName, message));
    }

    static SWIGExceptionHelper() {
      SWIGRegisterExceptionCallbacks_ogr(
                                applicationDelegate,
                                arithmeticDelegate,
                                divideByZeroDelegate,
                                indexOutOfRangeDelegate,
                                invalidOperationDelegate,
                                ioDelegate,
                                nullReferenceDelegate,
                                outOfMemoryDelegate,
                                overflowDelegate,
                                systemDelegate);

      SWIGRegisterExceptionCallbacksArgument_ogr(
                                argumentDelegate,
                                argumentNullDelegate,
                                argumentOutOfRangeDelegate);
    }
  }

  protected static SWIGExceptionHelper swigExceptionHelper = new SWIGExceptionHelper();

  public class SWIGPendingException {
    [ThreadStatic]
    private static Exception pendingException = null;
    private static int numExceptionsPending = 0;

    public static bool Pending {
      get {
        bool pending = false;
        if (numExceptionsPending > 0)
          if (pendingException != null)
            pending = true;
        return pending;
      } 
    }

    public static void Set(Exception e) {
      if (pendingException != null)
        throw new ApplicationException("FATAL: An earlier pending exception from unmanaged code was missed and thus not thrown (" + pendingException.ToString() + ")", e);
      pendingException = e;
      lock(typeof(ogrPINVOKE)) {
        numExceptionsPending++;
      }
    }

    public static Exception Retrieve() {
      Exception e = null;
      if (numExceptionsPending > 0) {
        if (pendingException != null) {
          e = pendingException;
          pendingException = null;
          lock(typeof(ogrPINVOKE)) {
            numExceptionsPending--;
          }
        }
      }
      return e;
    }
  }


  protected class SWIGStringHelper {

    public delegate string SWIGStringDelegate(string message);
    static SWIGStringDelegate stringDelegate = new SWIGStringDelegate(CreateString);

    [DllImport("ogr", EntryPoint="SWIGRegisterStringCallback_ogr")]
    public static extern void SWIGRegisterStringCallback_ogr(SWIGStringDelegate stringDelegate);

    static string CreateString(string cString) {
      return cString;
    }

    static SWIGStringHelper() {
      SWIGRegisterStringCallback_ogr(stringDelegate);
    }
  }

  static protected SWIGStringHelper swigStringHelper = new SWIGStringHelper();


  [DllImport("ogr", EntryPoint="CSharp_get_wkb25Bit")]
  public static extern int get_wkb25Bit();

  [DllImport("ogr", EntryPoint="CSharp_get_wkbUnknown")]
  public static extern int get_wkbUnknown();

  [DllImport("ogr", EntryPoint="CSharp_get_wkbPoint")]
  public static extern int get_wkbPoint();

  [DllImport("ogr", EntryPoint="CSharp_get_wkbLineString")]
  public static extern int get_wkbLineString();

  [DllImport("ogr", EntryPoint="CSharp_get_wkbPolygon")]
  public static extern int get_wkbPolygon();

  [DllImport("ogr", EntryPoint="CSharp_get_wkbMultiPoint")]
  public static extern int get_wkbMultiPoint();

  [DllImport("ogr", EntryPoint="CSharp_get_wkbMultiLineString")]
  public static extern int get_wkbMultiLineString();

  [DllImport("ogr", EntryPoint="CSharp_get_wkbMultiPolygon")]
  public static extern int get_wkbMultiPolygon();

  [DllImport("ogr", EntryPoint="CSharp_get_wkbGeometryCollection")]
  public static extern int get_wkbGeometryCollection();

  [DllImport("ogr", EntryPoint="CSharp_get_wkbNone")]
  public static extern int get_wkbNone();

  [DllImport("ogr", EntryPoint="CSharp_get_wkbLinearRing")]
  public static extern int get_wkbLinearRing();

  [DllImport("ogr", EntryPoint="CSharp_get_wkbPoint25D")]
  public static extern int get_wkbPoint25D();

  [DllImport("ogr", EntryPoint="CSharp_get_wkbLineString25D")]
  public static extern int get_wkbLineString25D();

  [DllImport("ogr", EntryPoint="CSharp_get_wkbPolygon25D")]
  public static extern int get_wkbPolygon25D();

  [DllImport("ogr", EntryPoint="CSharp_get_wkbMultiPoint25D")]
  public static extern int get_wkbMultiPoint25D();

  [DllImport("ogr", EntryPoint="CSharp_get_wkbMultiLineString25D")]
  public static extern int get_wkbMultiLineString25D();

  [DllImport("ogr", EntryPoint="CSharp_get_wkbMultiPolygon25D")]
  public static extern int get_wkbMultiPolygon25D();

  [DllImport("ogr", EntryPoint="CSharp_get_wkbGeometryCollection25D")]
  public static extern int get_wkbGeometryCollection25D();

  [DllImport("ogr", EntryPoint="CSharp_get_OFTInteger")]
  public static extern int get_OFTInteger();

  [DllImport("ogr", EntryPoint="CSharp_get_OFTIntegerList")]
  public static extern int get_OFTIntegerList();

  [DllImport("ogr", EntryPoint="CSharp_get_OFTReal")]
  public static extern int get_OFTReal();

  [DllImport("ogr", EntryPoint="CSharp_get_OFTRealList")]
  public static extern int get_OFTRealList();

  [DllImport("ogr", EntryPoint="CSharp_get_OFTString")]
  public static extern int get_OFTString();

  [DllImport("ogr", EntryPoint="CSharp_get_OFTStringList")]
  public static extern int get_OFTStringList();

  [DllImport("ogr", EntryPoint="CSharp_get_OFTWideString")]
  public static extern int get_OFTWideString();

  [DllImport("ogr", EntryPoint="CSharp_get_OFTWideStringList")]
  public static extern int get_OFTWideStringList();

  [DllImport("ogr", EntryPoint="CSharp_get_OFTBinary")]
  public static extern int get_OFTBinary();

  [DllImport("ogr", EntryPoint="CSharp_get_OJUndefined")]
  public static extern int get_OJUndefined();

  [DllImport("ogr", EntryPoint="CSharp_get_OJLeft")]
  public static extern int get_OJLeft();

  [DllImport("ogr", EntryPoint="CSharp_get_OJRight")]
  public static extern int get_OJRight();

  [DllImport("ogr", EntryPoint="CSharp_get_wkbXDR")]
  public static extern int get_wkbXDR();

  [DllImport("ogr", EntryPoint="CSharp_get_wkbNDR")]
  public static extern int get_wkbNDR();

  [DllImport("ogr", EntryPoint="CSharp_get_OLCRandomRead")]
  public static extern string get_OLCRandomRead();

  [DllImport("ogr", EntryPoint="CSharp_get_OLCSequentialWrite")]
  public static extern string get_OLCSequentialWrite();

  [DllImport("ogr", EntryPoint="CSharp_get_OLCRandomWrite")]
  public static extern string get_OLCRandomWrite();

  [DllImport("ogr", EntryPoint="CSharp_get_OLCFastSpatialFilter")]
  public static extern string get_OLCFastSpatialFilter();

  [DllImport("ogr", EntryPoint="CSharp_get_OLCFastFeatureCount")]
  public static extern string get_OLCFastFeatureCount();

  [DllImport("ogr", EntryPoint="CSharp_get_OLCFastGetExtent")]
  public static extern string get_OLCFastGetExtent();

  [DllImport("ogr", EntryPoint="CSharp_get_OLCCreateField")]
  public static extern string get_OLCCreateField();

  [DllImport("ogr", EntryPoint="CSharp_get_OLCTransactions")]
  public static extern string get_OLCTransactions();

  [DllImport("ogr", EntryPoint="CSharp_get_OLCDeleteFeature")]
  public static extern string get_OLCDeleteFeature();

  [DllImport("ogr", EntryPoint="CSharp_get_OLCFastSetNextByIndex")]
  public static extern string get_OLCFastSetNextByIndex();

  [DllImport("ogr", EntryPoint="CSharp_get_ODsCCreateLayer")]
  public static extern string get_ODsCCreateLayer();

  [DllImport("ogr", EntryPoint="CSharp_get_ODsCDeleteLayer")]
  public static extern string get_ODsCDeleteLayer();

  [DllImport("ogr", EntryPoint="CSharp_get_ODrCCreateDataSource")]
  public static extern string get_ODrCCreateDataSource();

  [DllImport("ogr", EntryPoint="CSharp_get_ODrCDeleteDataSource")]
  public static extern string get_ODrCDeleteDataSource();

  [DllImport("ogr", EntryPoint="CSharp_get_Driver_name")]
  public static extern string get_Driver_name(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Driver_CreateDataSource")]
  public static extern IntPtr Driver_CreateDataSource(HandleRef jarg1, string jarg2, HandleRef jarg3);

  [DllImport("ogr", EntryPoint="CSharp_Driver_CopyDataSource")]
  public static extern IntPtr Driver_CopyDataSource(HandleRef jarg1, HandleRef jarg2, string jarg3, HandleRef jarg4);

  [DllImport("ogr", EntryPoint="CSharp_Driver_Open")]
  public static extern IntPtr Driver_Open(HandleRef jarg1, string jarg2, int jarg3);

  [DllImport("ogr", EntryPoint="CSharp_Driver_DeleteDataSource")]
  public static extern int Driver_DeleteDataSource(HandleRef jarg1, string jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Driver_TestCapability")]
  public static extern int Driver_TestCapability(HandleRef jarg1, string jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Driver_GetName")]
  public static extern string Driver_GetName(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_get_DataSource_name")]
  public static extern string get_DataSource_name(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_DataSource_Destroy")]
  public static extern void DataSource_Destroy(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_DataSource_Release")]
  public static extern void DataSource_Release(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_DataSource_Reference")]
  public static extern int DataSource_Reference(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_DataSource_Dereference")]
  public static extern int DataSource_Dereference(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_DataSource_GetRefCount")]
  public static extern int DataSource_GetRefCount(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_DataSource_GetSummaryRefCount")]
  public static extern int DataSource_GetSummaryRefCount(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_DataSource_GetLayerCount")]
  public static extern int DataSource_GetLayerCount(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_DataSource_GetName")]
  public static extern string DataSource_GetName(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_DataSource_DeleteLayer")]
  public static extern int DataSource_DeleteLayer(HandleRef jarg1, int jarg2);

  [DllImport("ogr", EntryPoint="CSharp_DataSource_CreateLayer")]
  public static extern IntPtr DataSource_CreateLayer(HandleRef jarg1, string jarg2, HandleRef jarg3, int jarg4, HandleRef jarg5);

  [DllImport("ogr", EntryPoint="CSharp_DataSource_CopyLayer")]
  public static extern IntPtr DataSource_CopyLayer(HandleRef jarg1, HandleRef jarg2, string jarg3, HandleRef jarg4);

  [DllImport("ogr", EntryPoint="CSharp_DataSource_GetLayerByIndex")]
  public static extern IntPtr DataSource_GetLayerByIndex(HandleRef jarg1, int jarg2);

  [DllImport("ogr", EntryPoint="CSharp_DataSource_GetLayerByName")]
  public static extern IntPtr DataSource_GetLayerByName(HandleRef jarg1, string jarg2);

  [DllImport("ogr", EntryPoint="CSharp_DataSource_TestCapability")]
  public static extern int DataSource_TestCapability(HandleRef jarg1, string jarg2);

  [DllImport("ogr", EntryPoint="CSharp_DataSource_ExecuteSQL")]
  public static extern IntPtr DataSource_ExecuteSQL(HandleRef jarg1, string jarg2, HandleRef jarg3, string jarg4);

  [DllImport("ogr", EntryPoint="CSharp_DataSource_ReleaseResultSet")]
  public static extern void DataSource_ReleaseResultSet(HandleRef jarg1, HandleRef jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Layer_Reference")]
  public static extern int Layer_Reference(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Layer_Dereference")]
  public static extern int Layer_Dereference(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Layer_GetRefCount")]
  public static extern int Layer_GetRefCount(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Layer_SetSpatialFilter")]
  public static extern void Layer_SetSpatialFilter(HandleRef jarg1, HandleRef jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Layer_SetSpatialFilterRect")]
  public static extern void Layer_SetSpatialFilterRect(HandleRef jarg1, double jarg2, double jarg3, double jarg4, double jarg5);

  [DllImport("ogr", EntryPoint="CSharp_Layer_GetSpatialFilter")]
  public static extern IntPtr Layer_GetSpatialFilter(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Layer_SetAttributeFilter")]
  public static extern int Layer_SetAttributeFilter(HandleRef jarg1, string jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Layer_ResetReading")]
  public static extern void Layer_ResetReading(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Layer_GetName")]
  public static extern string Layer_GetName(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Layer_GetFeature")]
  public static extern IntPtr Layer_GetFeature(HandleRef jarg1, int jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Layer_GetNextFeature")]
  public static extern IntPtr Layer_GetNextFeature(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Layer_SetNextByIndex")]
  public static extern int Layer_SetNextByIndex(HandleRef jarg1, int jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Layer_SetFeature")]
  public static extern int Layer_SetFeature(HandleRef jarg1, HandleRef jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Layer_CreateFeature")]
  public static extern int Layer_CreateFeature(HandleRef jarg1, HandleRef jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Layer_DeleteFeature")]
  public static extern int Layer_DeleteFeature(HandleRef jarg1, int jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Layer_SyncToDisk")]
  public static extern int Layer_SyncToDisk(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Layer_GetLayerDefn")]
  public static extern IntPtr Layer_GetLayerDefn(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Layer_GetFeatureCount")]
  public static extern int Layer_GetFeatureCount(HandleRef jarg1, int jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Layer_GetExtent")]
  public static extern void Layer_GetExtent(HandleRef jarg1, int jarg3);

  [DllImport("ogr", EntryPoint="CSharp_Layer_TestCapability")]
  public static extern int Layer_TestCapability(HandleRef jarg1, string jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Layer_CreateField")]
  public static extern int Layer_CreateField(HandleRef jarg1, HandleRef jarg2, int jarg3);

  [DllImport("ogr", EntryPoint="CSharp_Layer_StartTransaction")]
  public static extern int Layer_StartTransaction(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Layer_CommitTransaction")]
  public static extern int Layer_CommitTransaction(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Layer_RollbackTransaction")]
  public static extern int Layer_RollbackTransaction(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Layer_GetSpatialRef")]
  public static extern IntPtr Layer_GetSpatialRef(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Layer_GetFeatureRead")]
  public static extern IntPtr Layer_GetFeatureRead(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_new_Feature")]
  public static extern IntPtr new_Feature(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Feature_Destroy")]
  public static extern void Feature_Destroy(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Feature_GetDefnRef")]
  public static extern IntPtr Feature_GetDefnRef(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Feature_SetGeometry")]
  public static extern int Feature_SetGeometry(HandleRef jarg1, HandleRef jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Feature_SetGeometryDirectly")]
  public static extern int Feature_SetGeometryDirectly(HandleRef jarg1, HandleRef jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Feature_GetGeometryRef")]
  public static extern IntPtr Feature_GetGeometryRef(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Feature_Clone")]
  public static extern IntPtr Feature_Clone(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Feature_Equal")]
  public static extern int Feature_Equal(HandleRef jarg1, HandleRef jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Feature_GetFieldCount")]
  public static extern int Feature_GetFieldCount(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Feature_GetFieldDefnRef__SWIG_0")]
  public static extern IntPtr Feature_GetFieldDefnRef__SWIG_0(HandleRef jarg1, int jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Feature_GetFieldDefnRef__SWIG_1")]
  public static extern IntPtr Feature_GetFieldDefnRef__SWIG_1(HandleRef jarg1, string jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Feature_GetFieldAsString__SWIG_0")]
  public static extern string Feature_GetFieldAsString__SWIG_0(HandleRef jarg1, int jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Feature_GetFieldAsString__SWIG_1")]
  public static extern string Feature_GetFieldAsString__SWIG_1(HandleRef jarg1, string jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Feature_GetFieldAsInteger__SWIG_0")]
  public static extern int Feature_GetFieldAsInteger__SWIG_0(HandleRef jarg1, int jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Feature_GetFieldAsInteger__SWIG_1")]
  public static extern int Feature_GetFieldAsInteger__SWIG_1(HandleRef jarg1, string jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Feature_GetFieldAsDouble__SWIG_0")]
  public static extern double Feature_GetFieldAsDouble__SWIG_0(HandleRef jarg1, int jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Feature_GetFieldAsDouble__SWIG_1")]
  public static extern double Feature_GetFieldAsDouble__SWIG_1(HandleRef jarg1, string jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Feature_IsFieldSet__SWIG_0")]
  public static extern int Feature_IsFieldSet__SWIG_0(HandleRef jarg1, int jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Feature_IsFieldSet__SWIG_1")]
  public static extern int Feature_IsFieldSet__SWIG_1(HandleRef jarg1, string jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Feature_GetFieldIndex")]
  public static extern int Feature_GetFieldIndex(HandleRef jarg1, string jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Feature_GetFID")]
  public static extern int Feature_GetFID(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Feature_SetFID")]
  public static extern int Feature_SetFID(HandleRef jarg1, int jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Feature_DumpReadable")]
  public static extern void Feature_DumpReadable(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Feature_UnsetField__SWIG_0")]
  public static extern void Feature_UnsetField__SWIG_0(HandleRef jarg1, int jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Feature_UnsetField__SWIG_1")]
  public static extern void Feature_UnsetField__SWIG_1(HandleRef jarg1, string jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Feature_SetField__SWIG_0")]
  public static extern void Feature_SetField__SWIG_0(HandleRef jarg1, int jarg2, string jarg3);

  [DllImport("ogr", EntryPoint="CSharp_Feature_SetField__SWIG_1")]
  public static extern void Feature_SetField__SWIG_1(HandleRef jarg1, string jarg2, string jarg3);

  [DllImport("ogr", EntryPoint="CSharp_Feature_SetFrom")]
  public static extern int Feature_SetFrom(HandleRef jarg1, HandleRef jarg2, int jarg3);

  [DllImport("ogr", EntryPoint="CSharp_Feature_GetStyleString")]
  public static extern string Feature_GetStyleString(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Feature_SetStyleString")]
  public static extern void Feature_SetStyleString(HandleRef jarg1, string jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Feature_GetFieldType__SWIG_0")]
  public static extern int Feature_GetFieldType__SWIG_0(HandleRef jarg1, int jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Feature_GetFieldType__SWIG_1")]
  public static extern int Feature_GetFieldType__SWIG_1(HandleRef jarg1, string jarg2, string jarg3);

  [DllImport("ogr", EntryPoint="CSharp_new_FeatureDefn")]
  public static extern IntPtr new_FeatureDefn(string jarg1);

  [DllImport("ogr", EntryPoint="CSharp_FeatureDefn_Destroy")]
  public static extern void FeatureDefn_Destroy(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_FeatureDefn_GetName")]
  public static extern string FeatureDefn_GetName(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_FeatureDefn_GetFieldCount")]
  public static extern int FeatureDefn_GetFieldCount(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_FeatureDefn_GetFieldDefn")]
  public static extern IntPtr FeatureDefn_GetFieldDefn(HandleRef jarg1, int jarg2);

  [DllImport("ogr", EntryPoint="CSharp_FeatureDefn_GetFieldIndex")]
  public static extern int FeatureDefn_GetFieldIndex(HandleRef jarg1, string jarg2);

  [DllImport("ogr", EntryPoint="CSharp_FeatureDefn_AddFieldDefn")]
  public static extern void FeatureDefn_AddFieldDefn(HandleRef jarg1, HandleRef jarg2);

  [DllImport("ogr", EntryPoint="CSharp_FeatureDefn_GetGeomType")]
  public static extern int FeatureDefn_GetGeomType(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_FeatureDefn_SetGeomType")]
  public static extern void FeatureDefn_SetGeomType(HandleRef jarg1, int jarg2);

  [DllImport("ogr", EntryPoint="CSharp_FeatureDefn_Reference")]
  public static extern int FeatureDefn_Reference(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_FeatureDefn_Dereference")]
  public static extern int FeatureDefn_Dereference(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_FeatureDefn_GetReferenceCount")]
  public static extern int FeatureDefn_GetReferenceCount(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_new_FieldDefn")]
  public static extern IntPtr new_FieldDefn(string jarg1, int jarg2);

  [DllImport("ogr", EntryPoint="CSharp_FieldDefn_Destroy")]
  public static extern void FieldDefn_Destroy(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_FieldDefn_GetName")]
  public static extern string FieldDefn_GetName(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_FieldDefn_GetNameRef")]
  public static extern string FieldDefn_GetNameRef(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_FieldDefn_SetName")]
  public static extern void FieldDefn_SetName(HandleRef jarg1, string jarg2);

  [DllImport("ogr", EntryPoint="CSharp_FieldDefn_GetFieldType")]
  public static extern int FieldDefn_GetFieldType(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_FieldDefn_SetType")]
  public static extern void FieldDefn_SetType(HandleRef jarg1, int jarg2);

  [DllImport("ogr", EntryPoint="CSharp_FieldDefn_GetJustify")]
  public static extern int FieldDefn_GetJustify(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_FieldDefn_SetJustify")]
  public static extern void FieldDefn_SetJustify(HandleRef jarg1, int jarg2);

  [DllImport("ogr", EntryPoint="CSharp_FieldDefn_GetWidth")]
  public static extern int FieldDefn_GetWidth(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_FieldDefn_SetWidth")]
  public static extern void FieldDefn_SetWidth(HandleRef jarg1, int jarg2);

  [DllImport("ogr", EntryPoint="CSharp_FieldDefn_GetPrecision")]
  public static extern int FieldDefn_GetPrecision(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_FieldDefn_SetPrecision")]
  public static extern void FieldDefn_SetPrecision(HandleRef jarg1, int jarg2);

  [DllImport("ogr", EntryPoint="CSharp_CreateGeometryFromWkb")]
  public static extern IntPtr CreateGeometryFromWkb(int jarg1, HandleRef jarg3);

  [DllImport("ogr", EntryPoint="CSharp_CreateGeometryFromWkt")]
  public static extern IntPtr CreateGeometryFromWkt(HandleRef jarg1, HandleRef jarg2);

  [DllImport("ogr", EntryPoint="CSharp_CreateGeometryFromGML")]
  public static extern IntPtr CreateGeometryFromGML(string jarg1);

  [DllImport("ogr", EntryPoint="CSharp_new_Geometry")]
  public static extern IntPtr new_Geometry(int jarg1, string jarg2, int jarg3, string jarg4, string jarg5);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_ExportToWkt")]
  public static extern string Geometry_ExportToWkt(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_ExportToWkb")]
  public static extern int Geometry_ExportToWkb(HandleRef jarg1, int jarg4);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_ExportToGML")]
  public static extern string Geometry_ExportToGML(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_AddPoint")]
  public static extern void Geometry_AddPoint(HandleRef jarg1, double jarg2, double jarg3, double jarg4);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_AddGeometryDirectly")]
  public static extern int Geometry_AddGeometryDirectly(HandleRef jarg1, HandleRef jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_AddGeometry")]
  public static extern int Geometry_AddGeometry(HandleRef jarg1, HandleRef jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_Clone")]
  public static extern IntPtr Geometry_Clone(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_Destroy")]
  public static extern void Geometry_Destroy(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_GetGeometryType")]
  public static extern int Geometry_GetGeometryType(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_GetGeometryName")]
  public static extern string Geometry_GetGeometryName(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_GetArea")]
  public static extern double Geometry_GetArea(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_GetPointCount")]
  public static extern int Geometry_GetPointCount(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_GetX")]
  public static extern double Geometry_GetX(HandleRef jarg1, int jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_GetY")]
  public static extern double Geometry_GetY(HandleRef jarg1, int jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_GetZ")]
  public static extern double Geometry_GetZ(HandleRef jarg1, int jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_GetGeometryCount")]
  public static extern int Geometry_GetGeometryCount(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_SetPoint")]
  public static extern void Geometry_SetPoint(HandleRef jarg1, int jarg2, double jarg3, double jarg4, double jarg5);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_GetGeometryRef")]
  public static extern IntPtr Geometry_GetGeometryRef(HandleRef jarg1, int jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_GetBoundary")]
  public static extern IntPtr Geometry_GetBoundary(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_ConvexHull")]
  public static extern IntPtr Geometry_ConvexHull(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_Buffer")]
  public static extern IntPtr Geometry_Buffer(HandleRef jarg1, double jarg2, int jarg3);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_Intersection")]
  public static extern IntPtr Geometry_Intersection(HandleRef jarg1, HandleRef jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_Union")]
  public static extern IntPtr Geometry_Union(HandleRef jarg1, HandleRef jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_Difference")]
  public static extern IntPtr Geometry_Difference(HandleRef jarg1, HandleRef jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_SymmetricDifference")]
  public static extern IntPtr Geometry_SymmetricDifference(HandleRef jarg1, HandleRef jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_Distance")]
  public static extern double Geometry_Distance(HandleRef jarg1, HandleRef jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_Empty")]
  public static extern void Geometry_Empty(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_Intersect")]
  public static extern int Geometry_Intersect(HandleRef jarg1, HandleRef jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_Equal")]
  public static extern int Geometry_Equal(HandleRef jarg1, HandleRef jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_Disjoint")]
  public static extern int Geometry_Disjoint(HandleRef jarg1, HandleRef jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_Touches")]
  public static extern int Geometry_Touches(HandleRef jarg1, HandleRef jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_Crosses")]
  public static extern int Geometry_Crosses(HandleRef jarg1, HandleRef jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_Within")]
  public static extern int Geometry_Within(HandleRef jarg1, HandleRef jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_Contains")]
  public static extern int Geometry_Contains(HandleRef jarg1, HandleRef jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_Overlaps")]
  public static extern int Geometry_Overlaps(HandleRef jarg1, HandleRef jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_TransformTo")]
  public static extern int Geometry_TransformTo(HandleRef jarg1, HandleRef jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_Transform")]
  public static extern int Geometry_Transform(HandleRef jarg1, HandleRef jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_GetSpatialReference")]
  public static extern IntPtr Geometry_GetSpatialReference(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_AssignSpatialReference")]
  public static extern void Geometry_AssignSpatialReference(HandleRef jarg1, HandleRef jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_CloseRings")]
  public static extern void Geometry_CloseRings(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_FlattenTo2D")]
  public static extern void Geometry_FlattenTo2D(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_GetEnvelope")]
  public static extern void Geometry_GetEnvelope(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_Centroid")]
  public static extern IntPtr Geometry_Centroid(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_WkbSize")]
  public static extern int Geometry_WkbSize(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_GetCoordinateDimension")]
  public static extern int Geometry_GetCoordinateDimension(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Geometry_GetDimension")]
  public static extern int Geometry_GetDimension(HandleRef jarg1);

  [DllImport("ogr", EntryPoint="CSharp_GetDriverCount")]
  public static extern int GetDriverCount();

  [DllImport("ogr", EntryPoint="CSharp_GetOpenDSCount")]
  public static extern int GetOpenDSCount();

  [DllImport("ogr", EntryPoint="CSharp_SetGenerate_DB2_V72_BYTE_ORDER")]
  public static extern int SetGenerate_DB2_V72_BYTE_ORDER(int jarg1);

  [DllImport("ogr", EntryPoint="CSharp_RegisterAll")]
  public static extern void RegisterAll();

  [DllImport("ogr", EntryPoint="CSharp_OGR_GetDriverByName")]
  public static extern IntPtr OGR_GetDriverByName(string jarg1);

  [DllImport("ogr", EntryPoint="CSharp_OGR_GetDriver")]
  public static extern IntPtr OGR_GetDriver(int jarg1);

  [DllImport("ogr", EntryPoint="CSharp_GetOpenDS")]
  public static extern IntPtr GetOpenDS(int jarg1);

  [DllImport("ogr", EntryPoint="CSharp_Open__SWIG_0")]
  public static extern IntPtr Open__SWIG_0(string jarg1, int jarg2);

  [DllImport("ogr", EntryPoint="CSharp_Open__SWIG_1")]
  public static extern IntPtr Open__SWIG_1(string jarg1);

  [DllImport("ogr", EntryPoint="CSharp_OpenShared__SWIG_0")]
  public static extern IntPtr OpenShared__SWIG_0(string jarg1, int jarg2);

  [DllImport("ogr", EntryPoint="CSharp_OpenShared__SWIG_1")]
  public static extern IntPtr OpenShared__SWIG_1(string jarg1);
}
