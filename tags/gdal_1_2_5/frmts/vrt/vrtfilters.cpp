/******************************************************************************
 * $Id$
 *
 * Project:  Virtual GDAL Datasets
 * Purpose:  Implementation of some filter types.
 * Author:   Frank Warmerdam <warmerdam@pobox.com>
 *
 ******************************************************************************
 * Copyright (c) 2003, Frank Warmerdam <warmerdam@pobox.com>
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
 ******************************************************************************
 *
 * $Log$
 * Revision 1.5  2004/08/11 18:46:45  warmerda
 * pass pszVRTPath through serialize methods
 *
 * Revision 1.4  2004/03/16 18:34:35  warmerda
 * added support for relativeToVRT attribute on SourceFilename
 *
 * Revision 1.3  2003/09/11 19:53:32  warmerda
 * avoid type casting warnings
 *
 * Revision 1.2  2003/08/07 17:11:21  warmerda
 * added normalized flag for kernel based filters
 *
 * Revision 1.1  2003/07/17 20:27:18  warmerda
 * New
 *
 */

#include "vrtdataset.h"
#include "cpl_minixml.h"
#include "cpl_string.h"

CPL_CVSID("$Id$");

/************************************************************************/
/* ==================================================================== */
/*                          VRTFilteredSource                           */
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                         VRTFilteredSource()                          */
/************************************************************************/

VRTFilteredSource::VRTFilteredSource()

{
    nExtraEdgePixels = 0;
    nSupportedTypesCount = 1;
    aeSupportedTypes[0] = GDT_Float32;
}

/************************************************************************/
/*                         ~VRTFilteredSource()                         */
/************************************************************************/

VRTFilteredSource::~VRTFilteredSource()

{
}

/************************************************************************/
/*                         SetExtraEdgePixels()                         */
/************************************************************************/

void VRTFilteredSource::SetExtraEdgePixels( int nEdgePixels )

{
    nExtraEdgePixels = nEdgePixels;
}

/************************************************************************/
/*                   SetFilteringDataTypesSupported()                   */
/************************************************************************/

void VRTFilteredSource::SetFilteringDataTypesSupported( int nTypeCount, 
                                                        GDALDataType *paeTypes)

{
    if( nTypeCount > 
        (int) sizeof(sizeof(aeSupportedTypes)/sizeof(GDALDataType)) )
    {
        CPLAssert( FALSE );
        nTypeCount = (int) 
            sizeof(sizeof(aeSupportedTypes)/sizeof(GDALDataType));
    }

    nSupportedTypesCount = nTypeCount;
    memcpy( aeSupportedTypes, paeTypes, sizeof(GDALDataType) * nTypeCount );
}

/************************************************************************/
/*                          IsTypeSupported()                           */
/************************************************************************/

int VRTFilteredSource::IsTypeSupported( GDALDataType eTestType )

{
    int i;

    for( i = 0; i < nSupportedTypesCount; i++ )
    {
        if( eTestType == aeSupportedTypes[i] )
            return TRUE;
    }

    return FALSE;
}

/************************************************************************/
/*                              RasterIO()                              */
/************************************************************************/

CPLErr
VRTFilteredSource::RasterIO( int nXOff, int nYOff, int nXSize, int nYSize, 
                             void *pData, int nBufXSize, int nBufYSize, 
                             GDALDataType eBufType, 
                             int nPixelSpace, int nLineSpace )

