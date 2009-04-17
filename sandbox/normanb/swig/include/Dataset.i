/******************************************************************************
 * $Id: Dataset.i 16493 2009-03-07 12:07:37Z rouault $
 *
 * Name:     Dataset.i
 * Project:  GDAL Python Interface
 * Purpose:  GDAL Core SWIG Interface declarations.
 * Author:   Kevin Ruland, kruland@ku.edu
 *
 ******************************************************************************
 * Copyright (c) 2005, Kevin Ruland
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *****************************************************************************/

#if !defined(SWIGJAVA)
%{

static
CPLErr DSReadRaster_internal( GDALDatasetShadow *obj, 
                            int xoff, int yoff, int xsize, int ysize,
                            int buf_xsize, int buf_ysize,
                            GDALDataType buf_type,
                            int *buf_size, char **buf,
                            int band_list, int *pband_list )
{
  CPLErr result;
  *buf_size = (size_t)buf_xsize * buf_ysize * (GDALGetDataTypeSize( buf_type ) / 8) * band_list;
  *buf = (char*) VSIMalloc3( buf_xsize, buf_ysize, (GDALGetDataTypeSize( buf_type ) / 8) * band_list );
  if (*buf)
  {
    result = GDALDatasetRasterIO(obj, GF_Read, xoff, yoff, xsize, ysize,
                                    (void*) *buf, buf_xsize, buf_ysize, buf_type,
                                    band_list, pband_list, 0, 0, 0 );
    if ( result != CE_None ) {
        free( *buf );
        *buf = 0;
        *buf_size = 0;
    }
  }
  else
  {
    result = CE_Failure;
    *buf = 0;
    *buf_size = 0;
  }
  return result;
}
%}
#endif


//************************************************************************
//
// Define the extensions for GDALAsyncRasterIO (nee GDALAsyncRasterIOShadow)
//
//************************************************************************
%rename (GDALAsyncRasterIO) GDALAsyncRasterIOShadow;

class GDALAsyncRasterIOShadow {
private:
  GDALAsyncRasterIOShadow();
public:
%extend {
%immutable;
    int XOff;
    int YOff;
    int XSize;
    int YSize;
    void * Buffer;
    int BufferXSize;
    int BufferYSize;
    GDALDataType BufferType;
    int BandCount;
    int NDataRead;
    int PixelSpace;
    int LineSpace;
    int BandSpace;
%mutable;

    %apply (int *OUTPUT){int *xoff, int *yoff, int *buf_xsize, int *buf_ysize}
    GDALAsyncStatusType GetNextUpdatedRegion(bool wait, int timeout, int* xoff, int* yoff, int* buf_xsize, int* buf_ysize)
    {
        return GDALGetNextUpdatedRegion(self, wait, timeout, xoff, yoff, buf_xsize, buf_ysize);
    }
    %clear (int *xoff, int *yoff, int *buf_xsize, int *buf_ysize);
    
//    void LockBuffer(int xbufoff, int ybufoff, int xbufsize, int ybufsize)
//    {
//        GDALLockBuffer(self, xbufoff, ybufoff, xbufsize, ybufsize);
//    }
    
    void LockBuffer()
    {
        GDALLockBuffer(self);
    }
    
    void UnlockBuffer()
    {
        GDALUnlockBuffer(self);
    }
    } /* extend */
}; /* GDALAsyncRasterIOShadow */ 


//************************************************************************
//
// Define the extensions for Dataset (nee GDALDatasetShadow)
//
//************************************************************************

%rename (Dataset) GDALDatasetShadow;

