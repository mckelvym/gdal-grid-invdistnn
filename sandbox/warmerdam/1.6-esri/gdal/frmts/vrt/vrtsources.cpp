/******************************************************************************
 * $Id$
 *
 * Project:  Virtual GDAL Datasets
 * Purpose:  Implementation of VRTSimpleSource, VRTFuncSource and 
 *           VRTAveragedSource.
 * Author:   Frank Warmerdam <warmerdam@pobox.com>
 *
 ******************************************************************************
 * Copyright (c) 2001, Frank Warmerdam <warmerdam@pobox.com>
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

#include "vrtdataset.h"
#include "gdal_proxy.h"
#include "cpl_minixml.h"
#include "cpl_string.h"

CPL_CVSID("$Id$");

/************************************************************************/
/* ==================================================================== */
/*                             VRTSource                                */
/* ==================================================================== */
/************************************************************************/

VRTSource::~VRTSource()
{
}

/************************************************************************/
/* ==================================================================== */
/*                          VRTSimpleSource                             */
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                          VRTSimpleSource()                           */
/************************************************************************/

VRTSimpleSource::VRTSimpleSource()

{
    poRasterBand = NULL;
    bNoDataSet = FALSE;
}

/************************************************************************/
/*                          ~VRTSimpleSource()                          */
/************************************************************************/

VRTSimpleSource::~VRTSimpleSource()

{
    if( poRasterBand != NULL && poRasterBand->GetDataset() != NULL )
    {
        if( poRasterBand->GetDataset()->GetShared() )
            GDALClose( (GDALDatasetH) poRasterBand->GetDataset() );
        else
            poRasterBand->GetDataset()->Dereference();
    }
}

/************************************************************************/
/*                             SetSrcBand()                             */
/************************************************************************/

void VRTSimpleSource::SetSrcBand( GDALRasterBand *poNewSrcBand )

{
    poRasterBand = poNewSrcBand;
}

/************************************************************************/
/*                            SetSrcWindow()                            */
/************************************************************************/

void VRTSimpleSource::SetSrcWindow( int nNewXOff, int nNewYOff, 
                                    int nNewXSize, int nNewYSize )

{
    nSrcXOff = nNewXOff;
    nSrcYOff = nNewYOff;
    nSrcXSize = nNewXSize;
    nSrcYSize = nNewYSize;
}

/************************************************************************/
/*                            SetDstWindow()                            */
/************************************************************************/

void VRTSimpleSource::SetDstWindow( int nNewXOff, int nNewYOff, 
                                    int nNewXSize, int nNewYSize )

{
    nDstXOff = nNewXOff;
    nDstYOff = nNewYOff;
    nDstXSize = nNewXSize;
    nDstYSize = nNewYSize;
}

/************************************************************************/
/*                           SetNoDataValue()                           */
/************************************************************************/

void VRTSimpleSource::SetNoDataValue( double dfNewNoDataValue )

{
    if( dfNewNoDataValue == VRT_NODATA_UNSET )
    {
        bNoDataSet = FALSE;
        dfNoDataValue = VRT_NODATA_UNSET;
    }
    else
    {
        bNoDataSet = TRUE;
        dfNoDataValue = dfNewNoDataValue;
    }
}

/************************************************************************/
/*                           SerializeToXML()                           */
/************************************************************************/

CPLXMLNode *VRTSimpleSource::SerializeToXML( const char *pszVRTPath )

{
    CPLXMLNode      *psSrc;
    int              bRelativeToVRT;
    const char      *pszRelativePath;
    int              nBlockXSize, nBlockYSize;

    if( poRasterBand == NULL )
        return NULL;

    GDALDataset     *poDS = poRasterBand->GetDataset();

    if( poDS == NULL || poRasterBand->GetBand() < 1 )
        return NULL;

    psSrc = CPLCreateXMLNode( NULL, CXT_Element, "SimpleSource" );

    pszRelativePath = 
        CPLExtractRelativePath( pszVRTPath, poDS->GetDescription(), 
                                &bRelativeToVRT );
    
    CPLSetXMLValue( psSrc, "SourceFilename", pszRelativePath );
    
    CPLCreateXMLNode( 
        CPLCreateXMLNode( CPLGetXMLNode( psSrc, "SourceFilename" ), 
                          CXT_Attribute, "relativeToVRT" ), 
        CXT_Text, bRelativeToVRT ? "1" : "0" );

    CPLSetXMLValue( psSrc, "SourceBand", 
                    CPLSPrintf("%d",poRasterBand->GetBand()) );

    /* Write a few additional useful properties of the dataset */
    /* so that we can use a proxy dataset when re-opening. See XMLInit() */
    /* below */
    CPLSetXMLValue( psSrc, "SourceProperties.#RasterXSize", 
                    CPLSPrintf("%d",poRasterBand->GetXSize()) );
    CPLSetXMLValue( psSrc, "SourceProperties.#RasterYSize", 
                    CPLSPrintf("%d",poRasterBand->GetYSize()) );
    CPLSetXMLValue( psSrc, "SourceProperties.#DataType", 
                GDALGetDataTypeName( poRasterBand->GetRasterDataType() ) );
    poRasterBand->GetBlockSize(&nBlockXSize, &nBlockYSize);
    CPLSetXMLValue( psSrc, "SourceProperties.#BlockXSize", 
                    CPLSPrintf("%d",nBlockXSize) );
    CPLSetXMLValue( psSrc, "SourceProperties.#BlockYSize", 
                    CPLSPrintf("%d",nBlockYSize) );

    if( nSrcXOff != -1 
        || nSrcYOff != -1 
        || nSrcXSize != -1 
        || nSrcYSize != -1 )
    {
        CPLSetXMLValue( psSrc, "SrcRect.#xOff", 
                        CPLSPrintf( "%d", nSrcXOff ) );
        CPLSetXMLValue( psSrc, "SrcRect.#yOff", 
                        CPLSPrintf( "%d", nSrcYOff ) );
        CPLSetXMLValue( psSrc, "SrcRect.#xSize", 
                        CPLSPrintf( "%d", nSrcXSize ) );
        CPLSetXMLValue( psSrc, "SrcRect.#ySize", 
                        CPLSPrintf( "%d", nSrcYSize ) );
    }

    if( nDstXOff != -1 
        || nDstYOff != -1 
        || nDstXSize != -1 
        || nDstYSize != -1 )
    {
        CPLSetXMLValue( psSrc, "DstRect.#xOff", CPLSPrintf( "%d", nDstXOff ) );
        CPLSetXMLValue( psSrc, "DstRect.#yOff", CPLSPrintf( "%d", nDstYOff ) );
        CPLSetXMLValue( psSrc, "DstRect.#xSize",CPLSPrintf( "%d", nDstXSize ));
        CPLSetXMLValue( psSrc, "DstRect.#ySize",CPLSPrintf( "%d", nDstYSize ));
    }

    return psSrc;
}

