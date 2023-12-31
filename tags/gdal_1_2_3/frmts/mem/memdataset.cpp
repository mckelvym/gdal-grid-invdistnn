/******************************************************************************
 * $Id$
 *
 * Project:  Memory Array Translator
 * Purpose:  Complete implementation.
 * Author:   Frank Warmerdam, warmerda@home.com
 *
 ******************************************************************************
 * Copyright (c) 2000, Frank Warmerdam
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
 * Revision 1.18  2004/08/30 18:50:27  warmerda
 * Fixed so that SetProjection() succeeds.
 *
 * Revision 1.17  2004/04/15 18:54:10  warmerda
 * added UnitType, Offset, Scale and CategoryNames support
 *
 * Revision 1.16  2003/04/16 14:37:54  warmerda
 * Comment out debug messages ... too noisy.
 *
 * Revision 1.15  2003/02/03 17:57:30  warmerda
 * Fix for last fix.
 *
 * Revision 1.14  2003/02/03 16:28:35  warmerda
 * fixed fatal bug with nWordSize for unpacked arrays in read/write block
 *
 * Revision 1.13  2002/12/21 21:13:04  warmerda
 * fixed memory leak of colortable
 *
 * Revision 1.12  2002/11/23 18:54:17  warmerda
 * added CREATIONDATATYPES metadata for drivers
 *
 * Revision 1.11  2002/11/20 05:18:09  warmerda
 * added AddBand() implementation
 *
 * Revision 1.10  2002/09/04 06:50:37  warmerda
 * avoid static driver pointers
 *
 * Revision 1.9  2002/06/12 21:12:25  warmerda
 * update to metadata based driver info
 *
 * Revision 1.8  2002/06/10 21:31:57  warmerda
 * preserve projection and geotransform
 *
 * Revision 1.7  2002/05/29 16:01:54  warmerda
 * fixed SetColorInterpretation
 *
 * Revision 1.6  2002/04/12 17:37:31  warmerda
 * added colortable support
 *
 * Revision 1.5  2002/03/01 16:45:53  warmerda
 * added support for retaining nodata value
 *
 * Revision 1.4  2001/10/26 20:03:28  warmerda
 * added C entry point for creating MEMRasterBand
 *
 * Revision 1.3  2001/07/18 04:51:57  warmerda
 * added CPL_CVSID
 *
 * Revision 1.2  2000/07/19 19:07:04  warmerda
 * break linkage between MEMDataset and MEMRasterBand
 *
 * Revision 1.1  2000/07/19 15:55:11  warmerda
 * New
 *
 */

#include "memdataset.h"
#include "cpl_string.h"

CPL_CVSID("$Id$");

/************************************************************************/
/*                        MEMCreateRasterBand()                         */
/************************************************************************/

GDALRasterBandH MEMCreateRasterBand( GDALDataset *poDS, int nBand, 
                                    GByte *pabyData, GDALDataType eType, 
                                    int nPixelOffset, int nLineOffset, 
                                    int bAssumeOwnership )

{
    return (GDALRasterBandH) 
        new MEMRasterBand( poDS, nBand, pabyData, eType, nPixelOffset, 
                           nLineOffset, bAssumeOwnership );
}

/************************************************************************/
/*                           MEMRasterBand()                            */
/************************************************************************/

MEMRasterBand::MEMRasterBand( GDALDataset *poDS, int nBand,
                              GByte *pabyDataIn, GDALDataType eTypeIn, 
                              int nPixelOffsetIn, int nLineOffsetIn,
                              int bAssumeOwnership )

{
    //CPLDebug( "MEM", "MEMRasterBand(%p)", this );

    this->poDS = poDS;
    this->nBand = nBand;

    this->eAccess = poDS->GetAccess();

    eDataType = eTypeIn;

    nBlockXSize = poDS->GetRasterXSize();
    nBlockYSize = 1;

    if( nPixelOffsetIn == 0 )
        nPixelOffsetIn = GDALGetDataTypeSize(eTypeIn) / 8;

    if( nLineOffsetIn == 0 )
        nLineOffsetIn = nPixelOffsetIn * nBlockXSize;

    nPixelOffset = nPixelOffsetIn;
    nLineOffset = nLineOffsetIn;
    bOwnData = bAssumeOwnership;

    pabyData = pabyDataIn;

    bNoDataSet  = FALSE;

    poColorTable = NULL;
    
    eColorInterp = GCI_Undefined;

    papszCategoryNames = NULL;
    dfOffset = 0.0;
    dfScale = 1.0;
    pszUnitType = NULL;
}

