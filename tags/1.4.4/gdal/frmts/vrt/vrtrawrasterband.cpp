/******************************************************************************
 * $Id$
 *
 * Project:  Virtual GDAL Datasets
 * Purpose:  Implementation of VRTRawRasterBand
 * Author:   Frank Warmerdam <warmerdam@pobox.com>
 *
 ******************************************************************************
 * Copyright (c) 2004, Frank Warmerdam <warmerdam@pobox.com>
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
#include "cpl_minixml.h"
#include "cpl_string.h"
#include "rawdataset.h"

CPL_CVSID("$Id$");

/************************************************************************/
/* ==================================================================== */
/*                          VRTRawRasterBand                            */
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                          VRTRawRasterBand()                          */
/************************************************************************/

VRTRawRasterBand::VRTRawRasterBand( GDALDataset *poDS, int nBand,
                                    GDALDataType eType )

{
    Initialize( poDS->GetRasterXSize(), poDS->GetRasterYSize() );

    this->poDS = poDS;
    this->nBand = nBand;

    if( eType != GDT_Unknown )
        this->eDataType = eType;

    poRawRaster = NULL;
    pszSourceFilename = NULL;
}

/************************************************************************/
/*                         ~VRTRawRasterBand()                          */
/************************************************************************/

VRTRawRasterBand::~VRTRawRasterBand()

{
    FlushCache();
    ClearRawLink();
}

/************************************************************************/
/*                             IRasterIO()                              */
/************************************************************************/

CPLErr VRTRawRasterBand::IRasterIO( GDALRWFlag eRWFlag,
                                 int nXOff, int nYOff, int nXSize, int nYSize,
                                 void * pData, int nBufXSize, int nBufYSize,
                                 GDALDataType eBufType,
                                 int nPixelSpace, int nLineSpace )

{
    if( poRawRaster == NULL )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "No raw raster band configured on VRTRawRasterBand." );
        return CE_Failure;
    }

/* -------------------------------------------------------------------- */
/*      Do we have overviews that would be appropriate to satisfy       */
/*      this request?                                                   */
/* -------------------------------------------------------------------- */
    if( (nBufXSize < nXSize || nBufYSize < nYSize)
        && GetOverviewCount() > 0 )
    {
        if( OverviewRasterIO( eRWFlag, nXOff, nYOff, nXSize, nYSize, 
                              pData, nBufXSize, nBufYSize, 
                              eBufType, nPixelSpace, nLineSpace ) == CE_None )
            return CE_None;
    }

    return poRawRaster->RasterIO( eRWFlag, nXOff, nYOff, nXSize, nYSize, 
                                  pData, nBufXSize, nBufYSize, 
                                  eBufType, nPixelSpace, nLineSpace );
}

/************************************************************************/
/*                             IReadBlock()                             */
/************************************************************************/

CPLErr VRTRawRasterBand::IReadBlock( int nBlockXOff, int nBlockYOff,
                                   void * pImage )

{
    if( poRawRaster == NULL )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "No raw raster band configured on VRTRawRasterBand." );
        return CE_Failure;
    }

    return poRawRaster->ReadBlock( nBlockXOff, nBlockYOff, pImage );
}

/************************************************************************/
/*                            IWriteBlock()                             */
/************************************************************************/

CPLErr VRTRawRasterBand::IWriteBlock( int nBlockXOff, int nBlockYOff,
                                      void * pImage )

{
    if( poRawRaster == NULL )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "No raw raster band configured on VRTRawRasterBand." );
        return CE_Failure;
    }

    return poRawRaster->WriteBlock( nBlockXOff, nBlockYOff, pImage );
}

/************************************************************************/
/*                             SetRawLink()                             */
/************************************************************************/

CPLErr VRTRawRasterBand::SetRawLink( const char *pszFilename, 
                                     const char *pszVRTPath,
                                     int bRelativeToVRT, 
                                     vsi_l_offset nImageOffset, 
                                     int nPixelOffset, int nLineOffset, 
                                     const char *pszByteOrder )

