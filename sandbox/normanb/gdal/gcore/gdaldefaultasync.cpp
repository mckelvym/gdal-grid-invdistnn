/******************************************************************************
 * $Id: gdaldataset.cpp 16796 2009-04-17 23:35:04Z normanb $
 *
 * Project:  GDAL Core
 * Purpose:  Implementation of GDALDefaultAsyncRasterIO and the 
 *           GDALAsyncRasterIO base class.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2010, Frank Warmerdam
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
 ****************************************************************************/

#include "gdal_priv.h"

CPL_CVSID("$Id: gdaldataset.cpp 16796 2009-04-17 23:35:04Z normanb $");

CPL_C_START
GDALAsyncRasterIO *
GDALGetDefaultAsyncRasterIO( GDALDataset* poDS,
                             int nXOff, int nYOff,
                             int nXSize, int nYSize,
                             void *pBuf,
                             int nBufXSize, int nBufYSize,
                             GDALDataType eBufType,
                             int nBandCount, int* panBandMap,
                             int nPixelSpace, int nLineSpace,
                             int nBandSpace, char **papszOptions);
CPL_C_END

/************************************************************************/
/* ==================================================================== */
/*                         GDALAsyncRasterIO                            */
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                         GDALAsyncRasterIO()                          */
/************************************************************************/

GDALAsyncRasterIO::GDALAsyncRasterIO()
{
}

/************************************************************************/
/*                         ~GDALAsyncRasterIO()                         */
/************************************************************************/
GDALAsyncRasterIO::~GDALAsyncRasterIO()
{
}

/************************************************************************/
/*                        GetNextUpdatedRegion()                        */
/************************************************************************/

/**
 * \fn GDALAsyncStatusType GDALAsyncRasterIO::GetNextUpdatedRegion( int nTimeout, int* pnBufXOff, int* pnBufYOff, int* pnBufXSize, int* pnBufXSize) = 0;
 * 
 * \brief Get async IO update
 *
 * Provide an opportunity for an asynchronous IO request to update the
 * image buffer and return an indication of the area of the buffer that
 * has been updated.  
 *
 * The nTimeout parameter can be used to wait for additional data to 
 * become available.  The timeout does not limit the amount
 * of time this method may spend actually processing available data.
 *
 * The following return status are possible.
 * - GARIO_PENDING: No imagery was altered in the buffer, but there is still 
 * activity pending, and the application should continue to call 
 * GetNextUpdatedRegion() as time permits.
 * - GARIO_UPDATE: Some of the imagery has been updated, but there is still 
 * activity pending.
 * - GARIO_ERROR: Something has gone wrong. The asynchronous request should 
 * be ended.
 * - GARIO_COMPLETE: An update has occured and there is no more pending work 
 * on this request. The request should be ended and the buffer used. 
 *
 * @param nTimeout the number of milliseconds to wait for additional updates, 
 * or zero if no waiting should be done.
 * @param pnBufXOff location to return the X offset of the area of the
 * request buffer that has been updated.
 * @param pnBufYOff location to return the Y offset of the area of the
 * request buffer that has been updated.
 * @param pnBufXSize location to return the X size of the area of the
 * request buffer that has been updated.
 * @param pnBufYSize location to return the Y size of the area of the
 * request buffer that has been updated.
 * 
 * @return GARIO_ status, details described above. 
 */

/************************************************************************/
/*                      GDALGetNextUpdatedRegion()                      */
/************************************************************************/

GDALAsyncStatusType CPL_STDCALL 
GDALGetNextUpdatedRegion(GDALAsyncRasterIOH hARIO, int timeout,
                         int* pnxbufoff, int* pnybufoff, 
                         int* pnxbufsize, int* pnybufsize)
{
    VALIDATE_POINTER1(hARIO, "GDALGetNextUpdatedRegion", GARIO_ERROR);
    return ((GDALAsyncRasterIO *)hARIO)->GetNextUpdatedRegion(
        timeout, pnxbufoff, pnybufoff, pnxbufsize, pnybufsize);
}

/************************************************************************/
/*                             LockBuffer()                             */
/************************************************************************/

/**
 * \brief Lock image buffer.
 *
 * Locks the image buffer passed into GDALDataset::BeginAsyncRasterIO(). 
 * This is useful to ensure the image buffer is not being modified while
 * it is being used by the application.  UnlockBuffer() should be used
 * to release this lock when it is no longer needed.
 */

void GDALAsyncRasterIO::LockBuffer()

{
}


/************************************************************************/
/*                           GDALLockBuffer()                           */
/************************************************************************/
void CPL_STDCALL GDALLockBuffer(GDALAsyncRasterIOH hARIO)
{
    VALIDATE_POINTER0(hARIO, "GDALAsyncRasterIO");
    ((GDALAsyncRasterIO *)hARIO) ->LockBuffer();
}

/************************************************************************/
/*                            UnlockBuffer()                            */
/************************************************************************/

/**
 * \brief Unlock image buffer.
 *
 * Releases a lock on the image buffer previously taken with LockBuffer().
 */