/************************************************************************/
/*                           ~MEMRasterBand()                           */
/************************************************************************/

MEMRasterBand::~MEMRasterBand()

{
    //CPLDebug( "MEM", "~MEMRasterBand(%p)", this );
    if( bOwnData )
    {
        //CPLDebug( "MEM", "~MEMRasterBand() - free raw data." );
        VSIFree( pabyData );
    }

    if( poColorTable != NULL )
        delete poColorTable;

    CPLFree( pszUnitType );
    CSLDestroy( papszCategoryNames );
}


/************************************************************************/
/*                             IReadBlock()                             */
/************************************************************************/

CPLErr MEMRasterBand::IReadBlock( int nBlockXOff, int nBlockYOff,
                                   void * pImage )

{
    int     nWordSize = GDALGetDataTypeSize( eDataType ) / 8;
    CPLAssert( nBlockXOff == 0 );

    if( nPixelOffset == nWordSize )
    {
        memcpy( pImage, 
                pabyData+nLineOffset*nBlockYOff, 
                nPixelOffset * nBlockXSize );
    }
    else
    {
        GByte *pabyCur = pabyData + nLineOffset*nBlockYOff;

        for( int iPixel = 0; iPixel < nBlockXSize; iPixel++ )
        {
            memcpy( ((GByte *) pImage) + iPixel*nWordSize, 
                    pabyCur + iPixel*nPixelOffset, 
                    nWordSize );
        }
    }

    return CE_None;
}

/************************************************************************/
/*                            IWriteBlock()                             */
/************************************************************************/

CPLErr MEMRasterBand::IWriteBlock( int nBlockXOff, int nBlockYOff,
                                     void * pImage )

{
    int     nWordSize = GDALGetDataTypeSize( eDataType ) / 8;
    CPLAssert( nBlockXOff == 0 );

    if( nPixelOffset == nWordSize )
    {
        memcpy( pabyData+nLineOffset*nBlockYOff, 
                pImage, 
                nPixelOffset * nBlockXSize );
    }
    else
    {
        GByte *pabyCur = pabyData + nLineOffset*nBlockYOff;

        for( int iPixel = 0; iPixel < nBlockXSize; iPixel++ )
        {
            memcpy( pabyCur + iPixel*nPixelOffset, 
                    ((GByte *) pImage) + iPixel*nWordSize, 
                    nWordSize );
        }
    }

    return CE_None;
}

/************************************************************************/
/*                            GetNoDataValue()                          */
/************************************************************************/
double MEMRasterBand::GetNoDataValue( int *pbSuccess )

{
    if( pbSuccess )
        *pbSuccess = bNoDataSet;

    if( bNoDataSet )
        return dfNoData;
    else
        return 0.0;
}

/************************************************************************/
/*                            SetNoDataValue()                          */
/************************************************************************/
CPLErr MEMRasterBand::SetNoDataValue( double dfNewValue )
{
    dfNoData = dfNewValue;
    bNoDataSet = TRUE;

    return CE_None;
}

/************************************************************************/
/*                       GetColorInterpretation()                       */
/************************************************************************/

GDALColorInterp MEMRasterBand::GetColorInterpretation()

{
    if( poColorTable != NULL )
        return GCI_PaletteIndex;
    else
        return eColorInterp;
}

/************************************************************************/
/*                       SetColorInterpretation()                       */
/************************************************************************/

CPLErr MEMRasterBand::SetColorInterpretation( GDALColorInterp eGCI )

{
    eColorInterp = eGCI;

    return CE_None;
}

/************************************************************************/
/*                           GetColorTable()                            */
/************************************************************************/

GDALColorTable *MEMRasterBand::GetColorTable()

{
    return poColorTable;
}

/************************************************************************/
/*                           SetColorTable()                            */
/************************************************************************/

CPLErr MEMRasterBand::SetColorTable( GDALColorTable *poCT )

{
    if( poColorTable != NULL )
        delete poColorTable;

    if( poCT == NULL )
        poColorTable = NULL;
    else
        poColorTable = poCT->Clone();

    return CE_None;
}