/************************************************************************/
/*                              XMLInit()                               */
/************************************************************************/

CPLErr VRTSimpleSource::XMLInit( CPLXMLNode *psSrc, const char *pszVRTPath )

{
/* -------------------------------------------------------------------- */
/*      Prepare filename.                                               */
/* -------------------------------------------------------------------- */
    char *pszSrcDSName = NULL;
    CPLXMLNode* psSourceFileNameNode = CPLGetXMLNode(psSrc,"SourceFilename");
    const char *pszFilename = 
        psSourceFileNameNode ? CPLGetXMLValue(psSourceFileNameNode,NULL, NULL) : NULL;

    if( pszFilename == NULL )
    {
        CPLError( CE_Warning, CPLE_AppDefined, 
                  "Missing <SourceFilename> element in VRTRasterBand." );
        return CE_Failure;
    }
    
    if( pszVRTPath != NULL
        && atoi(CPLGetXMLValue( psSourceFileNameNode, "relativetoVRT", "0")) )
    {
        pszSrcDSName = CPLStrdup(
            CPLProjectRelativeFilename( pszVRTPath, pszFilename ) );
    }
    else
        pszSrcDSName = CPLStrdup( pszFilename );


    int nSrcBand = atoi(CPLGetXMLValue(psSrc,"SourceBand","1"));

    /* Newly generated VRT will have RasterXSize, RasterYSize, DataType, */
    /* BlockXSize, BlockYSize tags, so that we don't have actually to */
    /* open the real dataset immediately, but we can use a proxy dataset */
    /* instead. This is particularly usefull when dealing with huge VRT */
    /* For example, a VRT with the world coverage of DTED0 (25594 files) */
    CPLXMLNode* psSrcProperties = CPLGetXMLNode(psSrc,"SourceProperties");
    int nRasterXSize = 0, nRasterYSize =0;
    GDALDataType eDataType = (GDALDataType)-1;
    int nBlockXSize = 0, nBlockYSize = 0;
    if (psSrcProperties)
    {
        nRasterXSize = atoi(CPLGetXMLValue(psSrcProperties,"RasterXSize","0"));
        nRasterYSize = atoi(CPLGetXMLValue(psSrcProperties,"RasterYSize","0"));
        const char *pszDataType = CPLGetXMLValue(psSrcProperties, "DataType", NULL);
        if( pszDataType != NULL )
        {
            for( int iType = 0; iType < GDT_TypeCount; iType++ )
            {
                const char *pszThisName = GDALGetDataTypeName((GDALDataType)iType);

                if( pszThisName != NULL && EQUAL(pszDataType,pszThisName) )
                {
                    eDataType = (GDALDataType) iType;
                    break;
                }
            }
        }
        nBlockXSize = atoi(CPLGetXMLValue(psSrcProperties,"BlockXSize","0"));
        nBlockYSize = atoi(CPLGetXMLValue(psSrcProperties,"BlockYSize","0"));
    }

    GDALDataset *poSrcDS;
    if (nRasterXSize == 0 || nRasterYSize == 0 || eDataType == (GDALDataType)-1 ||
        nBlockXSize == 0 || nBlockYSize == 0)
    {
        /* -------------------------------------------------------------------- */
        /*      Open the file (shared).                                         */
        /* -------------------------------------------------------------------- */
        poSrcDS = (GDALDataset *) GDALOpenShared( pszSrcDSName, GA_ReadOnly );
    }
    else
    {
        /* -------------------------------------------------------------------- */
        /*      Create a proxy dataset                                          */
        /* -------------------------------------------------------------------- */
        int i;
        GDALProxyPoolDataset* proxyDS = new GDALProxyPoolDataset(pszSrcDSName, nRasterXSize, nRasterYSize, GA_ReadOnly, TRUE);
        poSrcDS = proxyDS;

        /* Only the information of rasterBand nSrcBand will be accurate */
        /* but that's OK since we only use that band afterwards */
        for(i=1;i<=nSrcBand;i++)
            proxyDS->AddSrcBandDescription(eDataType, nBlockXSize, nBlockYSize);
    }

    CPLFree( pszSrcDSName );
    
    if( poSrcDS == NULL )
        return CE_Failure;

/* -------------------------------------------------------------------- */
/*      Get the raster band.                                            */
/* -------------------------------------------------------------------- */
    
    poRasterBand = poSrcDS->GetRasterBand(nSrcBand);
    if( poRasterBand == NULL )
        return CE_Failure;
    
/* -------------------------------------------------------------------- */
/*      Set characteristics.                                            */
/* -------------------------------------------------------------------- */
    CPLXMLNode* psSrcRect = CPLGetXMLNode(psSrc,"SrcRect");
    if (psSrcRect)
    {
        nSrcXOff = atoi(CPLGetXMLValue(psSrcRect,"xOff","-1"));
        nSrcYOff = atoi(CPLGetXMLValue(psSrcRect,"yOff","-1"));
        nSrcXSize = atoi(CPLGetXMLValue(psSrcRect,"xSize","-1"));
        nSrcYSize = atoi(CPLGetXMLValue(psSrcRect,"ySize","-1"));
    }
    else
    {
        nSrcXOff = nSrcYOff = nSrcXSize = nSrcYSize = -1;
    }

    CPLXMLNode* psDstRect = CPLGetXMLNode(psSrc,"DstRect");
    if (psDstRect)
    {
        nDstXOff = atoi(CPLGetXMLValue(psDstRect,"xOff","-1"));
        nDstYOff = atoi(CPLGetXMLValue(psDstRect,"yOff","-1"));
        nDstXSize = atoi(CPLGetXMLValue(psDstRect,"xSize","-1"));
        nDstYSize = atoi(CPLGetXMLValue(psDstRect,"ySize","-1"));
    }
    else
    {
        nDstXOff = nDstYOff = nDstXSize = nDstYSize = -1;
    }

    return CE_None;
}