{
/* -------------------------------------------------------------------- */
/*      For now we don't support filtered access to non-full            */
/*      resolution requests. Just collect the data directly without     */
/*      any operator.                                                   */
/* -------------------------------------------------------------------- */
    if( nBufXSize != nXSize || nBufYSize != nYSize )
    {
        return VRTComplexSource::RasterIO( nXOff, nYOff, nXSize, nYSize, 
                                           pData, nBufXSize, nBufYSize, 
                                           eBufType, nPixelSpace, nLineSpace );
    }

/* -------------------------------------------------------------------- */
/*      Determine the data type we want to request.  We try to match    */
/*      the source or destination request, and if both those fail we    */
/*      fallback to the first supported type at least as expressive     */
/*      as the request.                                                 */
/* -------------------------------------------------------------------- */
    GDALDataType eOperDataType = GDT_Unknown;
    int i;
    
    if( IsTypeSupported( eBufType ) )
        eOperDataType = eBufType;

    if( eOperDataType == GDT_Unknown 
        && IsTypeSupported( poRasterBand->GetRasterDataType() ) )
        eOperDataType = poRasterBand->GetRasterDataType();

    if( eOperDataType == GDT_Unknown )
    {
        for( i = 0; i < nSupportedTypesCount; i++ )
        {
            if( GDALDataTypeUnion( aeSupportedTypes[i], eBufType ) 
                == aeSupportedTypes[i] )
            {
                eOperDataType = aeSupportedTypes[i];
            }
        }
    }

    if( eOperDataType == GDT_Unknown )
    {
        eOperDataType = aeSupportedTypes[0];

        for( i = 1; i < nSupportedTypesCount; i++ )
        {
            if( GDALGetDataTypeSize( aeSupportedTypes[i] ) 
                > GDALGetDataTypeSize( eOperDataType ) )
            {
                eOperDataType = aeSupportedTypes[i];
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      Allocate the buffer of data into which our imagery will be      */
/*      read, with the extra edge pixels as well. This will be the      */
/*      source data fed into the filter.                                */
/* -------------------------------------------------------------------- */
    int nPixelOffset, nLineOffset;
    int nExtraXSize = nBufXSize + 2 * nExtraEdgePixels;
    int nExtraYSize = nBufYSize + 2 * nExtraEdgePixels;
    GByte *pabyWorkData;

    pabyWorkData = (GByte *) 
        VSIMalloc( nExtraXSize * nExtraYSize 
                   * (GDALGetDataTypeSize(eOperDataType) / 8) );
    
    if( pabyWorkData == NULL )
    {
        CPLError( CE_Failure, CPLE_OutOfMemory, 
                  "Work buffer allocation failed." );
        return CE_Failure;
    }

    nPixelOffset = GDALGetDataTypeSize( eOperDataType ) / 8;
    nLineOffset = nPixelOffset * nExtraXSize;

/* -------------------------------------------------------------------- */
/*      Allocate the output buffer if the passed in output buffer is    */
/*      not of the same type as our working format, or if the passed    */
/*      in buffer has an unusual organization.                          */
/* -------------------------------------------------------------------- */
    GByte *pabyOutData;

    if( nPixelSpace != nPixelOffset || nLineSpace != nLineOffset
        || eOperDataType != eBufType )
    {
        pabyOutData = (GByte *) 
            VSIMalloc( nBufXSize * nBufYSize * nPixelOffset );

        if( pabyOutData == NULL )
        {
            CPLError( CE_Failure, CPLE_OutOfMemory, 
                      "Work buffer allocation failed." );
            return CE_Failure;
        }
    }
    else
        pabyOutData = (GByte *) pData;

/* -------------------------------------------------------------------- */
/*      Figure out the extended window that we want to load.  Note      */
/*      that we keep track of the file window as well as the amount     */
/*      we will need to edge fill past the edge of the source dataset.  */
/* -------------------------------------------------------------------- */
    int nTopFill=0, nLeftFill=0, nRightFill=0, nBottomFill=0;
    int nFileXOff, nFileYOff, nFileXSize, nFileYSize;

    nFileXOff = nXOff - nExtraEdgePixels;
    nFileYOff = nYOff - nExtraEdgePixels;
    nFileXSize = nExtraXSize;
    nFileYSize = nExtraYSize;

    if( nFileXOff < 0 )
    {
        nLeftFill = -nFileXOff;
        nFileXOff = 0;
        nFileXSize -= nLeftFill;
    }

    if( nFileYOff < 0 )
    {
        nTopFill = -nFileYOff;
        nFileYOff = 0;
        nFileYSize -= nTopFill;
    }

    if( nFileXOff + nFileXSize > poRasterBand->GetXSize() )
    {
        nRightFill = nFileXOff + nFileXSize - poRasterBand->GetXSize();
        nFileXSize -= nRightFill;
    }

    if( nFileYOff + nFileYSize > poRasterBand->GetYSize() )
    {
        nBottomFill = nFileYOff + nFileYSize - poRasterBand->GetYSize();
        nFileYSize -= nBottomFill;
    }

/* -------------------------------------------------------------------- */
/*      Load the data.                                                  */
/* -------------------------------------------------------------------- */
    CPLErr eErr;

    eErr = 
      VRTComplexSource::RasterIO( nFileXOff, nFileYOff, nFileXSize, nFileYSize,
                                  pabyWorkData 
                                  + nLineOffset * nTopFill
                                  + nPixelOffset * nLeftFill,
                                  nFileXSize, nFileYSize, eOperDataType, 
                                  nPixelOffset, nLineOffset );

    if( eErr != CE_None )
    {
        if( pabyWorkData != pData )
            VSIFree( pabyWorkData );

        return eErr;
    }

/* -------------------------------------------------------------------- */
/*      Fill in missing areas.  Note that we replicate the edge         */
/*      valid values out.  We don't using "mirroring" which might be    */
/*      more suitable for some times of filters.  We also don't mark    */
/*      these pixels as "nodata" though perhaps we should.              */
/* -------------------------------------------------------------------- */
    if( nLeftFill != 0 || nRightFill != 0 )
    {
        for( i = nTopFill; i < nExtraYSize - nBottomFill; i++ )
        {
            if( nLeftFill != 0 )
                GDALCopyWords( pabyWorkData + nPixelOffset * nLeftFill
                               + i * nLineOffset, eOperDataType, 0, 
                               pabyWorkData + i * nLineOffset, eOperDataType, 
                               nPixelOffset, nLeftFill );

            if( nRightFill != 0 )
                GDALCopyWords( pabyWorkData + i * nLineOffset
                               + nPixelOffset * (nExtraXSize - nRightFill - 1),
                               eOperDataType, 0, 
                               pabyWorkData + i * nLineOffset
                               + nPixelOffset * (nExtraXSize - nRightFill),
                               eOperDataType, nPixelOffset, nRightFill );
        }
    }

    for( i = 0; i < nTopFill; i++ )
    {
        memcpy( pabyWorkData + i * nLineOffset, 
                pabyWorkData + nTopFill * nLineOffset, 
                nLineOffset );
    }

    for( i = nExtraYSize - nBottomFill; i < nExtraYSize; i++ )
    {
        memcpy( pabyWorkData + i * nLineOffset, 
                pabyWorkData + (nExtraYSize - nBottomFill - 1) * nLineOffset, 
                nLineOffset );
    }
    
/* -------------------------------------------------------------------- */
/*      Filter the data.                                                */
/* -------------------------------------------------------------------- */
    eErr = FilterData( nBufXSize, nBufYSize, eOperDataType, 
                       pabyWorkData, pabyOutData );

    VSIFree( pabyWorkData );
    if( eErr != CE_None )
    {
        if( pabyOutData != pData )
            VSIFree( pabyOutData );

        return eErr;
    }
    
/* -------------------------------------------------------------------- */
/*      Copy from work buffer to target buffer.                         */
/* -------------------------------------------------------------------- */
    if( pabyOutData != pData )
    {
        for( i = 0; i < nBufYSize; i++ )
        {
            GDALCopyWords( pabyOutData + i * (nPixelOffset * nBufXSize),
                           eOperDataType, nPixelOffset,
                           ((GByte *) pData) + i * nLineSpace, 
                           eBufType, nPixelSpace, nBufXSize );
        }

        VSIFree( pabyOutData );
    }

    return CE_None;
}

/************************************************************************/
/* ==================================================================== */
/*                       VRTKernelFilteredSource                        */
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                      VRTKernelFilteredSource()                       */
/************************************************************************/

VRTKernelFilteredSource::VRTKernelFilteredSource()

{
    GDALDataType aeSupTypes[] = { GDT_Float32 };
    padfKernelCoefs = NULL;
    nKernelSize = 0;
    bNormalized = FALSE;

    SetFilteringDataTypesSupported( 1, aeSupTypes );
}

/************************************************************************/
/*                      ~VRTKernelFilteredSource()                      */
/************************************************************************/

VRTKernelFilteredSource::~VRTKernelFilteredSource() 

{
    CPLFree( padfKernelCoefs );
}

/************************************************************************/
/*                           SetNormalized()                            */
/************************************************************************/

void VRTKernelFilteredSource::SetNormalized( int bNormalizedIn )

{
    bNormalized = bNormalizedIn;
}

/************************************************************************/
/*                             SetKernel()                              */
/************************************************************************/

CPLErr VRTKernelFilteredSource::SetKernel( int nNewKernelSize, 
                                           double *padfNewCoefs )

{
    if( nNewKernelSize < 1 || (nNewKernelSize % 2) != 1 )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Illegal filtering kernel size %d, must be odd positive number.", 
                  nNewKernelSize );
        return CE_Failure;
    }

    CPLFree( padfKernelCoefs );
    nKernelSize = nNewKernelSize;

    padfKernelCoefs = (double *) 
        CPLMalloc(sizeof(double) * nKernelSize * nKernelSize );
    memcpy( padfKernelCoefs, padfNewCoefs, 
            sizeof(double) * nKernelSize * nKernelSize );

    SetExtraEdgePixels( (nNewKernelSize - 1) / 2 );

    return CE_None;
}

/************************************************************************/
/*                             FilterData()                             */
/************************************************************************/

CPLErr VRTKernelFilteredSource::
FilterData( int nXSize, int nYSize, GDALDataType eType, 
            GByte *pabySrcData, GByte *pabyDstData )

{
/* -------------------------------------------------------------------- */
/*      Validate data type.                                             */
/* -------------------------------------------------------------------- */
    if( eType != GDT_Float32 )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Unsupported data type (%s) in VRTKernelFilteredSource::FilterData()", 
                  GDALGetDataTypeName( eType ) );
        return CE_Failure; 
    }

    CPLAssert( nExtraEdgePixels*2 + 1 == nKernelSize );