/************************************************************************/
/*                            GetUnitType()                             */
/************************************************************************/

const char *MEMRasterBand::GetUnitType()

{
    if( pszUnitType == NULL )
        return "";
    else
        return pszUnitType;
}

/************************************************************************/
/*                            SetUnitType()                             */
/************************************************************************/

CPLErr MEMRasterBand::SetUnitType( const char *pszNewValue )

{
    CPLFree( pszUnitType );
    
    if( pszNewValue == NULL )
        pszUnitType = NULL;
    else
        pszUnitType = CPLStrdup(pszNewValue);

    return CE_None;
}

/************************************************************************/
/*                             GetOffset()                              */
/************************************************************************/

double MEMRasterBand::GetOffset( int *pbSuccess )

{
    if( pbSuccess != NULL )
        *pbSuccess = TRUE;

    return dfOffset;
}

/************************************************************************/
/*                             SetOffset()                              */
/************************************************************************/

CPLErr MEMRasterBand::SetOffset( double dfNewOffset )

{
    dfOffset = dfNewOffset;
    return CE_None;
}

/************************************************************************/
/*                              GetScale()                              */
/************************************************************************/

double MEMRasterBand::GetScale( int *pbSuccess )

{
    if( pbSuccess != NULL )
        *pbSuccess = TRUE;

    return dfScale;
}

/************************************************************************/
/*                              SetScale()                              */
/************************************************************************/

CPLErr MEMRasterBand::SetScale( double dfNewScale )

{
    dfScale = dfNewScale;
    return CE_None;
}

/************************************************************************/
/*                          GetCategoryNames()                          */
/************************************************************************/

char **MEMRasterBand::GetCategoryNames()

{
    return papszCategoryNames;
}

/************************************************************************/
/*                          SetCategoryNames()                          */
/************************************************************************/

CPLErr MEMRasterBand::SetCategoryNames( char ** papszNewNames )

{
    CSLDestroy( papszCategoryNames );
    papszCategoryNames = CSLDuplicate( papszNewNames );

    return CE_None;
}

/************************************************************************/
/* ==================================================================== */
/*      MEMDataset                                                     */
/* ==================================================================== */
/************************************************************************/


/************************************************************************/
/*                            MEMDataset()                             */
/************************************************************************/

MEMDataset::MEMDataset()

{
    pszProjection = NULL;
    bGeoTransformSet = FALSE;
    adfGeoTransform[0] = 0.0;
    adfGeoTransform[1] = 1.0;
    adfGeoTransform[2] = 0.0;
    adfGeoTransform[3] = 0.0;
    adfGeoTransform[4] = 0.0;
    adfGeoTransform[5] = -1.0;
}

/************************************************************************/
/*                            ~MEMDataset()                            */
/************************************************************************/

MEMDataset::~MEMDataset()

{
    FlushCache();
    CPLFree( pszProjection );
}

/************************************************************************/
/*                          GetProjectionRef()                          */
/************************************************************************/

const char *MEMDataset::GetProjectionRef()

{
    if( pszProjection == NULL )
        return "";
    else
        return pszProjection;
}

/************************************************************************/
/*                           SetProjection()                            */
/************************************************************************/

CPLErr MEMDataset::SetProjection( const char *pszProjectionIn )

{
    CPLFree( pszProjection );
    pszProjection = CPLStrdup( pszProjectionIn );

    return CE_None;
}

/************************************************************************/
/*                          GetGeoTransform()                           */
/************************************************************************/

CPLErr MEMDataset::GetGeoTransform( double *padfGeoTransform )

{
    memcpy( padfGeoTransform, adfGeoTransform, sizeof(double) * 6 );
    if( bGeoTransformSet )
        return CE_None;
    else
        return CE_Failure;
}

/************************************************************************/
/*                          SetGeoTransform()                           */
/************************************************************************/

CPLErr MEMDataset::SetGeoTransform( double *padfGeoTransform )

{
    memcpy( adfGeoTransform, padfGeoTransform, sizeof(double) * 6 );
    bGeoTransformSet = TRUE;

    return CE_None;
}

/************************************************************************/
/*                              AddBand()                               */
/*                                                                      */
/*      Add a new band to the dataset, allowing creation options to     */
/*      specify the existing memory to use, otherwise create new        */
/*      memory.                                                         */
/************************************************************************/

CPLErr MEMDataset::AddBand( GDALDataType eType, char **papszOptions )