/************************************************************************/
/*                              SrcToDst()                              */
/*                                                                      */
/*      Note: this is a no-op if the dst window is -1,-1,-1,-1.         */
/************************************************************************/

void VRTSimpleSource::SrcToDst( double dfX, double dfY,
                                double &dfXOut, double &dfYOut )

{
    dfXOut = ((dfX - nSrcXOff) / nSrcXSize) * nDstXSize + nDstXOff;
    dfYOut = ((dfY - nSrcYOff) / nSrcYSize) * nDstYSize + nDstYOff;
}

/************************************************************************/
/*                              DstToSrc()                              */
/*                                                                      */
/*      Note: this is a no-op if the dst window is -1,-1,-1,-1.         */
/************************************************************************/

void VRTSimpleSource::DstToSrc( double dfX, double dfY,
                                double &dfXOut, double &dfYOut )

{
    dfXOut = ((dfX - nDstXOff) / nDstXSize) * nSrcXSize + nSrcXOff;
    dfYOut = ((dfY - nDstYOff) / nDstYSize) * nSrcYSize + nSrcYOff;
}

/************************************************************************/
/*                          GetSrcDstWindow()                           */
/************************************************************************/

int 
VRTSimpleSource::GetSrcDstWindow( int nXOff, int nYOff, int nXSize, int nYSize,
                                  int nBufXSize, int nBufYSize,
                                  int *pnReqXOff, int *pnReqYOff,
                                  int *pnReqXSize, int *pnReqYSize,
                                  int *pnOutXOff, int *pnOutYOff, 
                                  int *pnOutXSize, int *pnOutYSize )