{
    ClearRawLink();

    ((VRTDataset *)poDS)->SetNeedsFlush();

/* -------------------------------------------------------------------- */
/*      Prepare filename.                                               */
/* -------------------------------------------------------------------- */
    char *pszExpandedFilename = NULL;

    if( pszFilename == NULL )
    {
        CPLError( CE_Warning, CPLE_AppDefined, 
                  "Missing <SourceFilename> element in VRTRasterBand." );
        return CE_Failure;
    }
    
    if( pszVRTPath != NULL && bRelativeToVRT )
    {
        pszExpandedFilename = CPLStrdup(
            CPLProjectRelativeFilename( pszVRTPath, pszFilename ) );
    }
    else
        pszExpandedFilename = CPLStrdup( pszFilename );

/* -------------------------------------------------------------------- */
/*      Try and open the file.  We always use the large file API.       */
/* -------------------------------------------------------------------- */
    FILE *fp = CPLOpenShared( pszExpandedFilename, "rb+", TRUE );

    if( fp == NULL )
        fp = CPLOpenShared( pszExpandedFilename, "rb", TRUE );

    if( fp == NULL )
    {
        CPLError( CE_Failure, CPLE_OpenFailed, 
                  "Unable to open %s.\n%s", 
                  pszExpandedFilename,
                  VSIStrerror( errno ) );
        
        CPLFree( pszExpandedFilename );
        return CE_Failure;
    }

    CPLFree( pszExpandedFilename );

    pszSourceFilename = CPLStrdup(pszFilename);
    this->bRelativeToVRT = bRelativeToVRT;

/* -------------------------------------------------------------------- */
/*      Work out if we are in native mode or not.                       */
/* -------------------------------------------------------------------- */
    int bNative = TRUE;

    if( pszByteOrder != NULL )
    {
        if( EQUAL(pszByteOrder,"LSB") )
            bNative = CPL_IS_LSB;
        else if( EQUAL(pszByteOrder,"MSB") )
            bNative = !CPL_IS_LSB;
        else
        {
            CPLError( CE_Failure, CPLE_AppDefined, 
                      "Illegal ByteOrder value '%s', should be LSB or MSB.", 
                      pszByteOrder );
            return CE_Failure;
        }
    }

/* -------------------------------------------------------------------- */
/*      Create a corresponding RawRasterBand.                           */
/* -------------------------------------------------------------------- */
    poRawRaster = new RawRasterBand( fp, nImageOffset, nPixelOffset, 
                                     nLineOffset, GetRasterDataType(), 
                                     bNative, GetXSize(), GetYSize(), TRUE );

/* -------------------------------------------------------------------- */
/*      Reset block size to match the raw raster.                       */
/* -------------------------------------------------------------------- */
    poRawRaster->GetBlockSize( &nBlockXSize, &nBlockYSize );

    return CE_None;
}


/************************************************************************/
/*                            ClearRawLink()                            */
/************************************************************************/

void VRTRawRasterBand::ClearRawLink()

{
    if( poRawRaster != NULL )
    {
        if( poRawRaster->GetFP() != NULL )
        {
            CPLCloseShared( poRawRaster->GetFP() );
        }
        delete poRawRaster;
        poRawRaster = NULL;
    }
    CPLFree( pszSourceFilename );
    pszSourceFilename = NULL;
}

/************************************************************************/
/*                              XMLInit()                               */
/************************************************************************/

CPLErr VRTRawRasterBand::XMLInit( CPLXMLNode * psTree, 
                                      const char *pszVRTPath )

{
    CPLErr eErr;

    eErr = VRTRasterBand::XMLInit( psTree, pszVRTPath );
    if( eErr != CE_None )
        return eErr;
    

/* -------------------------------------------------------------------- */
/*      Validate a bit.                                                 */
/* -------------------------------------------------------------------- */
    if( psTree == NULL || psTree->eType != CXT_Element
        || !EQUAL(psTree->pszValue,"VRTRasterBand") 
        || !EQUAL(CPLGetXMLValue(psTree,"subClass",""),"VRTRawRasterBand") )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Invalid node passed to VRTRawRasterBand::XMLInit()." );
        return CE_Failure;
    }