{
    int nBandId = GetRasterCount() + 1;
    GByte *pData;
    int   nPixelSize = (GDALGetDataTypeSize(eType) / 8);

/* -------------------------------------------------------------------- */
/*      Do we need to allocate the memory ourselves?  This is the       */
/*      simple case.                                                    */
/* -------------------------------------------------------------------- */
    if( CSLFetchNameValue( papszOptions, "DATAPOINTER" ) == NULL )
    {

        pData = (GByte *) 
            CPLCalloc(nPixelSize, GetRasterXSize() * GetRasterYSize() );

        if( pData == NULL )
        {
            CPLError( CE_Failure, CPLE_OutOfMemory,
                      "Unable to create band arrays ... out of memory." );
            return CE_Failure;
        }

        SetBand( nBandId,
                 new MEMRasterBand( this, nBandId, pData, eType, nPixelSize, 
                                    nPixelSize * GetRasterXSize(), TRUE ) );

        return CE_None;
    }

/* -------------------------------------------------------------------- */
/*      Get layout of memory and other flags.                           */
/* -------------------------------------------------------------------- */
    const char *pszOption;
    int nPixelOffset, nLineOffset;

    pData = (GByte *) strtol(CSLFetchNameValue(papszOptions,"DATAPOINTER"),
                            NULL, 0 );
    
    pszOption = CSLFetchNameValue(papszOptions,"PIXELOFFSET");
    if( pszOption == NULL )
        nPixelOffset = nPixelSize;
    else
        nPixelOffset = atoi(pszOption);

    pszOption = CSLFetchNameValue(papszOptions,"LINEOFFSET");
    if( pszOption == NULL )
        nLineOffset = GetRasterXSize() * nPixelOffset;
    else
        nLineOffset = atoi(pszOption);

    SetBand( nBandId,
             new MEMRasterBand( this, nBandId, pData, eType, 
                                nPixelOffset, nLineOffset, FALSE ) );

    return CE_None;
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

GDALDataset *MEMDataset::Open( GDALOpenInfo * poOpenInfo )

{
    char    **papszOptions;

/* -------------------------------------------------------------------- */
/*      Do we have the special filename signature for MEM format        */
/*      description strings?                                            */
/* -------------------------------------------------------------------- */
    if( !EQUALN(poOpenInfo->pszFilename,"MEM:::",6) 
        || poOpenInfo->fp != NULL )
        return NULL;

    papszOptions = CSLTokenizeStringComplex(poOpenInfo->pszFilename+6, ",",
                                            TRUE, FALSE );

/* -------------------------------------------------------------------- */
/*      Verify we have all required fields                              */
/* -------------------------------------------------------------------- */
    if( CSLFetchNameValue( papszOptions, "PIXELS" ) == NULL
        || CSLFetchNameValue( papszOptions, "LINES" ) == NULL
        || CSLFetchNameValue( papszOptions, "DATAPOINTER" ) == NULL )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
              "Missing required field (one of PIXELS, LINES or DATAPOINTER)\n"
              "Unable to access in-memory array." );

        CSLDestroy( papszOptions );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Create the new GTiffDataset object.                             */
/* -------------------------------------------------------------------- */
    MEMDataset *poDS;

    poDS = new MEMDataset();

    poDS->nRasterXSize = atoi(CSLFetchNameValue(papszOptions,"PIXELS"));
    poDS->nRasterYSize = atoi(CSLFetchNameValue(papszOptions,"LINES"));

    poDS->eAccess = GA_Update;

/* -------------------------------------------------------------------- */
/*      Extract other information.                                      */
/* -------------------------------------------------------------------- */
    const char *pszOption;
    GDALDataType eType;
    int nBands, nPixelOffset, nLineOffset, nBandOffset;
    GByte *pabyData;

    pszOption = CSLFetchNameValue(papszOptions,"BANDS");
    if( pszOption == NULL )
        nBands = 1;
    else
        nBands = atoi(pszOption);

    pszOption = CSLFetchNameValue(papszOptions,"DATATYPE");
    if( pszOption == NULL )
        eType = GDT_Byte;
    else
        eType = (GDALDataType) atoi(pszOption);

    pszOption = CSLFetchNameValue(papszOptions,"PIXELOFFSET");
    if( pszOption == NULL )
        nPixelOffset = GDALGetDataTypeSize(eType) / 8;
    else
        nPixelOffset = atoi(pszOption);

    pszOption = CSLFetchNameValue(papszOptions,"LINEOFFSET");
    if( pszOption == NULL )
        nLineOffset = poDS->nRasterXSize * nPixelOffset;
    else
        nLineOffset = atoi(pszOption);

    pszOption = CSLFetchNameValue(papszOptions,"BANDOFFSET");
    if( pszOption == NULL )
        nBandOffset = nLineOffset * poDS->nRasterYSize;
    else
        nBandOffset = atoi(pszOption);

    pabyData = (GByte *) strtol(CSLFetchNameValue(papszOptions,"DATAPOINTER"),
                                NULL, 0 );

/* -------------------------------------------------------------------- */
/*      Create band information objects.                                */
/* -------------------------------------------------------------------- */
    for( int iBand = 0; iBand < nBands; iBand++ )
    {
        poDS->SetBand( iBand+1, 
                       new MEMRasterBand( poDS, iBand+1, 
                                          pabyData + iBand * nBandOffset,
                                          eType, nPixelOffset, nLineOffset, 
                                          FALSE ) );
    }

/* -------------------------------------------------------------------- */
/*      Try to return a regular handle on the file.                     */
/* -------------------------------------------------------------------- */
    return poDS;
}