{
    int bDstWinSet = nDstXOff != -1 || nDstXSize != -1 
        || nDstYOff != -1 || nDstYSize != -1;

#ifdef DEBUG
    int bSrcWinSet = nSrcXOff != -1 || nSrcXSize != -1 
        || nSrcYOff != -1 || nSrcYSize != -1;

    CPLAssert( bSrcWinSet == bDstWinSet );
#endif

/* -------------------------------------------------------------------- */
/*      If the input window completely misses the portion of the        */
/*      virtual dataset provided by this source we have nothing to do.  */
/* -------------------------------------------------------------------- */
    if( bDstWinSet )
    {
        if( nXOff > nDstXOff + nDstXSize
            || nYOff > nDstYOff + nDstYSize
            || nXOff + nXSize < nDstXOff
            || nYOff + nYSize < nDstYOff )
            return FALSE;
    }

/* -------------------------------------------------------------------- */
/*      This request window corresponds to the whole output buffer.     */
/* -------------------------------------------------------------------- */
    *pnOutXOff = 0;
    *pnOutYOff = 0;
    *pnOutXSize = nBufXSize;
    *pnOutYSize = nBufYSize;

/* -------------------------------------------------------------------- */
/*      If the input window extents outside the portion of the on       */
/*      the virtual file that this source can set, then clip down       */
/*      the requested window.                                           */
/* -------------------------------------------------------------------- */
    int bModifiedX = FALSE, bModifiedY = FALSE;
    int nRXOff = nXOff;
    int nRYOff = nYOff;
    int nRXSize = nXSize;
    int nRYSize = nYSize;


    if( bDstWinSet )
    {
        if( nRXOff < nDstXOff )
        {
            nRXSize = nRXSize + nRXOff - nDstXOff;
            nRXOff = nDstXOff;
            bModifiedX = TRUE;
        }

        if( nRYOff < nDstYOff )
        {
            nRYSize = nRYSize + nRYOff - nDstYOff;
            nRYOff = nDstYOff;
            bModifiedY = TRUE;
        }

        if( nRXOff + nRXSize > nDstXOff + nDstXSize )
        {
            nRXSize = nDstXOff + nDstXSize - nRXOff;
            bModifiedX = TRUE;
        }

        if( nRYOff + nRYSize > nDstYOff + nDstYSize )
        {
            nRYSize = nDstYOff + nDstYSize - nRYOff;
            bModifiedY = TRUE;
        }
    }

/* -------------------------------------------------------------------- */
/*      Translate requested region in virtual file into the source      */
/*      band coordinates.                                               */
/* -------------------------------------------------------------------- */
    double      dfScaleX = nSrcXSize / (double) nDstXSize;
    double      dfScaleY = nSrcYSize / (double) nDstYSize;

    *pnReqXOff = (int) floor((nRXOff - nDstXOff) * dfScaleX + nSrcXOff);
    *pnReqYOff = (int) floor((nRYOff - nDstYOff) * dfScaleY + nSrcYOff);

    *pnReqXSize = (int) floor(nRXSize * dfScaleX + 0.5);
    *pnReqYSize = (int) floor(nRYSize * dfScaleY + 0.5);

/* -------------------------------------------------------------------- */
/*      Clamp within the bounds of the available source data.           */
/* -------------------------------------------------------------------- */
    if( *pnReqXOff < 0 )
    {
        *pnReqXSize += *pnReqXOff;
        *pnReqXOff = 0;

        bModifiedX = TRUE;
    }
    
    if( *pnReqYOff < 0 )
    {
        *pnReqYSize += *pnReqYOff;
        *pnReqYOff = 0;
        bModifiedY = TRUE;
    }

    if( *pnReqXSize == 0 )
        *pnReqXSize = 1;
    if( *pnReqYSize == 0 )
        *pnReqYSize = 1;

    if( *pnReqXOff + *pnReqXSize > poRasterBand->GetXSize() )
    {
        *pnReqXSize = poRasterBand->GetXSize() - *pnReqXOff;
        bModifiedX = TRUE;
    }

    if( *pnReqYOff + *pnReqYSize > poRasterBand->GetYSize() )
    {
        *pnReqYSize = poRasterBand->GetYSize() - *pnReqYOff;
        bModifiedY = TRUE;
    }

/* -------------------------------------------------------------------- */
/*      Don't do anything if the requesting region is completely off    */
/*      the source image.                                               */
/* -------------------------------------------------------------------- */
    if( *pnReqXOff >= poRasterBand->GetXSize()
        || *pnReqYOff >= poRasterBand->GetYSize()
        || *pnReqXSize <= 0 || *pnReqYSize <= 0 )
    {
        return FALSE;
    }

/* -------------------------------------------------------------------- */
/*      If we haven't had to modify the source rectangle, then the      */
/*      destination rectangle must be the whole region.                 */
/* -------------------------------------------------------------------- */
    if( !bModifiedX && !bModifiedY )
        return TRUE;

/* -------------------------------------------------------------------- */
/*      Now transform this possibly reduced request back into the       */
/*      destination buffer coordinates in case the output region is     */
/*      less than the whole buffer.                                     */
/* -------------------------------------------------------------------- */
    double dfDstULX, dfDstULY, dfDstLRX, dfDstLRY;
    double dfScaleWinToBufX, dfScaleWinToBufY;

    SrcToDst( (double) *pnReqXOff, (double) *pnReqYOff, dfDstULX, dfDstULY );
    SrcToDst( *pnReqXOff + *pnReqXSize, *pnReqYOff + *pnReqYSize, 
              dfDstLRX, dfDstLRY );

    if( bModifiedX )
    {
        dfScaleWinToBufX = nBufXSize / (double) nXSize;

        *pnOutXOff = (int) ((dfDstULX - nXOff) * dfScaleWinToBufX+0.001);
        *pnOutXSize = (int) ((dfDstLRX - nXOff) * dfScaleWinToBufX+0.001) 
            - *pnOutXOff;

        *pnOutXOff = MAX(0,*pnOutXOff);
        if( *pnOutXOff + *pnOutXSize > nBufXSize )
            *pnOutXSize = nBufXSize - *pnOutXOff;
    }

    if( bModifiedY )
    {
        dfScaleWinToBufY = nBufYSize / (double) nYSize;

        *pnOutYOff = (int) ((dfDstULY - nYOff) * dfScaleWinToBufY+0.001);
        *pnOutYSize = (int) ((dfDstLRY - nYOff) * dfScaleWinToBufY+0.001) 
            - *pnOutYOff;

        *pnOutYOff = MAX(0,*pnOutYOff);
        if( *pnOutYOff + *pnOutYSize > nBufYSize )
            *pnOutYSize = nBufYSize - *pnOutYOff;
    }

    if( *pnOutXSize < 1 || *pnOutYSize < 1 )
        return FALSE;
    else
        return TRUE;
}