/* -------------------------------------------------------------------- */
/*      Float32 case.                                                   */
/* -------------------------------------------------------------------- */
    if( eType == GDT_Float32 )
    {
        int iX, iY; 

        for( iY = 0; iY < nYSize; iY++ )
        {
            for( iX = 0; iX < nXSize; iX++ )
            {
                int    iYY, iKern = 0;
                double dfSum = 0.0, dfKernSum = 0.0;
                float  fResult;

                for( iYY = 0; iYY < nKernelSize; iYY++ )
                {
                    int i;
                    float *pafData = ((float *)pabySrcData) 
                        + (iY+iYY) * (nXSize+2*nExtraEdgePixels) + iX;

                    for( i = 0; i < nKernelSize; i++, pafData++, iKern++ )
                    {
                        dfSum += *pafData * padfKernelCoefs[iKern];
                        dfKernSum += padfKernelCoefs[iKern];
                    }
                }

                if( bNormalized )
                {
                    if( dfKernSum != 0.0 )
                        fResult = (float) (dfSum / dfKernSum);
                    else
                        fResult = 0.0;
                }
                else
                    fResult = (float) dfSum;

                ((float *) pabyDstData)[iX + iY * nXSize] = fResult;
            }
        }
    }

    return CE_None;
}

/************************************************************************/
/*                              XMLInit()                               */
/************************************************************************/