/************************************************************************/
/*                               Create()                               */
/************************************************************************/

GDALDataset *MEMDataset::Create( const char * pszFilename,
                                 int nXSize, int nYSize, int nBands,
                                 GDALDataType eType,
                                 char ** /* notdef: papszParmList */ )

{
/* -------------------------------------------------------------------- */
/*      First allocate band data, verifying that we can get enough      */
/*      memory.                                                         */
/* -------------------------------------------------------------------- */
    GByte  	**papBandData;
    int   	iBand;
    int         nWordSize = GDALGetDataTypeSize(eType) / 8;

    papBandData = (GByte **) CPLCalloc(sizeof(void *),nBands);
    for( iBand = 0; iBand < nBands; iBand++ )
    {
        papBandData[iBand] = (GByte *) VSICalloc( nWordSize, nXSize * nYSize );
        if( papBandData[iBand] == NULL )
        {
            for( iBand = 0; iBand < nBands; iBand++ )
            {
                if( papBandData[iBand] )
                    VSIFree( papBandData[iBand] );
            }

            CPLError( CE_Failure, CPLE_OutOfMemory,
                      "Unable to create band arrays ... out of memory." );
            return NULL;
        }
    }

/* -------------------------------------------------------------------- */
/*      Create the new GTiffDataset object.                             */
/* -------------------------------------------------------------------- */
    MEMDataset *poDS;

    poDS = new MEMDataset();

    poDS->nRasterXSize = nXSize;
    poDS->nRasterYSize = nYSize;
    poDS->eAccess = GA_Update;

/* -------------------------------------------------------------------- */
/*      Create band information objects.                                */
/* -------------------------------------------------------------------- */
    for( iBand = 0; iBand < nBands; iBand++ )
    {
        poDS->SetBand( iBand+1, 
                       new MEMRasterBand( poDS, iBand+1, papBandData[iBand],
                                          eType, 0, 0, TRUE ) );
    }

    CPLFree( papBandData );

/* -------------------------------------------------------------------- */
/*      Try to return a regular handle on the file.                     */
/* -------------------------------------------------------------------- */
    return poDS;
}

/************************************************************************/
/*                          GDALRegister_MEM()                          */
/************************************************************************/

void GDALRegister_MEM()

{
    GDALDriver	*poDriver;

    if( GDALGetDriverByName( "MEM" ) == NULL )
    {
        poDriver = new GDALDriver();
        
        poDriver->SetDescription( "MEM" );
        poDriver->SetMetadataItem( GDAL_DMD_LONGNAME, 
                                   "In Memory Raster" );
        poDriver->SetMetadataItem( GDAL_DMD_CREATIONDATATYPES, 
                                   "Byte Int16 UInt16 Int32 UInt32 Float32 Float64 CInt16 CInt32 CFloat32 CFloat64" );

        poDriver->pfnOpen = MEMDataset::Open;
        poDriver->pfnCreate = MEMDataset::Create;

        GetGDALDriverManager()->RegisterDriver( poDriver );
    }
}