/************************************************************************/
/*                              RasterIO()                              */
/************************************************************************/

CPLErr 
VRTSimpleSource::RasterIO( int nXOff, int nYOff, int nXSize, int nYSize,
                           void *pData, int nBufXSize, int nBufYSize, 
                           GDALDataType eBufType, 
                           int nPixelSpace, int nLineSpace )

{
    // The window we will actually request from the source raster band.
    int nReqXOff, nReqYOff, nReqXSize, nReqYSize;

    // The window we will actual set _within_ the pData buffer.
    int nOutXOff, nOutYOff, nOutXSize, nOutYSize;

    if( !GetSrcDstWindow( nXOff, nYOff, nXSize, nYSize,
                          nBufXSize, nBufYSize, 
                          &nReqXOff, &nReqYOff, &nReqXSize, &nReqYSize,
                          &nOutXOff, &nOutYOff, &nOutXSize, &nOutYSize ) )
    {
        return CE_None;
    }

/* -------------------------------------------------------------------- */
/*      Actually perform the IO request.                                */
/* -------------------------------------------------------------------- */
    CPLErr eErr;

    eErr = 
        poRasterBand->RasterIO( GF_Read, 
                                nReqXOff, nReqYOff, nReqXSize, nReqYSize,
                                ((unsigned char *) pData) 
                                + nOutXOff * nPixelSpace
                                + nOutYOff * nLineSpace, 
                                nOutXSize, nOutYSize, 
                                eBufType, nPixelSpace, nLineSpace );

    return eErr;
}

/************************************************************************/
/* ==================================================================== */
/*                         VRTAveragedSource                            */
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                         VRTAveragedSource()                          */
/************************************************************************/

VRTAveragedSource::VRTAveragedSource()
{
}

/************************************************************************/
/*                           SerializeToXML()                           */
/************************************************************************/

CPLXMLNode *VRTAveragedSource::SerializeToXML( const char *pszVRTPath )

{
    CPLXMLNode *psSrc = VRTSimpleSource::SerializeToXML( pszVRTPath );

    if( psSrc == NULL )
        return NULL;

    CPLFree( psSrc->pszValue );
    psSrc->pszValue = CPLStrdup( "AveragedSource" );

    return psSrc;
}

/************************************************************************/
/*                              RasterIO()                              */
/************************************************************************/

CPLErr 
VRTAveragedSource::RasterIO( int nXOff, int nYOff, int nXSize, int nYSize,
                           void *pData, int nBufXSize, int nBufYSize, 
                           GDALDataType eBufType, 
                           int nPixelSpace, int nLineSpace )

