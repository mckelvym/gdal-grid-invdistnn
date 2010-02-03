/******************************************************************************
 * $Id: gdaldataset.cpp 16796 2009-04-17 23:35:04Z normanb $
 *
 * Project:  GDAL Core
 * Purpose:  Implementation of GDALDefaultAsyncRasterIO
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

    virtual GDALAsyncStatusType GetNextUpdatedRegion(int bWait, int nTimeout,
                                                     int* pnBufXOff,
                                                     int* pnBufYOff,
                                                     int* pnBufXSize,
                                                     int* pnBufYSize);

    virtual void LockBuffer() {}
    virtual void LockBuffer(int nBufXOff, int nBufYOff, 
                            int nBufXSize, int nBufYSize) {}
    virtual void UnlockBuffer() {} 
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
GDALDefaultAsyncRasterIO::GetNextUpdatedRegion(int bWait, int nTimeout,
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