void GDALAsyncRasterIO::UnlockBuffer()

{
}

/************************************************************************/
/*                          GDALUnlockBuffer()                          */
/************************************************************************/
void CPL_STDCALL GDALUnlockBuffer(GDALAsyncRasterIOH hARIO)
{
    VALIDATE_POINTER0(hARIO, "GDALAsyncRasterIO");
    ((GDALAsyncRasterIO *)hARIO) ->UnlockBuffer();
}

/************************************************************************/
/* ==================================================================== */
/*                     GDALDefaultAsyncRasterIO                         */
/* ==================================================================== */
/************************************************************************/

class GDALDefaultAsyncRasterIO : public GDALAsyncRasterIO
{
  private:
    char **         papszOptions;

  public:
    GDALDefaultAsyncRasterIO(GDALDataset* poDS,
                             int nXOff, int nYOff,
                             int nXSize, int nYSize,
                             void *pBuf,
                             int nBufXSize, int nBufYSize,
                             GDALDataType eBufType,
                             int nBandCount, int* panBandMap,
                             int nPixelSpace, int nLineSpace,
                             int nBandSpace, char **papszOptions);
    ~GDALDefaultAsyncRasterIO();

    virtual GDALAsyncStatusType GetNextUpdatedRegion(int nTimeout,
                                                     int* pnBufXOff,
                                                     int* pnBufYOff,
                                                     int* pnBufXSize,
                                                     int* pnBufYSize);
};

/************************************************************************/
/*                    GDALGetDefaultAsyncRasterIO()                     */
/************************************************************************/

GDALAsyncRasterIO *
GDALGetDefaultAsyncRasterIO( GDALDataset* poDS,
                             int nXOff, int nYOff,
                             int nXSize, int nYSize,
                             void *pBuf,
                             int nBufXSize, int nBufYSize,
                             GDALDataType eBufType,
                             int nBandCount, int* panBandMap,
                             int nPixelSpace, int nLineSpace,
                             int nBandSpace, char **papszOptions)

{
    return new GDALDefaultAsyncRasterIO( poDS,
                                         nXOff, nYOff, nXSize, nYSize,
                                         pBuf, nBufXSize, nBufYSize, eBufType,
                                         nBandCount, panBandMap,
                                         nPixelSpace, nLineSpace, nBandSpace,
                                         papszOptions );
}

/************************************************************************/
/*                      GDALDefaultAsyncRasterIO()                      */
/************************************************************************/

GDALDefaultAsyncRasterIO::
GDALDefaultAsyncRasterIO( GDALDataset* poDS,
                          int nXOff, int nYOff,
                          int nXSize, int nYSize,
                          void *pBuf,
                          int nBufXSize, int nBufYSize,
                          GDALDataType eBufType,
                          int nBandCount, int* panBandMap,
                          int nPixelSpace, int nLineSpace,
                          int nBandSpace, char **papszOptions) 

{
    this->poDS = poDS;
    this->nXOff = nXOff;
    this->nYOff = nYOff;
    this->nXSize = nXSize;
    this->nYSize = nYSize;
    this->pBuf = pBuf;
    this->nBufXSize = nBufXSize;
    this->nBufYSize = nBufYSize;
    this->eBufType = eBufType;
    this->nBandCount = nBandCount;
    this->panBandMap = (int *) CPLMalloc(sizeof(int)*nBandCount);

    if( panBandMap != NULL )
        memcpy( this->panBandMap, panBandMap, sizeof(int)*nBandCount );
    else
    {
        for( int i; i < nBandCount; i++ )
            this->panBandMap[i] = i+1;
    }
    
    this->nPixelSpace = nPixelSpace;
    this->nLineSpace = nLineSpace;
    this->nBandSpace = nBandSpace;

    this->papszOptions = CSLDuplicate(papszOptions);
}

/************************************************************************/
/*                     ~GDALDefaultAsyncRasterIO()                      */
/************************************************************************/

GDALDefaultAsyncRasterIO::~GDALDefaultAsyncRasterIO()

{
    CPLFree( panBandMap );
    CSLDestroy( papszOptions );
}

/************************************************************************/
/*                        GetNextUpdatedRegion()                        */
/************************************************************************/

GDALAsyncStatusType
GDALDefaultAsyncRasterIO::GetNextUpdatedRegion(int nTimeout,
                                               int* pnBufXOff,
                                               int* pnBufYOff,
                                               int* pnBufXSize,
                                               int* pnBufYSize )

{
    CPLErr eErr;

    eErr = poDS->RasterIO( GF_Read, nXOff, nYOff, nXSize, nYSize, 
                           pBuf, nBufXSize, nBufYSize, eBufType, 
                           nBandCount, panBandMap, 
                           nPixelSpace, nLineSpace, nBandSpace );

    *pnBufXOff = 0;
    *pnBufYOff = 0;
    *pnBufXSize = nBufXSize;
    *pnBufYSize = nBufYSize;

    if( eErr == CE_None )
        return GARIO_COMPLETE;
    else
        return GARIO_ERROR;
}