{
    // The window we will actually request from the source raster band.
    int nReqXOff, nReqYOff, nReqXSize, nReqYSize;

    // The window we will actual set _within_ the pData buffer.
    int nOutXOff, nOutYOff, nOutXSize, nOutYSize;

    if( !GetSrcDstWindow( nXOff, nYOff, nXSize, nYSize, nBufXSize, nBufYSize, 
                          &nReqXOff, &nReqYOff, &nReqXSize, &nReqYSize,
                          &nOutXOff, &nOutYOff, &nOutXSize, &nOutYSize ) )
        return CE_None;

/* -------------------------------------------------------------------- */
/*      Allocate a temporary buffer to whole the full resolution        */
/*      data from the area of interest.                                 */
/* -------------------------------------------------------------------- */
    float *pafSrc;

    pafSrc = (float *) VSIMalloc3(sizeof(float), nReqXSize, nReqYSize);
    if( pafSrc == NULL )
    {
        CPLError( CE_Failure, CPLE_OutOfMemory, 
                  "Out of memory allocating working buffer in VRTAveragedSource::RasterIO()." );
        return CE_Failure;
    }

/* -------------------------------------------------------------------- */
/*      Load it.                                                        */
/* -------------------------------------------------------------------- */
    CPLErr eErr;
    
    eErr = poRasterBand->RasterIO( GF_Read, 
                                   nReqXOff, nReqYOff, nReqXSize, nReqYSize,
                                   pafSrc, nReqXSize, nReqYSize, GDT_Float32, 
                                   0, 0 );

    if( eErr != CE_None )
    {
        VSIFree( pafSrc );
        return eErr;
    }

/* -------------------------------------------------------------------- */
/*      Do the averaging.                                               */
/* -------------------------------------------------------------------- */
    for( int iBufLine = nOutYOff; iBufLine < nOutYOff + nOutYSize; iBufLine++ )
    {
        double  dfYDst;

        dfYDst = (iBufLine / (double) nBufYSize) * nYSize + nYOff;
        
        for( int iBufPixel = nOutXOff; 
             iBufPixel < nOutXOff + nOutXSize; 
             iBufPixel++ )
        {
            double dfXDst;
            double dfXSrcStart, dfXSrcEnd, dfYSrcStart, dfYSrcEnd;
            int    iXSrcStart, iYSrcStart, iXSrcEnd, iYSrcEnd;

            dfXDst = (iBufPixel / (double) nBufXSize) * nXSize + nXOff;

            // Compute the source image rectangle needed for this pixel.
            DstToSrc( dfXDst, dfYDst, dfXSrcStart, dfYSrcStart );
            DstToSrc( dfXDst+1.0, dfYDst+1.0, dfXSrcEnd, dfYSrcEnd );

            // Convert to integers, assuming that the center of the source
            // pixel must be in our rect to get included.
            if (dfXSrcEnd >= dfXSrcStart + 1)
            {
                iXSrcStart = (int) floor(dfXSrcStart+0.5);
                iXSrcEnd = (int) floor(dfXSrcEnd+0.5);
            }
            else
            {
                /* If the resampling factor is less than 100%, the distance */
                /* between the source pixel is < 1, so we stick to nearest */
                /* neighbour */
                iXSrcStart = (int) floor(dfXSrcStart);
                iXSrcEnd = iXSrcStart + 1;
            }
            if (dfYSrcEnd >= dfYSrcStart + 1)
            {
                iYSrcStart = (int) floor(dfYSrcStart+0.5);
                iYSrcEnd = (int) floor(dfYSrcEnd+0.5);
            }
            else
            {
                iYSrcStart = (int) floor(dfYSrcStart);
                iYSrcEnd = iYSrcStart + 1;
            }

            // Transform into the coordinate system of the source *buffer*
            iXSrcStart -= nReqXOff;
            iYSrcStart -= nReqYOff;
            iXSrcEnd -= nReqXOff;
            iYSrcEnd -= nReqYOff;

            double dfSum = 0.0;
            int    nPixelCount = 0;
            
            for( int iY = iYSrcStart; iY < iYSrcEnd; iY++ )
            {
                if( iY < 0 || iY >= nReqYSize )
                    continue;

                for( int iX = iXSrcStart; iX < iXSrcEnd; iX++ )
                {
                    if( iX < 0 || iX >= nReqXSize )
                        continue;

                    float fSampledValue = pafSrc[iX + iY * nReqXSize];

                    if( bNoDataSet
                        && ABS(fSampledValue-dfNoDataValue) < 0.0001 )
                        continue;

                    nPixelCount++;
                    dfSum += pafSrc[iX + iY * nReqXSize];
                }
            }

            if( nPixelCount == 0 )
                continue;

            // Compute output value.
            float dfOutputValue = (float) (dfSum / nPixelCount);

            // Put it in the output buffer.
            GByte *pDstLocation;

            pDstLocation = ((GByte *)pData) 
                + nPixelSpace * iBufPixel
                + nLineSpace * iBufLine;

            if( eBufType == GDT_Byte )
                *pDstLocation = (GByte) MIN(255,MAX(0,dfOutputValue));
            else
                GDALCopyWords( &dfOutputValue, GDT_Float32, 4, 
                               pDstLocation, eBufType, 8, 1 );
        }
    }

    VSIFree( pafSrc );

    return CE_None;
}

/************************************************************************/
/* ==================================================================== */
/*                          VRTComplexSource                            */
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                          VRTComplexSource()                          */
/************************************************************************/

VRTComplexSource::VRTComplexSource()

{
    bDoScaling = FALSE;
    dfScaleOff = 0.0;
    dfScaleRatio = 1.0;
    
    bNoDataSet = FALSE;
    dfNoDataValue = 0.0;

    padfLUTInputs = NULL;
    padfLUTOutputs = NULL;
    nLUTItemCount = 0;
    nColorTableComponent = 0;
}

VRTComplexSource::~VRTComplexSource()
{
    if (padfLUTInputs)
        VSIFree( padfLUTInputs );
    if (padfLUTOutputs)
        VSIFree( padfLUTOutputs );
}

/************************************************************************/
/*                           SerializeToXML()                           */
/************************************************************************/

CPLXMLNode *VRTComplexSource::SerializeToXML( const char *pszVRTPath )

{
    CPLXMLNode *psSrc = VRTSimpleSource::SerializeToXML( pszVRTPath );

    if( psSrc == NULL )
        return NULL;

    CPLFree( psSrc->pszValue );
    psSrc->pszValue = CPLStrdup( "ComplexSource" );

    if( bNoDataSet )
    {
        CPLSetXMLValue( psSrc, "NODATA", 
                        CPLSPrintf("%g", dfNoDataValue) );
    }
        
    if( bDoScaling )
    {
        CPLSetXMLValue( psSrc, "ScaleOffset", 
                        CPLSPrintf("%g", dfScaleOff) );
        CPLSetXMLValue( psSrc, "ScaleRatio", 
                        CPLSPrintf("%g", dfScaleRatio) );
    }

    if ( nLUTItemCount )
    {
        CPLString osLUT = CPLString().Printf("%g:%g", padfLUTInputs[0], padfLUTOutputs[0]);
        int i;
        for ( i = 1; i < nLUTItemCount; i++ )
            osLUT += CPLString().Printf(",%g:%g", padfLUTInputs[i], padfLUTOutputs[i]);
        CPLSetXMLValue( psSrc, "LUT", osLUT );
    }

    if ( nColorTableComponent )
    {
        CPLSetXMLValue( psSrc, "ColorTableComponent", 
                        CPLSPrintf("%d", nColorTableComponent) );
    }

    return psSrc;
}

/************************************************************************/
/*                              XMLInit()                               */
/************************************************************************/

CPLErr VRTComplexSource::XMLInit( CPLXMLNode *psSrc, const char *pszVRTPath )