class GDALDatasetShadow : public GDALMajorObjectShadow {
private:
  GDALDatasetShadow();
public:
%extend {

%immutable;
  int RasterXSize;
  int RasterYSize;
  int RasterCount;
//
// Needed
// _band list?
%mutable;

  ~GDALDatasetShadow() {
    if ( GDALDereferenceDataset( self ) <= 0 ) {
      GDALClose(self);
    }
  }

  GDALDriverShadow* GetDriver() {
    return (GDALDriverShadow*) GDALGetDatasetDriver( self );
  }

  GDALRasterBandShadow* GetRasterBand(int nBand ) {
    return (GDALRasterBandShadow*) GDALGetRasterBand( self, nBand );
  }

  char const *GetProjection() {
    return GDALGetProjectionRef( self );
  }

  char const *GetProjectionRef() {
    return GDALGetProjectionRef( self );
  }

  CPLErr SetProjection( char const *prj ) {
    return GDALSetProjection( self, prj );
  }

  void GetGeoTransform( double argout[6] ) {
    if ( GDALGetGeoTransform( self, argout ) != 0 ) {
      argout[0] = 0.0;
      argout[1] = 1.0;
      argout[2] = 0.0;
      argout[3] = 0.0;
      argout[4] = 0.0;
      argout[5] = 1.0;
    }
  }

  CPLErr SetGeoTransform( double argin[6] ) {
    return GDALSetGeoTransform( self, argin );
  }

  // The (int,int*) arguments are typemapped.  The name of the first argument
  // becomes the kwarg name for it.
#ifndef SWIGCSHARP  
#ifndef SWIGJAVA
%feature("kwargs") BuildOverviews;
#endif
%apply (int nList, int* pList) { (int overviewlist, int *pOverviews) };
#else
%apply (void *buffer_ptr) {int *pOverviews};
#endif
#ifdef SWIGJAVA
%apply (const char* stringWithDefaultValue) {const char* resampling};
  int BuildOverviews( const char *resampling,
                      int overviewlist, int *pOverviews,
                      GDALProgressFunc callback = NULL,
                      void* callback_data=NULL ) {
#else
  int BuildOverviews( const char *resampling = "NEAREST",
                      int overviewlist = 0 , int *pOverviews = 0,
                      GDALProgressFunc callback = NULL,
                      void* callback_data=NULL ) {
#endif
    return GDALBuildOverviews(  self, 
                                resampling ? resampling : "NEAREST", 
                                overviewlist, 
                                pOverviews, 
                                0, 
                                0, 
                                callback, 
                                callback_data);
  }
#ifndef SWIGCSHARP
%clear (int overviewlist, int *pOverviews);
#else
%clear (int *pOverviews);
#endif
#ifdef SWIGJAVA
%clear (const char *resampling);
#endif

  int GetGCPCount() {
    return GDALGetGCPCount( self );
  }

  const char *GetGCPProjection() {
    return GDALGetGCPProjection( self );
  }
  
#ifndef SWIGCSHARP
  void GetGCPs( int *nGCPs, GDAL_GCP const **pGCPs ) {
    *nGCPs = GDALGetGCPCount( self );
    *pGCPs = GDALGetGCPs( self );
  }

  CPLErr SetGCPs( int nGCPs, GDAL_GCP const *pGCPs, const char *pszGCPProjection ) {
    return GDALSetGCPs( self, nGCPs, pGCPs, pszGCPProjection );
  }

#endif

  void FlushCache() {
    GDALFlushCache( self );
  }

#ifndef SWIGJAVA
%feature ("kwargs") AddBand;
#endif
/* uses the defined char **options typemap */
  CPLErr AddBand( GDALDataType datatype = GDT_Byte, char **options = 0 ) {
    return GDALAddBand( self, datatype, options );
  }

  CPLErr CreateMaskBand( int nFlags ) {
      return GDALCreateDatasetMaskBand( self, nFlags );
  }

#if defined(SWIGPYTHON) || defined (SWIGJAVA)
%apply (char **out_ppsz_and_free) {char **};
#else
/* FIXME: wrong typemap. GetFileList() return should be CSLDestroy'ed */
%apply (char **options) {char **};
#endif
  char **GetFileList() {
    return GDALGetFileList( self );
  }
%clear char **;

#if !defined(SWIGCSHARP) && !defined(SWIGJAVA)
%feature("kwargs") WriteRaster;
%apply (int nLen, char *pBuf) { (int buf_len, char *buf_string) };
%apply (int *optional_int) { (int*) };
%apply (int *optional_int) { (GDALDataType *buf_type) };
%apply (int nList, int *pList ) { (int band_list, int *pband_list ) };
  CPLErr WriteRaster( int xoff, int yoff, int xsize, int ysize,
	              int buf_len, char *buf_string,
                      int *buf_xsize = 0, int *buf_ysize = 0,
                      GDALDataType *buf_type = 0,
                      int band_list = 0, int *pband_list = 0 ) {
    int nxsize = (buf_xsize==0) ? xsize : *buf_xsize;
    int nysize = (buf_ysize==0) ? ysize : *buf_ysize;
    GDALDataType ntype;
    if ( buf_type != 0 ) {
      ntype = (GDALDataType) *buf_type;
    } else {
      int lastband = GDALGetRasterCount( self ) - 1;
      ntype = GDALGetRasterDataType( GDALGetRasterBand( self, lastband ) );
    }
    bool myBandList = false;
    int nBandCount;
    int *pBandList;
    if ( band_list != 0 ) {
      myBandList = false;
      nBandCount = band_list;
      pBandList = pband_list;
    }
    else {
      myBandList = true;
      nBandCount = GDALGetRasterCount( self );
      pBandList = (int*) CPLMalloc( sizeof(int) * nBandCount );
      for( int i = 0; i< nBandCount; ++i ) {
        pBandList[i] = i;
      }
    }
    return GDALDatasetRasterIO( self, GF_Write, xoff, yoff, xsize, ysize,
                                (void*) buf_string, nxsize, nysize, ntype,
                                band_list, pband_list, 0, 0, 0 );
    if ( myBandList ) {
       CPLFree( pBandList );
    }
  }
%clear (int band_list, int *pband_list );
%clear (GDALDataType *buf_type);
%clear (int*);
%clear (int buf_len, char *buf_string);
#endif

#if !defined(SWIGCSHARP) && !defined(SWIGJAVA)
%feature("kwargs") ReadRaster;
%apply (int *optional_int) { (GDALDataType *buf_type) };
%apply (int nList, int *pList ) { (int band_list, int *pband_list ) };
%apply ( int *nLen, char **pBuf ) { (int *buf_len, char **buf ) };
%apply ( int *optional_int ) {(int*)};                            
CPLErr ReadRaster(  int xoff, int yoff, int xsize, int ysize,
	              int *buf_len, char **buf,
                      int *buf_xsize = 0, int *buf_ysize = 0,
                      GDALDataType *buf_type = 0,
                      int band_list = 0, int *pband_list = 0  )
{

    int nxsize = (buf_xsize==0) ? xsize : *buf_xsize;
    int nysize = (buf_ysize==0) ? ysize : *buf_ysize;
    GDALDataType ntype;
    if ( buf_type != 0 ) {
      ntype = (GDALDataType) *buf_type;
    } else {
      int lastband = GDALGetRasterCount( self ) - 1;
      ntype = GDALGetRasterDataType( GDALGetRasterBand( self, lastband ) );
    }
    bool myBandList = false;
    int nBandCount;
    int *pBandList;
    if ( band_list != 0 ) {
      myBandList = false;
      nBandCount = band_list;
      pBandList = pband_list;
    }
    else {
      myBandList = true;
      nBandCount = GDALGetRasterCount( self );
      pBandList = (int*) CPLMalloc( sizeof(int) * nBandCount );
      for( int i = 0; i< nBandCount; ++i ) {
        pBandList[i] = i;
      }
    }
                            
    return DSReadRaster_internal( self, xoff, yoff, xsize, ysize,
                                nxsize, nysize, ntype,
                                buf_len, buf, 
                                nBandCount, pBandList);
    if ( myBandList ) {
       CPLFree( pBandList );
    }

}
  
%clear (int *buf_len, char **buf );
%clear (int*);
#endif


/* NEEDED */
/* GetSubDatasets */
/* ReadAsArray */
/* AddBand */
/* AdviseRead */
/* ReadRaster */

#if !defined(SWIGCSHARP) && !defined(SWIGJAVA)
%feature("kwargs") BeginAsyncRasterIO;
%apply (int nList, int *pList ) { (int band_list, int *pband_list ) };
%apply (int *nLength, char **pBuffer ) { (int *buf_len, char **buf ) };
%apply(int *optional_int) { (int*) };  
  GDALAsyncRasterIOShadow* BeginAsyncRasterIO(int xOff, int yOff, int xSize, int ySize, int *buf_len, char **buf, \
								int buf_xsize, int buf_ysize, GDALDataType bufType = (GDALDataType)0, int band_list = 0, int *pband_list = 0, int nPixelSpace = 0, \
                                int nLineSpace = 0, int nBandSpace = 0, char **options = 0)  {
    
    if ((options != NULL) && (buf_xsize ==0) && (buf_ysize == 0))
    {
        // calculate an appropriate buffer size
        const char* pszLevel = CSLFetchNameValue(options, "LEVEL");
        if (pszLevel)
        {
            // round up
            int nLevel = atoi(pszLevel);
            int nRes = 2 << (nLevel - 1);
            buf_xsize = ceil(xSize / (1.0 * nRes));
            buf_ysize = ceil(ySize / (1.0 * nRes));
        }
    }
    
    
    int nxsize = (buf_xsize == 0) ? xSize : buf_xsize;
    int nysize = (buf_ysize == 0) ? ySize : buf_ysize;
    
    GDALDataType ntype;
    if (bufType != 0) {
        ntype = (GDALDataType) bufType;
    } 
    else {
        ntype = GDT_Byte;
    }
    
    bool myBandList = false;
    int nBCount;
    int* pBandList;
    
    if (band_list != 0){
        myBandList = false;
        nBCount = band_list;
        pBandList = pband_list;
    }        
    else
    {
        myBandList = true;
        nBCount = GDALGetRasterCount(self);
        pBandList = (int*)CPLMalloc(sizeof(int) * nBCount);
        for (int i = 0; i < nBCount; ++i) {
            pBandList[i] = i;
        }
    }
    
    // for python bindings create buffer 
    if (*buf == NULL)
    {
        // calculate buffer size
        // if type is byte typeSize is GDT_Int32 (4) since these are packed into an int (BGRA)
        if (ntype == GDT_Byte)
            *buf = (char*) VSIMalloc( nxsize * nysize * (int)GDALGetDataTypeSize(GDT_Int32) );
        else
            *buf = (char*) VSIMalloc( nxsize * nysize *  (int)GDALGetDataTypeSize(ntype)); 
    }
        
    // check we were able to create the buffer
    if (*buf)
    {
        return (GDALAsyncRasterIO*) GDALBeginAsyncRasterIO(self, xOff, yOff, xSize, ySize, (void*) *buf, nxsize, nysize, ntype, nBCount, pBandList, nPixelSpace, nLineSpace,
        nBandSpace, options);
    }
    else
    {
        *buf = 0;
        return NULL;
    }
    
    if ( myBandList ) {
       CPLFree( pBandList );
    }

  }

  %clear(int nBands, int *pband_list);
  %clear(int *buf_len, char **buf);
  %clear(int*);
#endif  

  void EndAsyncRasterIO(GDALAsyncRasterIOShadow* ario){
    GDALEndAsyncRasterIO(self, (GDALAsyncRasterIOH) ario);
  }

} /* extend */
}; /* GDALDatasetShadow */

%{
int GDALDatasetShadow_RasterXSize_get( GDALDatasetShadow *h ) {
  return GDALGetRasterXSize( h );
}
int GDALDatasetShadow_RasterYSize_get( GDALDatasetShadow *h ) {
  return GDALGetRasterYSize( h );
}
int GDALDatasetShadow_RasterCount_get( GDALDatasetShadow *h ) {
  return GDALGetRasterCount( h );
}
int GDALAsyncRasterIOShadow_XOff_get(GDALAsyncRasterIOShadow *h){
    return GDALGetXOffset(h);
}
int GDALAsyncRasterIOShadow_YOff_get(GDALAsyncRasterIOShadow *h){
    return GDALGetYOffset(h);
}
int GDALAsyncRasterIOShadow_XSize_get(GDALAsyncRasterIOShadow *h){
    return GDALGetXSize(h);
}
int GDALAsyncRasterIOShadow_YSize_get(GDALAsyncRasterIOShadow *h){
    return GDALGetYSize(h);
}
void * GDALAsyncRasterIOShadow_Buffer_get(GDALAsyncRasterIOShadow *h){
    return GDALGetBuffer(h);
}
int GDALAsyncRasterIOShadow_BufferXSize_get(GDALAsyncRasterIOShadow *h){
    return GDALGetBufferXSize(h);
}
int GDALAsyncRasterIOShadow_BufferYSize_get(GDALAsyncRasterIOShadow *h){
    return GDALGetBufferYSize(h);
}
GDALDataType GDALAsyncRasterIOShadow_BufferType_get(GDALAsyncRasterIOShadow *h){
    return GDALGetBufferType(h);
}
int GDALAsyncRasterIOShadow_BandCount_get(GDALAsyncRasterIOShadow *h){
    return GDALGetBandCount(h);
}

int GDALAsyncRasterIOShadow_NDataRead_get(GDALAsyncRasterIOShadow *h){
    return GDALGetNDataRead(h);
}

int GDALAsyncRasterIOShadow_PixelSpace_get(GDALAsyncRasterIOShadow *h){
    return GDALGetPixelSpace(h);
}
int GDALAsyncRasterIOShadow_LineSpace_get(GDALAsyncRasterIOShadow *h){
    return GDALGetLineSpace(h);
}
int GDALAsyncRasterIOShadow_BandSpace_get(GDALAsyncRasterIOShadow *h){
    return GDALGetBandSpace(h);
}
%}