CPLErr VRTKernelFilteredSource::XMLInit( CPLXMLNode *psTree, 
                                         const char *pszVRTPath )

{
    CPLErr eErr = VRTFilteredSource::XMLInit( psTree, pszVRTPath );
    int    nNewKernelSize, i, nCoefs;
    double *padfNewCoefs;

    if( eErr != CE_None )
        return eErr;

    nNewKernelSize = atoi(CPLGetXMLValue(psTree,"Kernel.Size","0"));

    if( nNewKernelSize == 0 )
        return CE_None;

    char **papszCoefItems = 
        CSLTokenizeString( CPLGetXMLValue(psTree,"Kernel.Coefs","") );

    nCoefs = CSLCount(papszCoefItems);

    if( nCoefs != nNewKernelSize * nNewKernelSize )
    {
        CSLDestroy( papszCoefItems );
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Got wrong number of filter kernel coefficients (%s)", 
                  CPLGetXMLValue(psTree,"Kernel.Coefs","") );
        return CE_Failure;
    }

    padfNewCoefs = (double *) CPLMalloc(sizeof(double) * nCoefs);

    for( i = 0; i < nCoefs; i++ )
        padfNewCoefs[i] = atof(papszCoefItems[i]);

    eErr = SetKernel( nNewKernelSize, padfNewCoefs );

    CPLFree( padfNewCoefs );
    CSLDestroy( papszCoefItems );

    SetNormalized( atoi(CPLGetXMLValue(psTree,"Kernel.normalized","0")) );

    return eErr;
}