{
    CPLErr eErr;

/* -------------------------------------------------------------------- */
/*      Do base initialization.                                         */
/* -------------------------------------------------------------------- */
    eErr = VRTSimpleSource::XMLInit( psSrc, pszVRTPath );
    if( eErr != CE_None )
        return eErr;

/* -------------------------------------------------------------------- */
/*      Complex parameters.                                             */
/* -------------------------------------------------------------------- */
    if( CPLGetXMLValue(psSrc, "ScaleOffset", NULL) != NULL 
        || CPLGetXMLValue(psSrc, "ScaleRatio", NULL) != NULL )
    {
        bDoScaling = TRUE;
        dfScaleOff = atof(CPLGetXMLValue(psSrc, "ScaleOffset", "0") );
        dfScaleRatio = atof(CPLGetXMLValue(psSrc, "ScaleRatio", "1") );
    }

    if( CPLGetXMLValue(psSrc, "NODATA", NULL) != NULL )
    {
        bNoDataSet = TRUE;
        dfNoDataValue = atof(CPLGetXMLValue(psSrc, "NODATA", "0"));
    }

    if( CPLGetXMLValue(psSrc, "LUT", NULL) != NULL )
    {
        int nIndex;
        char **papszValues = CSLTokenizeString2(CPLGetXMLValue(psSrc, "LUT", ""), ",:", CSLT_ALLOWEMPTYTOKENS);

        if (nLUTItemCount)
        {
            if (padfLUTInputs)
            {
                VSIFree( padfLUTInputs );
                padfLUTInputs = NULL;
            }
            if (padfLUTOutputs)
            {
                VSIFree( padfLUTOutputs );
                padfLUTOutputs = NULL;
            }
            nLUTItemCount = 0;
        }

        nLUTItemCount = CSLCount(papszValues) / 2;

        padfLUTInputs = (double *) VSIMalloc2(nLUTItemCount, sizeof(double));
        if ( !padfLUTInputs )
        {
            CSLDestroy(papszValues);
            nLUTItemCount = 0;
            return CE_Failure;
        }

        padfLUTOutputs = (double *) VSIMalloc2(nLUTItemCount, sizeof(double));
        if ( !padfLUTOutputs )
        {
            CSLDestroy(papszValues);
            VSIFree( padfLUTInputs );
            padfLUTInputs = NULL;
            nLUTItemCount = 0;
            return CE_Failure;
        }
        
        for ( nIndex = 0; nIndex < nLUTItemCount; nIndex++ )
        {
            padfLUTInputs[nIndex] = atof( papszValues[nIndex * 2] );
            padfLUTOutputs[nIndex] = atof( papszValues[nIndex * 2 + 1] );
        }
        
        CSLDestroy(papszValues);
    }
    
    if( CPLGetXMLValue(psSrc, "ColorTableComponent", NULL) != NULL )
    {
        nColorTableComponent = atoi(CPLGetXMLValue(psSrc, "ColorTableComponent", "0"));
    }

    return CE_None;
}

/************************************************************************/
/*                              LookupValue()                           */
/************************************************************************/

double
VRTComplexSource::LookupValue( double dfInput )
{
    int i;
    for ( i = 0; i < nLUTItemCount; i++ )
    {
        if (dfInput > padfLUTInputs[i])
            continue;
        if (i == 0)
            return padfLUTOutputs[0];
        
        if (padfLUTInputs[i-1] == padfLUTInputs[i])
            return padfLUTOutputs[i];

        return ((dfInput - padfLUTInputs[i-1]) * padfLUTOutputs[i] + 
            (padfLUTInputs[i] - dfInput) * padfLUTOutputs[i-1]) 
                                   / (padfLUTInputs[i] - padfLUTInputs[i-1]);
    }
    
    return padfLUTOutputs[nLUTItemCount - 1];
}

/************************************************************************/
/*                              RasterIO()                              */
/************************************************************************/

CPLErr 
VRTComplexSource::RasterIO( int nXOff, int nYOff, int nXSize, int nYSize,
                            void *pData, int nBufXSize, int nBufYSize, 
                            GDALDataType eBufType, 
                            int nPixelSpace, int nLineSpace )
    