/* -------------------------------------------------------------------- */
/*      Prepare filename.                                               */
/* -------------------------------------------------------------------- */
    int  bRelativeToVRT;

    const char *pszFilename = 
        CPLGetXMLValue(psTree, "SourceFilename", NULL);

    if( pszFilename == NULL )
    {
        CPLError( CE_Warning, CPLE_AppDefined, 
                  "Missing <SourceFilename> element in VRTRasterBand." );
        return CE_Failure;
    }

    bRelativeToVRT = atoi(CPLGetXMLValue(psTree,"SourceFilename.relativeToVRT",
                                         "1"));
    
/* -------------------------------------------------------------------- */
/*      Collect layout information.                                     */
/* -------------------------------------------------------------------- */
    vsi_l_offset nImageOffset;
    int nPixelOffset, nLineOffset;
    int nWordDataSize = GDALGetDataTypeSize( GetRasterDataType() ) / 8;

    nImageOffset = atoi(CPLGetXMLValue( psTree, "ImageOffset", "0") );

    if( CPLGetXMLValue( psTree, "PixelOffset", NULL ) == NULL )
        nPixelOffset = nWordDataSize;
    else
        nPixelOffset = atoi(CPLGetXMLValue( psTree, "PixelOffset", "0") );
    
    if( CPLGetXMLValue( psTree, "LineOffset", NULL ) == NULL )
        nLineOffset = nWordDataSize * GetXSize();
    else
        nLineOffset = atoi(CPLGetXMLValue( psTree, "LineOffset", "0") );

    const char *pszByteOrder = CPLGetXMLValue( psTree, "ByteOrder", NULL );

/* -------------------------------------------------------------------- */
/*      Open the file, and setup the raw layer access to the data.      */
/* -------------------------------------------------------------------- */
    return SetRawLink( pszFilename, pszVRTPath, bRelativeToVRT, 
                       nImageOffset, nPixelOffset, nLineOffset, 
                       pszByteOrder );
}

/************************************************************************/
/*                           SerializeToXML()                           */
/************************************************************************/

CPLXMLNode *VRTRawRasterBand::SerializeToXML( const char *pszVRTPath )

{
    CPLXMLNode *psTree;

    psTree = VRTRasterBand::SerializeToXML( pszVRTPath );

/* -------------------------------------------------------------------- */
/*      Set subclass.                                                   */
/* -------------------------------------------------------------------- */
    CPLCreateXMLNode( 
        CPLCreateXMLNode( psTree, CXT_Attribute, "subClass" ), 
        CXT_Text, "VRTRawRasterBand" );

/* -------------------------------------------------------------------- */
/*      Setup the filename with relative flag.                          */
/* -------------------------------------------------------------------- */
    CPLXMLNode *psNode;

    psNode = 
        CPLCreateXMLElementAndValue( psTree, "SourceFilename", 
                                     pszSourceFilename );
    
    CPLCreateXMLNode( 
        CPLCreateXMLNode( psNode, CXT_Attribute, "relativeToVRT" ), 
        CXT_Text, bRelativeToVRT ? "1" : "0"  );

/* -------------------------------------------------------------------- */
/*      We can't set the layout if there is no open rawband.            */
/* -------------------------------------------------------------------- */
    if( poRawRaster == NULL )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "VRTRawRasterBand::SerializeToXML() fails because poRawRaster is NULL." );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Set other layout information.                                   */
/* -------------------------------------------------------------------- */
    CPLCreateXMLElementAndValue( 
        psTree, "ImageOffset", 
        CPLSPrintf("%d",(int) poRawRaster->GetImgOffset()) );
    
    CPLCreateXMLElementAndValue( 
        psTree, "PixelOffset", 
        CPLSPrintf("%d",(int) poRawRaster->GetPixelOffset()) );
    
    CPLCreateXMLElementAndValue( 
        psTree, "LineOffset", 
        CPLSPrintf("%d",(int) poRawRaster->GetLineOffset()) );

    if( (poRawRaster->GetNativeOrder() && CPL_IS_LSB)
        || (!poRawRaster->GetNativeOrder() && !CPL_IS_LSB) )
        CPLCreateXMLElementAndValue( psTree, "ByteOrder", "LSB" );
    else
        CPLCreateXMLElementAndValue( psTree, "ByteOrder", "MSB" );
    
    return psTree;
}