/************************************************************************/
/*                           SerializeToXML()                           */
/************************************************************************/

CPLXMLNode *VRTKernelFilteredSource::SerializeToXML( const char *pszVRTPath )

{
    CPLXMLNode *psSrc = VRTFilteredSource::SerializeToXML( pszVRTPath );
    CPLXMLNode *psKernel;
    char *pszKernelCoefs;
    int iCoef, nCoefCount = nKernelSize * nKernelSize;

    if( psSrc == NULL )
        return NULL;

    CPLFree( psSrc->pszValue );
    psSrc->pszValue = CPLStrdup("KernelFilteredSource" );

    psKernel = CPLCreateXMLNode( psSrc, CXT_Element, "Kernel" );

    if( bNormalized )
        CPLCreateXMLNode( 
            CPLCreateXMLNode( psKernel, CXT_Attribute, "normalized" ), 
            CXT_Text, "1" );
    else
        CPLCreateXMLNode( 
            CPLCreateXMLNode( psKernel, CXT_Attribute, "normalized" ), 
            CXT_Text, "0" );

    pszKernelCoefs = (char *) CPLMalloc(nCoefCount * 32);

    strcpy( pszKernelCoefs, "" );
    for( iCoef = 0; iCoef < nCoefCount; iCoef++ )
        sprintf( pszKernelCoefs + strlen(pszKernelCoefs), 
                 "%.8g ", padfKernelCoefs[iCoef] );
    
    CPLSetXMLValue( psKernel, "Size", CPLSPrintf( "%d", nKernelSize ) );
    CPLSetXMLValue( psKernel, "Coefs", pszKernelCoefs );

    CPLFree( pszKernelCoefs );

    return psSrc;
}

/************************************************************************/
/*                       VRTParseFilterSources()                        */
/************************************************************************/

VRTSource *VRTParseFilterSources( CPLXMLNode *psChild, const char *pszVRTPath )

{
    VRTSource *poSrc;

    if( EQUAL(psChild->pszValue,"KernelFilteredSource") )
    {
        poSrc = new VRTKernelFilteredSource();
        if( poSrc->XMLInit( psChild, pszVRTPath ) == CE_None )
            return poSrc;
        else
            delete poSrc;
    }

    return NULL;
}