{
    // The window we will actually request from the source raster band.
    int nReqXOff, nReqYOff, nReqXSize, nReqYSize;

    // The window we will actual set _within_ the pData buffer.
    int nOutXOff, nOutYOff, nOutXSize, nOutYSize;

    if( !GetSrcDstWindow( nXOff, nYOff, nXSize, nYSize, nBufXSize, nBufYSize, 
                          &nReqXOff, &nReqYOff, &nReqXSize, &nReqYSize,
                          &nOutXOff, &nOutYOff, &nOutXSize, &nOutYSize ) )
        return CE_None;

/* -------------------------------------------------------------------- */
/*      Read into a temporary buffer.                                   */
/* -------------------------------------------------------------------- */
    float *pafData;
    CPLErr eErr;

    pafData = (float *) CPLMalloc(nOutXSize*nOutYSize*sizeof(float));
    eErr = poRasterBand->RasterIO( GF_Read, 
                                   nReqXOff, nReqYOff, nReqXSize, nReqYSize,
                                   pafData, nOutXSize, nOutYSize, GDT_Float32,
                                   sizeof(float), sizeof(float) * nOutXSize );
    if( eErr != CE_None )
    {
        CPLFree( pafData );
        return eErr;
    }

    GDALColorTable* poColorTable = NULL;
    if (nColorTableComponent != 0)
    {
        poColorTable = poRasterBand->GetColorTable();
        if (poColorTable == NULL)
        {
            CPLError(CE_Failure, CPLE_AppDefined,
                     "Source band has no color table.");
            return CE_Failure;
        }
    }

/* -------------------------------------------------------------------- */
/*      Selectively copy into output buffer with nodata masking,        */
/*      and/or scaling.                                                 */
/* -------------------------------------------------------------------- */
    int iX, iY;

    for( iY = 0; iY < nOutYSize; iY++ )
    {
        for( iX = 0; iX < nOutXSize; iX++ )
        {
            float fResult;

            fResult = pafData[iX + iY * nOutXSize];
            if( bNoDataSet && fResult == dfNoDataValue )
                continue;

            if (nColorTableComponent)
            {
                const GDALColorEntry* poEntry = poColorTable->GetColorEntry((int)fResult);
                if (poEntry)
                {
                    if (nColorTableComponent == 1)
                        fResult = poEntry->c1;
                    else if (nColorTableComponent == 2)
                        fResult = poEntry->c2;
                    else if (nColorTableComponent == 3)
                        fResult = poEntry->c3;
                    else if (nColorTableComponent == 4)
                        fResult = poEntry->c4;
                }
                else
                {
                    CPLError(CE_Failure, CPLE_AppDefined,
                             "No entry %d.", (int)fResult);
                    return CE_Failure;
                }
            }

            if( bDoScaling )
                fResult = (float) (fResult * dfScaleRatio + dfScaleOff);

            if (nLUTItemCount)
                fResult = (float) LookupValue( fResult );

            GByte *pDstLocation;

            pDstLocation = ((GByte *)pData) 
                + nPixelSpace * (iX + nOutXOff)
                + nLineSpace * (iY + nOutYOff);

            if( eBufType == GDT_Byte )
                *pDstLocation = (GByte) MIN(255,MAX(0,fResult));
            else
                GDALCopyWords( &fResult, GDT_Float32, 4, 
                               pDstLocation, eBufType, 8, 1 );
                
        }
    }

    CPLFree( pafData );

    return CE_None;
}

/************************************************************************/
/* ==================================================================== */
/*                          VRTFuncSource                               */
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                           VRTFuncSource()                            */
/************************************************************************/

VRTFuncSource::VRTFuncSource()

{
    pfnReadFunc = NULL;
    pCBData = NULL;
    fNoDataValue = (float) VRT_NODATA_UNSET;
    eType = GDT_Byte;
}

/************************************************************************/
/*                           ~VRTFuncSource()                           */
/************************************************************************/

VRTFuncSource::~VRTFuncSource()

{
}

/************************************************************************/
/*                           SerializeToXML()                           */
/************************************************************************/

CPLXMLNode *VRTFuncSource::SerializeToXML( const char * pszVRTPath )

{
    return NULL;
}

/************************************************************************/
/*                              RasterIO()                              */
/************************************************************************/

CPLErr 
VRTFuncSource::RasterIO( int nXOff, int nYOff, int nXSize, int nYSize,
                         void *pData, int nBufXSize, int nBufYSize, 
                         GDALDataType eBufType, 
                         int nPixelSpace, int nLineSpace )

{
    if( nPixelSpace*8 == GDALGetDataTypeSize( eBufType )
        && nLineSpace == nPixelSpace * nXSize 
        && nBufXSize == nXSize && nBufYSize == nYSize 
        && eBufType == eType )
    {
        return pfnReadFunc( pCBData,
                            nXOff, nYOff, nXSize, nYSize, 
                            pData );
    }
    else
    {
        printf( "%d,%d  %d,%d, %d,%d %d,%d %d,%d\n", 
                nPixelSpace*8, GDALGetDataTypeSize(eBufType),
                nLineSpace, nPixelSpace * nXSize, 
                nBufXSize, nXSize, 
                nBufYSize, nYSize, 
                (int) eBufType, (int) eType );
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "VRTFuncSource::RasterIO() - Irregular request." );
        return CE_Failure;
    }
}

/************************************************************************/
/*                        VRTParseCoreSources()                         */
/************************************************************************/

VRTSource *VRTParseCoreSources( CPLXMLNode *psChild, const char *pszVRTPath )

{
    VRTSource * poSource;

    if( EQUAL(psChild->pszValue,"AveragedSource") 
        || (EQUAL(psChild->pszValue,"SimpleSource")
            && EQUALN(CPLGetXMLValue(psChild, "Resampling", "Nearest"),
                      "Aver",4)) )
    {
        poSource = new VRTAveragedSource();
    }
    else if( EQUAL(psChild->pszValue,"SimpleSource") )
    {
        poSource = new VRTSimpleSource();
    }
    else if( EQUAL(psChild->pszValue,"ComplexSource") )
    {
        poSource = new VRTComplexSource();
    }
    else
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "VRTParseCoreSources() - Unknown source : %s", psChild->pszValue );
        return NULL;
    }

    if ( poSource->XMLInit( psChild, pszVRTPath ) == CE_None )
        return poSource;

    delete poSource;
    return NULL;
}

