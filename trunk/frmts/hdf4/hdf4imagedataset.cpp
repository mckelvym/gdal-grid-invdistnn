/******************************************************************************
 * $Id$
 *
 * Project:  Hierarchical Data Format Release 4 (HDF4)
 * Purpose:  Read subdatasets of HDF4 file.
 *           This driver initially based on code supplied by Markus Neteler
 * Author:   Andrey Kiselev, dron@remotesensing.org
 *
 ******************************************************************************
 * Copyright (c) 2002, Andrey Kiselev <dron@remotesensing.org>
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
 * Revision 1.37  2004/11/26 19:39:18  fwarmerdam
 * avoid initialization warning from purify
 *
 * Revision 1.36  2004/09/08 17:55:13  dron
 * Few problems fixed.
 *
 * Revision 1.35  2004/06/19 21:37:31  dron
 * Use HDF-EOS library for appropriate datasets; major cpde rewrite.
 *
 * Revision 1.34  2003/11/07 15:48:49  dron
 * GetDataType() moved to HDF4Dataset class.
 *
 * Revision 1.33  2003/10/31 19:12:47  dron
 * Read properly GR datasets with multiple samples per pixel.
 *
 * Revision 1.32  2003/10/25 10:34:39  dron
 * Disable GCP collecting for MODIS_L1B datasets.
 *
 * Revision 1.31  2003/09/28 05:45:02  dron
 * More logic to distinguish dimesions.
 *
 * Revision 1.30  2003/09/05 18:15:43  dron
 * Fixes in ASTER DEM zone detection.
 *
 * Revision 1.29  2003/06/30 14:44:28  dron
 * Fixed problem introduced with Hyperion support.
 *
 * Revision 1.28  2003/06/26 20:42:31  dron
 * Support for Hyperion Level 1 data product.
 *
 * Revision 1.27  2003/06/25 08:26:18  dron
 * Support for Aster Level 1A/1B/2 products.
 *
 * Revision 1.26  2003/06/12 15:07:56  dron
 * Added support for SeaWiFS Level 3 Standard Mapped Image Products.
 *
 * Revision 1.25  2003/06/10 09:32:31  dron
 * Added support for MODIS Level 3 products.
 *
 * Revision 1.24  2003/05/21 14:11:43  dron
 * MODIS Level 1B earth-view (EV) product now supported.
 *
 * Revision 1.23  2003/03/31 12:51:35  dron
 * GetNoData()/SetNoData() functions added.
 *
 * Revision 1.22  2003/02/28 17:07:37  dron
 * Global metadata now combined with local ones.
 *
 * Revision 1.21  2003/01/28 21:19:04  dron
 * DFNT_CHAR8 type was used instead of DFNT_INT8
 *
 * Revision 1.20  2003/01/27 16:45:16  dron
 * Fixed problems with wrong count in SDsetattr() calls.
 *
 * Revision 1.19  2002/12/30 15:07:45  dron
 * SetProjCS() call removed.
 */

#include <string.h>
#include <math.h>

#include "hdf.h"
#include "mfhdf.h"

#include "HdfEosDef.h"

#include "gdal_priv.h"
#include "cpl_string.h"
#include "ogr_spatialref.h"

#include "hdf4dataset.h"

CPL_CVSID("$Id$");

CPL_C_START
void    GDALRegister_HDF4(void);
CPL_C_END

// Signature to recognize files written by GDAL
const char      *pszGDALSignature =
        "Created with GDAL (http://www.remotesensing.org/gdal/)";

//const double    PI = 3.14159265358979323846;

/************************************************************************/
/* ==================================================================== */
/*                              HDF4ImageDataset                        */
/* ==================================================================== */
/************************************************************************/

class HDF4ImageDataset : public HDF4Dataset
{
    friend class HDF4ImageRasterBand;

    char        *pszFilename;
    int32       iGR, iPal, iDataset;
    int32       iRank, iNumType, nAttrs, iInterlaceMode, iPalInterlaceMode, iPalDataType;
    int32       nComps, nPalEntries;
    int32       aiDimSizes[MAX_VAR_DIMS];
    int         iXDim, iYDim, iBandDim;
    char        **papszLocalMetadata;
#define    N_COLOR_ENTRIES    256
    uint8       aiPaletteData[N_COLOR_ENTRIES][3]; // XXX: Static array for now
    char        szName[65];
    char        *pszSubdatasetName;
    char        *pszFieldName;

    GDALColorTable *poColorTable;

    int         bHasGeoTransfom;
    double      adfGeoTransform[6];
    char        *pszProjection;
    char        *pszGCPProjection;
    GDAL_GCP    *pasGCPList;
    int         nGCPCount;

    void                ReadCoordinates( const char*, double*, double* );
    void                ToUTM( OGRSpatialReference *, double *, double * );
    char**              GetSwatAttrs( int32 hSW, char **papszMetadata );
    char**              GetGridAttrs( int32 hGD, char **papszMetadata );

  public:
                HDF4ImageDataset();
                ~HDF4ImageDataset();
    
    static GDALDataset  *Open( GDALOpenInfo * );
    static GDALDataset  *Create( const char * pszFilename,
                                 int nXSize, int nYSize, int nBands,
                                 GDALDataType eType, char ** papszParmList );
    virtual void        FlushCache( void );
    CPLErr              GetGeoTransform( double * padfTransform );
    virtual CPLErr      SetGeoTransform( double * );
    const char          *GetProjectionRef();
    virtual CPLErr      SetProjection( const char * );
    virtual int         GetGCPCount();
    virtual const char  *GetGCPProjection();
    virtual const GDAL_GCP *GetGCPs();
};

/************************************************************************/
/* ==================================================================== */
/*                            HDF4ImageRasterBand                       */
/* ==================================================================== */
/************************************************************************/

class HDF4ImageRasterBand : public GDALRasterBand
{
    friend class HDF4ImageDataset;

    int         bNoDataSet;
    double      dfNoDataValue;

  public:

                HDF4ImageRasterBand( HDF4ImageDataset *, int, GDALDataType );
    
    virtual CPLErr          IReadBlock( int, int, void * );
    virtual CPLErr          IWriteBlock( int, int, void * );
    virtual GDALColorInterp GetColorInterpretation();
    virtual GDALColorTable *GetColorTable();
    virtual double	    GetNoDataValue( int * );
    virtual CPLErr	    SetNoDataValue( double );
};

/************************************************************************/
/*                           HDF4ImageRasterBand()                      */
/************************************************************************/

HDF4ImageRasterBand::HDF4ImageRasterBand( HDF4ImageDataset *poDS, int nBand,
                                          GDALDataType eType )

{
    this->poDS = poDS;
    this->nBand = nBand;
    eDataType = eType;
    bNoDataSet = FALSE;
    dfNoDataValue = -9999.0;

    nBlockXSize = poDS->GetRasterXSize();
    nBlockYSize = 1;
}

/************************************************************************/
/*                             IReadBlock()                             */
/************************************************************************/

CPLErr HDF4ImageRasterBand::IReadBlock( int nBlockXOff, int nBlockYOff,
                                        void * pImage )
{
    HDF4ImageDataset    *poGDS = (HDF4ImageDataset *) poDS;
    int32               iStart[MAX_NC_DIMS], iEdges[MAX_NC_DIMS];
    CPLErr              eErr = CE_None;

    if( poGDS->eAccess == GA_Update )
    {
        memset( pImage, 0,
                nBlockXSize * nBlockYSize * GDALGetDataTypeSize(eDataType) / 8 );
        return CE_None;
    }

    switch ( poGDS->iDatasetType )
    {
        case HDF4_SDS:
            {
                int32   iSDS = SDselect( poGDS->hSD, poGDS->iDataset );
                /* HDF rank:
                A rank 2 dataset is an image read in scan-line order (2D). 
                A rank 3 dataset is a series of images which are read in
                an image at a time to form a volume.
                A rank 4 dataset may be thought of as a series of volumes.

                The "iStart" array specifies the multi-dimensional index of the
                starting corner of the hyperslab to read. The values are zero
                based.

                The "edge" array specifies the number of values to read along
                each dimension of the hyperslab.

                The "iStride" array allows for sub-sampling along each
                dimension. If a iStride value is specified for a dimension,
                that many values will be skipped over when reading along that
                dimension. Specifying iStride = NULL in the C interface or
                iStride = 1 in either interface specifies contiguous reading
                of data. If the iStride values are set to 0, SDreaddata
                returns FAIL (or -1). No matter what iStride value is
                provided, data is always placed contiguously in buffer.
                */
                switch ( poGDS->iRank )
                {
                    case 4:     // 4Dim: volume-time
                    // FIXME: needs sample file. Does not works currently.
                    iStart[3] = 0/* range: 0--aiDimSizes[3]-1 */;       iEdges[3] = 1;
                    iStart[2] = 0/* range: 0--aiDimSizes[2]-1 */;       iEdges[2] = 1;
                    iStart[1] = nBlockYOff; iEdges[1] = nBlockYSize;
                    iStart[0] = nBlockXOff; iEdges[0] = nBlockXSize;
                    break;
                    case 3: // 3Dim: volume
                    iStart[poGDS->iBandDim] = nBand - 1;
                    iEdges[poGDS->iBandDim] = 1;
                
                    iStart[poGDS->iYDim] = nBlockYOff;
                    iEdges[poGDS->iYDim] = nBlockYSize;
                
                    iStart[poGDS->iXDim] = nBlockXOff;
                    iEdges[poGDS->iXDim] = nBlockXSize;
                    break;
                    case 2: // 2Dim: rows/cols
                    iStart[poGDS->iYDim] = nBlockYOff;
                    iEdges[poGDS->iYDim] = nBlockYSize;
                
                    iStart[poGDS->iXDim] = nBlockXOff;
                    iEdges[poGDS->iXDim] = nBlockXSize;
                    break;
                }
                
                // Read HDF SDS array
                SDreaddata( iSDS, iStart, NULL, iEdges, pImage );
                
                SDendaccess( iSDS );
            }
            break;

        case HDF4_GR:
        {
            int     nDataTypeSize =
                GDALGetDataTypeSize(poGDS->GetDataType(poGDS->iNumType)) / 8;
            GByte    *pbBuffer = (GByte *)
                CPLMalloc(nBlockXSize*nBlockYSize*poGDS->iRank*nBlockYSize);
            int     i, j;
            
            iStart[poGDS->iYDim] = nBlockYOff;
            iEdges[poGDS->iYDim] = nBlockYSize;
            
            iStart[poGDS->iXDim] = nBlockXOff;
            iEdges[poGDS->iXDim] = nBlockXSize;

            GRreadimage( poGDS->iGR, iStart, NULL, iEdges, pbBuffer );

            for ( i = 0, j = (nBand - 1) * nDataTypeSize;
                  i < nBlockXSize * nDataTypeSize;
                  i += nDataTypeSize, j += poGDS->nBands * nDataTypeSize )
                memcpy( (GByte *)pImage + i, pbBuffer + j, nDataTypeSize );

            CPLFree( pbBuffer );
        }
        break;

        case HDF4_EOS:
            {
                switch ( poGDS->iSubdatasetType )
                {
                    case EOS_GRID:
                        {
                            int32   hGD;

                            hGD = GDattach( poGDS->hHDF4,
                                            poGDS->pszSubdatasetName );
                            switch ( poGDS->iRank )
                            {
                                case 3: // 3Dim: volume
                                    iStart[poGDS->iBandDim] = nBand - 1;
                                    iEdges[poGDS->iBandDim] = 1;
                                
                                    iStart[poGDS->iYDim] = nBlockYOff;
                                    iEdges[poGDS->iYDim] = nBlockYSize;
                                
                                    iStart[poGDS->iXDim] = nBlockXOff;
                                    iEdges[poGDS->iXDim] = nBlockXSize;
                                    break;
                                case 2: // 2Dim: rows/cols
                                    iStart[poGDS->iYDim] = nBlockYOff;
                                    iEdges[poGDS->iYDim] = nBlockYSize;
                                
                                    iStart[poGDS->iXDim] = nBlockXOff;
                                    iEdges[poGDS->iXDim] = nBlockXSize;
                                    break;
                            }
                            GDreadfield( hGD, poGDS->pszFieldName,
                                         iStart, NULL, iEdges, pImage );
                            GDdetach( hGD );
                        }
                        break;
                    
                    case EOS_SWATH:
                        {
                            int32   hSW;

                            hSW = SWattach( poGDS->hHDF4,
                                            poGDS->pszSubdatasetName );
                            switch ( poGDS->iRank )
                            {
                                case 3: // 3Dim: volume
                                    iStart[poGDS->iBandDim] = nBand - 1;
                                    iEdges[poGDS->iBandDim] = 1;
                                
                                    iStart[poGDS->iYDim] = nBlockYOff;
                                    iEdges[poGDS->iYDim] = nBlockYSize;
                                
                                    iStart[poGDS->iXDim] = nBlockXOff;
                                    iEdges[poGDS->iXDim] = nBlockXSize;
                                    break;
                                case 2: // 2Dim: rows/cols
                                    iStart[poGDS->iYDim] = nBlockYOff;
                                    iEdges[poGDS->iYDim] = nBlockYSize;
                                
                                    iStart[poGDS->iXDim] = nBlockXOff;
                                    iEdges[poGDS->iXDim] = nBlockXSize;
                                    break;
                            }
                            SWreadfield( hSW, poGDS->pszFieldName,
                                         iStart, NULL, iEdges, pImage );
                            SWdetach( hSW );
                        }
                        break;
                    default:
                        break;
                }
            }
            break;

        default:
            eErr = CE_Failure;
            break;
    }

    return eErr;
}

/************************************************************************/
/*                            IWriteBlock()                             */
/************************************************************************/

CPLErr HDF4ImageRasterBand::IWriteBlock( int nBlockXOff, int nBlockYOff,
                                         void * pImage )
{
    HDF4ImageDataset    *poGDS = (HDF4ImageDataset *)poDS;
    int32               iStart[MAX_NC_DIMS], iEdges[MAX_NC_DIMS];
    CPLErr              eErr = CE_None;

    CPLAssert( poGDS != NULL
               && nBlockXOff >= 0
               && nBlockYOff >= 0
               && pImage != NULL );

    switch ( poGDS->iRank )
    {
        case 3:
            {
                int32   iSDS = SDselect( poGDS->hSD, poGDS->iDataset );

                iStart[poGDS->iBandDim] = nBand - 1;
                iEdges[poGDS->iBandDim] = 1;
            
                iStart[poGDS->iYDim] = nBlockYOff;
                iEdges[poGDS->iYDim] = nBlockYSize;
            
                iStart[poGDS->iXDim] = nBlockXOff;
                iEdges[poGDS->iXDim] = nBlockXSize;

                if ( (SDwritedata( iSDS, iStart, NULL,
                                   iEdges, (VOIDP)pImage )) < 0 )
                    eErr = CE_Failure;

                SDendaccess( iSDS );
            }
            break;

        case 2:
            {
                int32 iSDS = SDselect( poGDS->hSD, nBand - 1 );
                iStart[poGDS->iYDim] = nBlockYOff;
                iEdges[poGDS->iYDim] = nBlockYSize;
            
                iStart[poGDS->iXDim] = nBlockXOff;
                iEdges[poGDS->iXDim] = nBlockXSize;

                if ( (SDwritedata( iSDS, iStart, NULL,
                                   iEdges, (VOIDP)pImage )) < 0 )
                    eErr = CE_Failure;

                SDendaccess( iSDS );
            }
            break;

        default:
            eErr = CE_Failure;
            break;
    }

    return eErr;
}

/************************************************************************/
/*                           GetColorTable()                            */
/************************************************************************/

GDALColorTable *HDF4ImageRasterBand::GetColorTable()
{
    HDF4ImageDataset    *poGDS = (HDF4ImageDataset *) poDS;

    return poGDS->poColorTable;
}

/************************************************************************/
/*                       GetColorInterpretation()                       */
/************************************************************************/

GDALColorInterp HDF4ImageRasterBand::GetColorInterpretation()
{
    HDF4ImageDataset    *poGDS = (HDF4ImageDataset *) poDS;

    if ( poGDS->iDatasetType == HDF4_SDS )
        return GCI_GrayIndex;
    else if ( poGDS->iDatasetType == HDF4_GR )
    {
        if ( poGDS->poColorTable != NULL )
            return GCI_PaletteIndex;
        else if ( poGDS->nBands != 1 )
        {
            if ( nBand == 1 )
                return GCI_RedBand;
            else if ( nBand == 2 )
                return GCI_GreenBand;
            else if ( nBand == 3 )
                return GCI_BlueBand;
            else if ( nBand == 4 )
                return GCI_AlphaBand;
            else
                return GCI_Undefined;
        }
        else
            return GCI_GrayIndex;
    }
    else
        return GCI_GrayIndex;
}

/************************************************************************/
/*                           GetNoDataValue()                           */
/************************************************************************/

double HDF4ImageRasterBand::GetNoDataValue( int * pbSuccess )

{
    if( pbSuccess )
        *pbSuccess = bNoDataSet;

    return dfNoDataValue;
}

/************************************************************************/
/*                           SetNoDataValue()                           */
/************************************************************************/

CPLErr HDF4ImageRasterBand::SetNoDataValue( double dfNoData )

{
    bNoDataSet = TRUE;
    dfNoDataValue = dfNoData;

    return CE_None;
}

/************************************************************************/
/* ==================================================================== */
/*                              HDF4ImageDataset                        */
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                           HDF4ImageDataset()                         */
/************************************************************************/

HDF4ImageDataset::HDF4ImageDataset()
{
    pszFilename = NULL;
    hSD = 0;
    hGR = 0;
    iGR = 0;
    iDatasetType = HDF4_UNKNOWN;
    pszSubdatasetName = NULL;
    pszFieldName = NULL;
    papszLocalMetadata = NULL;
    poColorTable = NULL;
    pszProjection = CPLStrdup( "" );
    pszGCPProjection = CPLStrdup( "" );
    bHasGeoTransfom = FALSE;
    adfGeoTransform[0] = 0.0;
    adfGeoTransform[1] = 1.0;
    adfGeoTransform[2] = 0.0;
    adfGeoTransform[3] = 0.0;
    adfGeoTransform[4] = 0.0;
    adfGeoTransform[5] = 1.0;

    nGCPCount = 0;
    pasGCPList = NULL;

}

/************************************************************************/
/*                            ~HDF4ImageDataset()                       */
/************************************************************************/

HDF4ImageDataset::~HDF4ImageDataset()
{
    FlushCache();
    
    if ( pszFilename )
        CPLFree( pszFilename );
    if ( hSD > 0 )
        SDend( hSD );
    if ( iGR > 0 )
        GRendaccess( iGR );
    if ( hGR > 0 )
        GRend( hGR );
    if ( pszSubdatasetName )
        CPLFree( pszSubdatasetName );
    if ( pszFieldName )
        CPLFree( pszFieldName );
    if ( papszLocalMetadata )
        CSLDestroy( papszLocalMetadata );
    if ( poColorTable != NULL )
        delete poColorTable;
    if ( pszProjection )
        CPLFree( pszProjection );
    if ( pszGCPProjection )
        CPLFree( pszGCPProjection );
    if( nGCPCount > 0 )
    {
        for( int i = 0; i < nGCPCount; i++ )
        {
            if ( pasGCPList[i].pszId )
                CPLFree( pasGCPList[i].pszId );
            if ( pasGCPList[i].pszInfo )
                CPLFree( pasGCPList[i].pszInfo );
        }

        CPLFree( pasGCPList );
    }
}

/************************************************************************/
/*                          GetGeoTransform()                           */
/************************************************************************/

CPLErr HDF4ImageDataset::GetGeoTransform( double * padfTransform )
{
    memcpy( padfTransform, adfGeoTransform, sizeof(double) * 6 );

    if ( !bHasGeoTransfom )
        return CE_Failure;

    return CE_None;
}

/************************************************************************/
/*                          SetGeoTransform()                           */
/************************************************************************/

CPLErr HDF4ImageDataset::SetGeoTransform( double * padfTransform )
{
    bHasGeoTransfom = TRUE;
    memcpy( adfGeoTransform, padfTransform, sizeof(double) * 6 );

    return CE_None;
}

/************************************************************************/
/*                          GetProjectionRef()                          */
/************************************************************************/

const char *HDF4ImageDataset::GetProjectionRef()

{
    return pszProjection;
}

/************************************************************************/
/*                          SetProjection()                             */
/************************************************************************/

CPLErr HDF4ImageDataset::SetProjection( const char *pszNewProjection )

{
    if ( pszProjection )
        CPLFree( pszProjection );
    pszProjection = CPLStrdup( pszNewProjection );

    return CE_None;
}

/************************************************************************/
/*                            GetGCPCount()                             */
/************************************************************************/

int HDF4ImageDataset::GetGCPCount()

{
    return nGCPCount;
}

/************************************************************************/
/*                          GetGCPProjection()                          */
/************************************************************************/

const char *HDF4ImageDataset::GetGCPProjection()

{
    if( nGCPCount > 0 )
        return pszGCPProjection;
    else
        return "";
}

/************************************************************************/
/*                               GetGCPs()                              */
/************************************************************************/

const GDAL_GCP *HDF4ImageDataset::GetGCPs()
{
    return pasGCPList;
}

/************************************************************************/
/*                             FlushCache()                             */
/************************************************************************/

void HDF4ImageDataset::FlushCache()

{
    int         iBand;
    char        *pszName;
    const char  *pszValue;
    
    GDALDataset::FlushCache();

    if( eAccess == GA_ReadOnly )
        return;

    // Write out transformation matrix
    pszValue = CPLSPrintf( "%f, %f, %f, %f, %f, %f",
                                   adfGeoTransform[0], adfGeoTransform[1],
                                   adfGeoTransform[2], adfGeoTransform[3],
                                   adfGeoTransform[4], adfGeoTransform[5] );
    if ( (SDsetattr( hSD, "TransformationMatrix", DFNT_CHAR8,
                     strlen(pszValue) + 1, pszValue )) < 0 )
    {
        CPLDebug( "HDF4Image",
                  "Cannot write transformation matrix to output file" );
    }

    // Write out projection
    if ( pszProjection != NULL && !EQUAL( pszProjection, "" ) )
    {
        if ( (SDsetattr( hSD, "Projection", DFNT_CHAR8,
                         strlen(pszProjection) + 1, pszProjection )) < 0 )
            {
                CPLDebug( "HDF4Image",
                          "Cannot write projection information to output file");
            }
    }

    // Store all metadata from source dataset as HDF attributes
    if ( papszMetadata )
    {
        char    **papszMeta = papszMetadata;

        while ( *papszMeta )
        {
            pszValue = CPLParseNameValue( *papszMeta++, &pszName );
            if ( (SDsetattr( hSD, pszName, DFNT_CHAR8,
                             strlen(pszValue) + 1, pszValue )) < 0 );
            {
                CPLDebug( "HDF4Image",
                          "Cannot write metadata information to output file");
            }

            CPLFree( pszName );
        }
    }

    // Write out NoData values
    for ( iBand = 1; iBand <= nBands; iBand++ )
    {
        HDF4ImageRasterBand *poBand =
            (HDF4ImageRasterBand *)GetRasterBand(iBand);

        if ( poBand->bNoDataSet )
        {
            pszName = CPLStrdup( CPLSPrintf( "NoDataValue%d", iBand ) );
            pszValue = CPLSPrintf( "%lf", poBand->dfNoDataValue );
            if ( (SDsetattr( hSD, pszName, DFNT_CHAR8,
                             strlen(pszValue) + 1, pszValue )) < 0 )
                {
                    CPLDebug( "HDF4Image",
                              "Cannot write NoData value for band %d "
                              "to output file", iBand);
                }

            CPLFree( pszName );
       }
    }

    // Write out band descriptions
    for ( iBand = 1; iBand <= nBands; iBand++ )
    {
        HDF4ImageRasterBand *poBand =
            (HDF4ImageRasterBand *)GetRasterBand(iBand);

        pszName = CPLStrdup( CPLSPrintf( "BandDesc%d", iBand ) );
        pszValue = poBand->GetDescription();
        if ( pszValue != NULL && !EQUAL( pszValue, "" ) )
        {
            if ( (SDsetattr( hSD, pszName, DFNT_CHAR8,
                             strlen(pszValue) + 1, pszValue )) < 0 )
            {
                CPLDebug( "HDF4Image",
                          "Cannot write band's %d description to output file",
                          iBand);
            }
        }

        CPLFree( pszName );
    }
}

/************************************************************************/
/*                                ToUTM()                               */
/************************************************************************/

void HDF4ImageDataset::ToUTM( OGRSpatialReference *poProj,
                              double *pdfGeoX, double *pdfGeoY )
{
    OGRCoordinateTransformation *poTransform = NULL;
    OGRSpatialReference *poLatLong = NULL;
    poLatLong = poProj->CloneGeogCS();
    poTransform = OGRCreateCoordinateTransformation( poLatLong, poProj );
    
    if( poTransform != NULL )
        poTransform->Transform( 1, pdfGeoX, pdfGeoY, NULL );
        
    if( poTransform != NULL )
        delete poTransform;

    if( poLatLong != NULL )
        delete poLatLong;
}

/************************************************************************/
/*                            ReadCoordinates()                         */
/************************************************************************/

void HDF4ImageDataset::ReadCoordinates( const char *pszString,
                                        double *pdfX, double *pdfY )
{
    char **papszStrList;
    papszStrList = CSLTokenizeString2( pszString, ", ", 0 );
    *pdfX = atof(papszStrList[0]);
    *pdfY = atof(papszStrList[1]);
    CSLDestroy( papszStrList );
}

/************************************************************************/
/*                            GetSwatAttrs()                            */
/************************************************************************/

char**  HDF4ImageDataset::GetSwatAttrs( int32 hSW, char **papszMetadata )
{
    int32       nStrBufSize = 0;

    if ( SWinqattrs( hSW, NULL, &nStrBufSize ) > 0 && nStrBufSize > 0 )
    {
        char    *pszAttrList;
        char    **papszAttributes;
        int     i, nAttrs;

        pszAttrList = (char *)CPLMalloc( nStrBufSize + 1 );
        SWinqattrs( hSW, pszAttrList, &nStrBufSize );
#if DEBUG
        CPLDebug( "HDF4Image", "List of attributes in swath %s: %s",
                  pszFieldName, pszAttrList );
#endif
        papszAttributes = CSLTokenizeString2( pszAttrList, ",",
                                              CSLT_HONOURSTRINGS );
        nAttrs = CSLCount( papszAttributes );
        for ( i = 0; i < nAttrs; i++ )
        {
            int32       iNumType, nValues;
            void	*pData = NULL;
            char	*pszTemp = NULL;

            SWattrinfo( hSW, papszAttributes[i], &iNumType, &nValues );

            if ( iNumType == DFNT_CHAR8 || iNumType == DFNT_UCHAR8 )
                pData = CPLMalloc( (nValues + 1) * GetDataTypeSize(iNumType) );
            else
                pData = CPLMalloc( nValues * GetDataTypeSize(iNumType) );

            SWreadattr( hSW, papszAttributes[i], pData );

            if ( iNumType == DFNT_CHAR8 || iNumType == DFNT_UCHAR8 )
            {
                ((char *)pData)[nValues] = '\0';
                papszMetadata = CSLAddNameValue( papszMetadata,
                                                 papszAttributes[i], 
                                                 (const char *) pData );
            }
            else
            {
                pszTemp = SPrintArray( GetDataType(iNumType), pData,
                                       nValues, ", " );
                papszMetadata = CSLAddNameValue( papszMetadata,
                                                 papszAttributes[i], pszTemp );
                if ( pszTemp )
                    CPLFree( pszTemp );
            }
            
            if ( pData )
                CPLFree( pData );

        }

        CSLDestroy( papszAttributes );
        CPLFree( pszAttrList );
    }

    return papszMetadata;
}

/************************************************************************/
/*                            GetGridAttrs()                            */
/************************************************************************/

char**  HDF4ImageDataset::GetGridAttrs( int32 hGD, char **papszMetadata )
{
    int32       nStrBufSize = 0;

    if ( GDinqattrs( hGD, NULL, &nStrBufSize ) > 0 && nStrBufSize > 0 )
    {
        char    *pszAttrList;
        char    **papszAttributes;
        int     i, nAttrs;

        pszAttrList = (char *)CPLMalloc( nStrBufSize + 1 );
        GDinqattrs( hGD, pszAttrList, &nStrBufSize );
#if DEBUG
        CPLDebug( "HDF4Image", "List of attributes in grid %s: %s",
                  pszFieldName, pszAttrList );
#endif
        papszAttributes = CSLTokenizeString2( pszAttrList, ",",
                                              CSLT_HONOURSTRINGS );
        nAttrs = CSLCount( papszAttributes );
        for ( i = 0; i < nAttrs; i++ )
        {
            int32       iNumType, nValues;
            void	*pData = NULL;
            char	*pszTemp = NULL;

            GDattrinfo( hGD, papszAttributes[i], &iNumType, &nValues );

            if ( iNumType == DFNT_CHAR8 || iNumType == DFNT_UCHAR8 )
                pData = CPLMalloc( (nValues + 1) * GetDataTypeSize(iNumType) );
            else
                pData = CPLMalloc( nValues * GetDataTypeSize(iNumType) );

            GDreadattr( hGD, papszAttributes[i], pData );

            if ( iNumType == DFNT_CHAR8 || iNumType == DFNT_UCHAR8 )
            {
                ((char *)pData)[nValues] = '\0';
                papszMetadata = CSLAddNameValue( papszMetadata,
                                                 papszAttributes[i], 
                                                 (const char *) pData );
            }
            else
            {
                pszTemp = SPrintArray( GetDataType(iNumType), pData,
                                       nValues, ", " );
                papszMetadata = CSLAddNameValue( papszMetadata,
                                                 papszAttributes[i], pszTemp );
                if ( pszTemp )
                    CPLFree( pszTemp );
            }
            
            if ( pData )
                CPLFree( pData );

        }

        CSLDestroy( papszAttributes );
        CPLFree( pszAttrList );
    }

    return papszMetadata;
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

GDALDataset *HDF4ImageDataset::Open( GDALOpenInfo * poOpenInfo )
{
    int     i;
    
    if( !EQUALN( poOpenInfo->pszFilename, "HDF4_SDS:", 9 ) &&
        !EQUALN( poOpenInfo->pszFilename, "HDF4_GR:", 8 ) &&
        !EQUALN( poOpenInfo->pszFilename, "HDF4_GD:", 8 ) &&
        !EQUALN( poOpenInfo->pszFilename, "HDF4_EOS:", 9 ) )
        return NULL;

/* -------------------------------------------------------------------- */
/*      Create a corresponding GDALDataset.                             */
/* -------------------------------------------------------------------- */
    char                **papszSubdatasetName;
    HDF4ImageDataset    *poDS;

    poDS = new HDF4ImageDataset( );

    poDS->fp = poOpenInfo->fp;
    poOpenInfo->fp = NULL;

    papszSubdatasetName = CSLTokenizeString2( poOpenInfo->pszFilename,
                                              ":", CSLT_HONOURSTRINGS );
    if ( CSLCount( papszSubdatasetName ) != 4
         && CSLCount( papszSubdatasetName ) != 5 )
    {
        CSLDestroy( papszSubdatasetName );
        return NULL;
    }

    poDS->pszFilename = CPLStrdup( papszSubdatasetName[2] );

    if( EQUAL( papszSubdatasetName[0], "HDF4_SDS" ) )
        poDS->iDatasetType = HDF4_SDS;
    else if ( EQUAL( papszSubdatasetName[0], "HDF4_GR" ) )
        poDS->iDatasetType = HDF4_GR;
    else if ( EQUAL( papszSubdatasetName[0], "HDF4_EOS" ) )
        poDS->iDatasetType = HDF4_EOS;
    else
        poDS->iDatasetType = HDF4_UNKNOWN;
    
    if( EQUAL( papszSubdatasetName[1], "GDAL_HDF4" ) )
        poDS->iSubdatasetType = GDAL_HDF4;
    else if( EQUAL( papszSubdatasetName[1], "EOS_GRID" ) )
        poDS->iSubdatasetType = EOS_GRID;
    else if( EQUAL( papszSubdatasetName[1], "EOS_SWATH" ) )
        poDS->iSubdatasetType = EOS_SWATH;
    else if( EQUAL( papszSubdatasetName[1], "SEAWIFS_L3" ) )
        poDS->iSubdatasetType= SEAWIFS_L3;
    else if( EQUAL( papszSubdatasetName[1], "HYPERION_L1" ) )
        poDS->iSubdatasetType= HYPERION_L1;
    else
        poDS->iSubdatasetType = UNKNOWN;

    // Does our file still here?
    if ( !Hishdf( poDS->pszFilename ) )
        return NULL;

/* -------------------------------------------------------------------- */
/*      Try opening the dataset.                                        */
/* -------------------------------------------------------------------- */
    int32       iAttribute, nValues, iAttrNumType;
    char        szAttrName[MAX_NC_NAME];
    
/* -------------------------------------------------------------------- */
/*      Select SDS or GR to read from.                                  */
/* -------------------------------------------------------------------- */
    if ( poDS->iDatasetType == HDF4_EOS )
    {
        poDS->pszSubdatasetName = CPLStrdup( papszSubdatasetName[3] );
        poDS->pszFieldName = CPLStrdup( papszSubdatasetName[4] );
    }
    else
        poDS->iDataset = atoi( papszSubdatasetName[3] );
    CSLDestroy( papszSubdatasetName );

    switch ( poDS->iDatasetType )
    {
        case HDF4_EOS:
        {
            switch ( poDS->iSubdatasetType )
            {

/* -------------------------------------------------------------------- */
/*  HDF-EOS Swath.                                                      */
/* -------------------------------------------------------------------- */
                case EOS_SWATH:
                {
                    int32   hSW,  nDimensions, nStrBufSize;
                    char    **papszDimList = NULL,
                        **papszGeolocations = NULL, **papszDimMap = NULL;
                    char    *pszDimList = NULL,
                        *pszGeoList = NULL, *pszDimMaps = NULL;
                    int32   *paiRank = NULL, *paiNumType = NULL,
                        *paiOffset = NULL, *paiIncrement = NULL;
                    int     nDimCount;
                    
                    if( poOpenInfo->eAccess == GA_ReadOnly )
                        poDS->hHDF4 = SWopen( poDS->pszFilename, DFACC_READ );
                    else
                        poDS->hHDF4 = SWopen( poDS->pszFilename, DFACC_WRITE );
                    
                    if( poDS->hHDF4 <= 0 )
                        return( NULL );

                    hSW = SWattach( poDS->hHDF4, poDS->pszSubdatasetName );

                    nDimensions = SWnentries( hSW, HDFE_NENTDIM, &nStrBufSize );
                    pszDimList = (char *)CPLMalloc( nStrBufSize + 1 );
                    SWfieldinfo( hSW, poDS->pszFieldName, &poDS->iRank,
                                 poDS->aiDimSizes, &poDS->iNumType, pszDimList );
#if DEBUG
                    CPLDebug( "HDF4Image",
                              "List of dimensions in swath %s: %s",
                              poDS->pszFieldName, pszDimList );
#endif

                    papszDimList = CSLTokenizeString2( pszDimList, ",",
                                                       CSLT_HONOURSTRINGS );
                    nDimCount = CSLCount( papszDimList );

                    poDS->papszLocalMetadata = 
                        poDS->GetSwatAttrs( hSW, poDS->papszLocalMetadata );

                    // Search for the "Bands" name or take the first dimension
                    // as a number of bands
                    if ( poDS->iRank == 2 )
                        poDS->nBands = 1;
                    else
                    {
                        poDS->nBands = poDS->aiDimSizes[0];
                        /*for ( i = 0; i < nDimCount; i++ )
                          {
                          if ( EQUALN( papszDimList[i], "Band", 4 ) )
                          {
                          poDS->nBands = poDS->aiDimSizes[i];
                          break;
                          }
                          }*/
                    }

                    // Search for the "XDim" and "YDim" names or take the last
                    // two dimensions as X and Y sizes
                    poDS->iXDim = nDimCount - 1;
                    poDS->iYDim = nDimCount - 2;
                    /*for ( i = 0; i < nDimCount; i++ )
                      {
                      if ( EQUALN( papszDimList[i], "X", 1 ) )
                      poDS->iXDim = i;
                      else if ( EQUALN( papszDimList[i], "Y", 1 ) )
                      poDS->iYDim = i;
                      }*/

                    // Retrieve NODATA value. FIXME: doesn't work.
                    //SWgetfillvalue( hGD, poDS->pszFieldName, );

/* -------------------------------------------------------------------- */
/*  Retrieve geolocation fields.                                        */
/* -------------------------------------------------------------------- */
                    /*char    szXGeo[8192], szYGeo[8192];
                      int32   nDataFields, nDimMaps;

                      nDataFields = SWnentries( hSW, HDFE_NENTGFLD, &nStrBufSize );
                      pszGeoList = (char *)CPLMalloc( nStrBufSize + 1 );
                      paiRank = (int32 *)CPLMalloc( nDataFields * sizeof(int32) );
                      paiNumType = (int32 *)CPLMalloc( nDataFields * sizeof(int32) );
                      SWinqgeofields( hSW, pszGeoList, paiRank, paiNumType );

                      #if DEBUG
                      CPLDebug( "HDF4Image",
                      "Number of geolocation fields in swath %s: %d",
                      poDS->pszSubdatasetName, nDataFields );
                      CPLDebug( "HDF4Image",
                      "List of geolocation fields in swath %s: %s",
                      poDS->pszSubdatasetName, pszGeoList );
                      CPLDebug( "HDF4Image", "Geolocation fields ranks: %s",
                      SPrintArray( GDT_UInt32, paiRank, nDataFields, "," ) );
                      #endif
                      papszGeolocations = CSLTokenizeString2( pszGeoList, ",",
                      CSLT_HONOURSTRINGS );
                      // Read geolocation data
                      nDimMaps = SWnentries( hSW, HDFE_NENTMAP, &nStrBufSize );
                      pszDimMaps = (char *)CPLMalloc( nStrBufSize + 1 );
                      paiOffset = (int32 *)CPLMalloc( nDataFields * sizeof(int32) );
                      paiIncrement = (int32 *)CPLMalloc( nDataFields * sizeof(int32) );
                      SWinqmaps( hSW, pszDimMaps, paiOffset, paiIncrement );

                      #if DEBUG
                      CPLDebug( "HDF4Image",
                      "List of geolocation maps in swath %s: %s",
                      poDS->pszSubdatasetName, pszDimMaps );
                      CPLDebug( "HDF4Image", "Geolocation map offsets: %s",
                      SPrintArray( GDT_UInt32, paiOffset, nDataFields, "," ) );
                      CPLDebug( "HDF4Image", "Geolocation map increments: %s",
                      SPrintArray( GDT_UInt32, paiIncrement,
                      nDataFields, "," ) );
                      #endif

                      papszDimMap = CSLTokenizeString2( pszDimMaps, ",",
                      CSLT_HONOURSTRINGS );
                      for ( i = 0; i < CSLCount(papszDimMap); i++ )
                      {
                      if ( strstr(papszDimMap[i], papszDimList[poDS->iXDim]) )
                      {
                      strncpy( szXGeo, papszDimList[poDS->iXDim], 8192 );
                      char *pszTemp1 = papszDimMap[i];
                      char *pszTemp2 = szXGeo;
                      while ( *pszTemp1 && *pszTemp1 != '/' )
                      *pszTemp2++ = *pszTemp1++;
                      *pszTemp2 = '\0';
                      }

                      else if ( strstr(papszDimMap[i], papszDimList[poDS->iYDim]) )
                      {
                      strncpy( szYGeo, papszDimList[poDS->iYDim], 8192 );
                      char *pszTemp1 = papszDimMap[i];
                      char *pszTemp2 = szYGeo;
                      while ( *pszTemp1 && *pszTemp1 != '/' )
                      *pszTemp2++ = *pszTemp1++;
                      *pszTemp2 = '\0';
                      }
                      }

                      for ( i = 0; i < CSLCount(papszGeolocations); i++ )
                      {
                      int32   *paiStart, *paiEdges;
                      if ( EQUALN(papszGeolocations[i], "Latitude", 8) )
                      {
                      char    szGeoDimList[8192];
                      char    **papszGeoDimList = NULL;
                      int32   iRank, iNumType;
                      int32   aiDimSizes[MAX_VAR_DIMS];
                      double  *padfLat;
                      int     iGCP;
                            
                      SWfieldinfo( hSW, papszGeolocations[i], &iRank,
                      aiDimSizes, &iNumType, szGeoDimList );
                      papszGeoDimList = CSLTokenizeString2( szGeoDimList,
                      ",", CSLT_HONOURSTRINGS );

                      #if DEBUG
                      CPLDebug( "HDF4Image",
                      "List of dimensions in geolocation field %s: %s",
                      papszGeolocations[i], szGeoDimList );
                      #endif

                      paiStart = (int32 *)CPLMalloc( iRank * sizeof(int32) );
                      paiEdges = (int32 *)CPLMalloc( iRank * sizeof(int32) );
                      paiStart[0] = 0;
                      paiStart[1] = 0;
                      paiEdges[0] =
                      aiDimSizes[CSLFindString( papszGeoDimList,
                      szXGeo )];
                      paiEdges[1] =
                      aiDimSizes[CSLFindString( papszGeoDimList,
                      szYGeo )];
                      poDS->nGCPCount = paiEdges[0] * paiEdges[1];
                      poDS->pasGCPList = (GDAL_GCP *)
                      CPLCalloc( poDS->nGCPCount, sizeof( GDAL_GCP ) );
                      GDALInitGCPs( poDS->nGCPCount, poDS->pasGCPList );
                      padfLat = (double *)
                      CPLMalloc( poDS->nGCPCount * sizeof(double));
                            
                      SWreadfield( hSW, papszGeolocations[i],
                      paiStart, NULL, paiEdges, padfLat );

                      for ( iGCP = 0; iGCP < poDS->nGCPCount, iGCP++ )
                      {
                                // GCPs in Level 1A dataset are in geocentric
                                // coordinates. Convert them in geodetic (we
                                // will convert latitudes only, longitudes
                                // does not need to be converted, because
                                // they are the same).
                                // This calculation valid for WGS84 datum only.
                                poDS->pasGCPList[index].dfGCPY = 
                                atan(tan(padfLat[index]*PI/180)/0.99330562)*180/PI;
                                poDS->pasGCPList[index].dfGCPX = 0.0;//pdfLong[index];
                                poDS->pasGCPList[index].dfGCPZ = 0.0;

                                poDS->pasGCPList[index].dfGCPPixel =
                                piLatticeX[index] + 0.5;
                                poDS->pasGCPList[index].dfGCPLine =
                                piLatticeY[index] + 0.5;
                                poDS->nGCPCount++;
                                }

                                CPLFree( padfLat );
                                CPLFree( paiStart );
                                CPLFree( paiEdges );
                                CSLDestroy( papszGeoDimList );
                                }
                                } */

                    CSLDestroy( papszDimMap );
                    CSLDestroy( papszGeolocations );
                    CPLFree( paiOffset );
                    CPLFree( paiIncrement );
                    CPLFree( paiNumType );
                    CPLFree( paiRank );
                    CPLFree( pszDimMaps );
                    CPLFree( pszGeoList );
                    CPLFree( pszDimList );

                    CSLDestroy( papszDimList );
                    SWdetach( hSW );
                }
                break;

/* -------------------------------------------------------------------- */
/*  HDF-EOS Grid.                                                       */
/* -------------------------------------------------------------------- */
                case EOS_GRID:
                {
                    int32   hGD, iProjCode = 0, iZoneCode = 0, iSphereCode = 0;
                    int32   nXSize, nYSize;
                    char    szDimList[8192];
                    char    **papszDimList = NULL;
                    int     nDimCount;
                    double  adfUpLeft[2], adfLowRight[2], adfProjParms[15];
                    
                    if( poOpenInfo->eAccess == GA_ReadOnly )
                        poDS->hHDF4 = GDopen( poDS->pszFilename, DFACC_READ );
                    else
                        poDS->hHDF4 = GDopen( poDS->pszFilename, DFACC_WRITE );
                    
                    if( poDS->hHDF4 <= 0 )
                        return( NULL );

                    hGD = GDattach( poDS->hHDF4, poDS->pszSubdatasetName );
                    GDfieldinfo( hGD, poDS->pszFieldName, &poDS->iRank,
                                 poDS->aiDimSizes, &poDS->iNumType, szDimList );
#if DEBUG
                    CPLDebug( "HDF4Image",
                              "List of dimensions in grid %s: %s",
                              poDS->pszFieldName, szDimList);
#endif
                    papszDimList = CSLTokenizeString2( szDimList, ",",
                                                       CSLT_HONOURSTRINGS );
                    nDimCount = CSLCount( papszDimList );

                    // Search for the "Bands" name or take the first dimension
                    // as a number of bands
                    if ( poDS->iRank == 2 )
                        poDS->nBands = 1;
                    else
                    {
                        poDS->nBands = poDS->aiDimSizes[0];
                        for ( i = 0; i < nDimCount; i++ )
                        {
                            if ( EQUALN( papszDimList[i], "Band", 4 ) )
                            {
                                poDS->nBands = poDS->aiDimSizes[i];
                                break;
                            }
                        }
                    }

                    // Search for the "XDim" and "YDim" names or take the last
                    // two dimensions as X and Y sizes
                    poDS->iXDim = nDimCount - 1;
                    poDS->iYDim = nDimCount - 2;
                    for ( i = 0; i < nDimCount; i++ )
                    {
                        if ( EQUALN( papszDimList[i], "X", 1 ) )
                            poDS->iXDim = i;
                        else if ( EQUALN( papszDimList[i], "Y", 1 ) )
                            poDS->iYDim = i;
                    }

                    // Retrieve projection information
                    if ( GDprojinfo( hGD, &iProjCode, &iZoneCode,
                                     &iSphereCode, adfProjParms) >= 0 )
                    {
#if DEBUG
                        CPLDebug( "HDF4Image",
                                  "Grid projection: "
                                  "projection code: %d, zone code %d, "
                                  "sphere code %d",
                                  iProjCode, iZoneCode, iSphereCode );
#endif
                        OGRSpatialReference oSRS;
                        oSRS.importFromUSGS( iProjCode, iZoneCode,
                                             adfProjParms, iSphereCode );
                        
                        if ( poDS->pszProjection )
                            CPLFree( poDS->pszProjection );
                        oSRS.exportToWkt( &poDS->pszProjection );
                    }

                    // Retrieve geotransformation matrix
                    if ( GDgridinfo( hGD, &nXSize, &nYSize,
                                     adfUpLeft, adfLowRight ) >= 0 )
                    {
#if DEBUG
                        CPLDebug( "HDF4Image",
                                  "Grid geolocation: "
                                  "top left X %f, top left Y %f, "
                                  "low right X %f, low right Y %f, "
                                  "cols %d, rows %d",
                                  adfUpLeft[0], adfUpLeft[1],
                                  adfLowRight[0], adfLowRight[1],
                                  nXSize, nYSize );
#endif
                        if ( iProjCode )
                        {
                            // For projected systems coordinates are in meters
                            poDS->adfGeoTransform[0] = adfUpLeft[0];
                            poDS->adfGeoTransform[3] = adfUpLeft[1];
                            poDS->adfGeoTransform[1] =
                                (adfLowRight[0] - adfUpLeft[0]) / nXSize;
                            poDS->adfGeoTransform[5] =
                                (adfLowRight[1] - adfUpLeft[1]) / nYSize;
                        }
                        else
                        {
                            // Handle angular geographic coordinates here
                            poDS->adfGeoTransform[0] =
                                CPLPackedDMSToDec(adfUpLeft[0]);
                            poDS->adfGeoTransform[3] =
                                CPLPackedDMSToDec(adfUpLeft[1]);
                            poDS->adfGeoTransform[1] =
                                (CPLPackedDMSToDec(adfLowRight[0]) -
                                 CPLPackedDMSToDec(adfUpLeft[0])) / nXSize;
                            poDS->adfGeoTransform[5] =
                                (CPLPackedDMSToDec(adfLowRight[1]) -
                                 CPLPackedDMSToDec(adfUpLeft[1])) / nYSize;
                        }
                        poDS->adfGeoTransform[2] = 0.0;
                        poDS->adfGeoTransform[4] = 0.0;
                        poDS->bHasGeoTransfom = TRUE;
                    }

                    // Retrieve NODATA value
                    //GDgetfillvalue( hGD, poDS->pszFieldName, );
                    poDS->papszLocalMetadata = 
                        poDS->GetGridAttrs( hGD, poDS->papszLocalMetadata );

                    CSLDestroy( papszDimList );
                    GDdetach( hGD );
                }
                break;
                
                default:
                    break;
            }
        }
        break;

/* -------------------------------------------------------------------- */
/*  'Plain' HDF scientific datasets.                                    */
/* -------------------------------------------------------------------- */
        case HDF4_SDS:
        {
            int32   iSDS;

/* -------------------------------------------------------------------- */
/*  Part, common for all types of SDSs.                                 */
/* -------------------------------------------------------------------- */
            if( poOpenInfo->eAccess == GA_ReadOnly )
                poDS->hHDF4 = Hopen( poDS->pszFilename, DFACC_READ, 0 );
            else
                poDS->hHDF4 = Hopen( poDS->pszFilename, DFACC_WRITE, 0 );
            
            if( poDS->hHDF4 <= 0 )
                return( NULL );

            poDS->hSD = SDstart( poDS->pszFilename, DFACC_READ );
            if ( poDS->hSD == -1 )
                return NULL;
                
            if ( poDS->ReadGlobalAttributes( poDS->hSD ) != CE_None )
                return NULL;

            memset( poDS->aiDimSizes, 0, sizeof(int32) * MAX_VAR_DIMS );
            iSDS = SDselect( poDS->hSD, poDS->iDataset );
            SDgetinfo( iSDS, poDS->szName, &poDS->iRank, poDS->aiDimSizes,
                       &poDS->iNumType, &poDS->nAttrs);

            // We will duplicate global metadata for every subdataset
            poDS->papszLocalMetadata =
                CSLDuplicate( poDS->papszGlobalMetadata );

            for ( iAttribute = 0; iAttribute < poDS->nAttrs; iAttribute++ )
            {
                SDattrinfo( iSDS, iAttribute, szAttrName,
                            &iAttrNumType, &nValues );
                poDS->papszLocalMetadata =
                    poDS->TranslateHDF4Attributes( iSDS, iAttribute,
                                                   szAttrName, iAttrNumType,
                                                   nValues,
                                                   poDS->papszLocalMetadata );
            }
            poDS->SetMetadata( poDS->papszLocalMetadata, "" );
            SDendaccess( iSDS );

            CPLDebug( "HDF4Image",
                      "aiDimSizes[0]=%d, aiDimSizes[1]=%d, "
                      "aiDimSizes[2]=%d, aiDimSizes[3]=%d",
                      poDS->aiDimSizes[0], poDS->aiDimSizes[1],
                      poDS->aiDimSizes[2], poDS->aiDimSizes[3] );

            switch( poDS->iRank )
            {
                case 2:
                    poDS->nBands = 1;
                    poDS->iXDim = 1;
                    poDS->iYDim = 0;
                    break;
                case 3:
                    if( poDS->aiDimSizes[0] < poDS->aiDimSizes[2] )
                    {
                        poDS->iBandDim = 0;
                        poDS->iXDim = 2;
                        poDS->iYDim = 1;
                    }
                    else
                    {
                        if( poDS->aiDimSizes[1] <= poDS->aiDimSizes[0] &&
                            poDS->aiDimSizes[1] <= poDS->aiDimSizes[2] )
                        {
                            poDS->iBandDim = 1;
                            poDS->iXDim = 2;
                            poDS->iYDim = 0;
                        }
                        else
                        {
                            poDS->iBandDim = 2;
                            poDS->iXDim = 1;
                            poDS->iYDim = 0;

                        }
                    }
                    poDS->nBands = poDS->aiDimSizes[poDS->iBandDim];
                    break;
                case 4: // FIXME
                    poDS->nBands = poDS->aiDimSizes[2] * poDS->aiDimSizes[3];
                    break;
                default:
                    break;
            }
        }
        break;

/* -------------------------------------------------------------------- */
/*  'Plain' HDF rasters.                                                */
/* -------------------------------------------------------------------- */
        case HDF4_GR:
            if( poOpenInfo->eAccess == GA_ReadOnly )
                poDS->hHDF4 = Hopen( poDS->pszFilename, DFACC_READ, 0 );
            else
                poDS->hHDF4 = Hopen( poDS->pszFilename, DFACC_WRITE, 0 );
            
            if( poDS->hHDF4 <= 0 )
                return( NULL );

            poDS->hGR = GRstart( poDS->hHDF4 );
            if ( poDS->hGR == -1 )
                return NULL;
                
            poDS->iGR = GRselect( poDS->hGR, poDS->iDataset );
            if ( GRgetiminfo( poDS->iGR, poDS->szName,
                              &poDS->iRank, &poDS->iNumType,
                              &poDS->iInterlaceMode, poDS->aiDimSizes,
                              &poDS->nAttrs ) != 0 )
                return NULL;

            // We will duplicate global metadata for every subdataset
            poDS->papszLocalMetadata = CSLDuplicate( poDS->papszGlobalMetadata );

            for ( iAttribute = 0; iAttribute < poDS->nAttrs; iAttribute++ )
            {
                GRattrinfo( poDS->iGR, iAttribute, szAttrName,
                            &iAttrNumType, &nValues );
                poDS->papszLocalMetadata = 
                    poDS->TranslateHDF4Attributes( poDS->iGR, iAttribute,
                                                   szAttrName, iAttrNumType, nValues, poDS->papszLocalMetadata );
            }
            poDS->SetMetadata( poDS->papszLocalMetadata, "" );
            // Read colour table
            GDALColorEntry oEntry;
                 
            poDS->iPal = GRgetlutid ( poDS->iGR, poDS->iDataset );
            if ( poDS->iPal != -1 )
            {
                GRgetlutinfo( poDS->iPal, &poDS->nComps, &poDS->iPalDataType,
                              &poDS->iPalInterlaceMode, &poDS->nPalEntries );
                GRreadlut( poDS->iPal, poDS->aiPaletteData );
                poDS->poColorTable = new GDALColorTable();
                for( i = 0; i < N_COLOR_ENTRIES; i++ )
                {
                    oEntry.c1 = poDS->aiPaletteData[i][0];
                    oEntry.c2 = poDS->aiPaletteData[i][1];
                    oEntry.c3 = poDS->aiPaletteData[i][2];
                    oEntry.c4 = 255;
                        
                    poDS->poColorTable->SetColorEntry( i, &oEntry );
                }
            }

            poDS->iXDim = 0;
            poDS->iYDim = 1;
            poDS->nBands = poDS->iRank;
            break;
        default:
            return NULL;
    }
    
    poDS->nRasterXSize = poDS->aiDimSizes[poDS->iXDim];
    poDS->nRasterYSize = poDS->aiDimSizes[poDS->iYDim];

    if ( poDS->iSubdatasetType == HYPERION_L1 )
    {
        // XXX: Hyperion SDSs has Height x Bands x Width dimensions scheme
        poDS->nBands = poDS->aiDimSizes[1];
        poDS->nRasterXSize = poDS->aiDimSizes[2];
        poDS->nRasterYSize = poDS->aiDimSizes[0];
    }

/* -------------------------------------------------------------------- */
/*      Create band information objects.                                */
/* -------------------------------------------------------------------- */
    for( i = 1; i <= poDS->nBands; i++ )
        poDS->SetBand( i, new HDF4ImageRasterBand( poDS, i,
                                                   poDS->GetDataType( poDS->iNumType ) ) );

/* -------------------------------------------------------------------- */
/*      Now we will handle particular types of HDF products. Every      */
/*      HDF product has its own structure.                              */
/* -------------------------------------------------------------------- */

    // Variables for reading georeferencing
    double          dfULX, dfULY, dfLRX, dfLRY;
    OGRSpatialReference oSRS;

    switch ( poDS->iSubdatasetType )
    {
/* -------------------------------------------------------------------- */
/*  HDF, created by GDAL.                                               */
/* -------------------------------------------------------------------- */
        case GDAL_HDF4:
        {
            const char  *pszValue;

            CPLDebug( "HDF4Image",
                      "Input dataset interpreted as GDAL_HDF4" );

            if ( (pszValue =
                  CSLFetchNameValue(poDS->papszGlobalMetadata,
                                    "Projection")) )
            {
                if ( poDS->pszProjection )
                    CPLFree( poDS->pszProjection );
                poDS->pszProjection = CPLStrdup( pszValue );
            }
            if ( (pszValue = CSLFetchNameValue(poDS->papszGlobalMetadata,
                                               "TransformationMatrix")) )
            {
                int i = 0;
                char *pszString = (char *) pszValue; 
                while ( *pszValue && i < 6 )
                {
                    poDS->adfGeoTransform[i++] = strtod(pszString, &pszString);
                    pszString++;
                }
                poDS->bHasGeoTransfom = TRUE;
            }
            for( i = 1; i <= poDS->nBands; i++ )
            {
                if ( (pszValue =
                      CSLFetchNameValue(poDS->papszGlobalMetadata,
                                        CPLSPrintf("BandDesc%d", i))) )
                    poDS->GetRasterBand( i )->SetDescription( pszValue );
            }
            for( i = 1; i <= poDS->nBands; i++ )
            {
                if ( (pszValue =
                      CSLFetchNameValue(poDS->papszGlobalMetadata,
                                        CPLSPrintf("NoDataValue%d", i))) )
                    poDS->GetRasterBand(i)->SetNoDataValue(atof(pszValue));
            }
        }
        break;

/* -------------------------------------------------------------------- */
/*      SeaWiFS Level 3 Standard Mapped Image Products.                 */
/*      Organized similar to MODIS Level 3 products.                    */
/* -------------------------------------------------------------------- */
        case SEAWIFS_L3:
        {
            CPLDebug( "HDF4Image", "Input dataset interpreted as SEAWIFS_L3" );

            // Read band description
            for ( i = 1; i <= poDS->nBands; i++ )
            {
                poDS->GetRasterBand( i )->SetDescription(
                    CSLFetchNameValue( poDS->papszGlobalMetadata, "Parameter" ) );
            }

            // Read coordinate system and geotransform matrix
            oSRS.SetWellKnownGeogCS( "WGS84" );
            
            if ( EQUAL(CSLFetchNameValue(poDS->papszGlobalMetadata,
                                         "Map Projection"),
                       "Equidistant Cylindrical") )
            {
                oSRS.SetEquirectangular( 0.0, 0.0, 0.0, 0.0 );
                oSRS.SetLinearUnits( SRS_UL_METER, 1 );
                if ( poDS->pszProjection )
                    CPLFree( poDS->pszProjection );
                oSRS.exportToWkt( &poDS->pszProjection );
            }

            dfULX = atof( CSLFetchNameValue(poDS->papszGlobalMetadata,
                                            "Westernmost Longitude") );
            dfULY = atof( CSLFetchNameValue(poDS->papszGlobalMetadata,
                                            "Northernmost Latitude") );
            dfLRX = atof( CSLFetchNameValue(poDS->papszGlobalMetadata,
                                            "Easternmost Longitude") );
            dfLRY = atof( CSLFetchNameValue(poDS->papszGlobalMetadata,
                                            "Southernmost Latitude") );
            poDS->ToUTM( &oSRS, &dfULX, &dfULY );
            poDS->ToUTM( &oSRS, &dfLRX, &dfLRY );
            poDS->adfGeoTransform[0] = dfULX;
            poDS->adfGeoTransform[3] = dfULY;
            poDS->adfGeoTransform[1] = (dfLRX - dfULX) / poDS->nRasterXSize;
            poDS->adfGeoTransform[5] = (dfULY - dfLRY) / poDS->nRasterYSize;
            if ( dfULY > 0)     // Northern hemisphere
                poDS->adfGeoTransform[5] = - poDS->adfGeoTransform[5];
            poDS->adfGeoTransform[2] = 0.0;
            poDS->adfGeoTransform[4] = 0.0;
            poDS->bHasGeoTransfom = TRUE;
        }
        break;

/* -------------------------------------------------------------------- */
/*      Hyperion Level 1.                                               */
/* -------------------------------------------------------------------- */
        case HYPERION_L1:
        {
            CPLDebug( "HDF4Image", "Input dataset interpreted as HYPERION_L1" );
        }
        break;

        default:
            break;
    }

    return( poDS );
}

/************************************************************************/
/*                               Create()                               */
/************************************************************************/

GDALDataset *HDF4ImageDataset::Create( const char * pszFilename,
                                       int nXSize, int nYSize, int nBands,
                                       GDALDataType eType,
                                       char **papszOptions )

{
/* -------------------------------------------------------------------- */
/*      Create the dataset.                                             */
/* -------------------------------------------------------------------- */
    HDF4ImageDataset    *poDS;
    const char          *pszSDSName;
    int                 iBand;
    int32               iSDS, aiDimSizes[MAX_VAR_DIMS];

    poDS = new HDF4ImageDataset();

/* -------------------------------------------------------------------- */
/*      Choose rank for the created dataset.                            */
/* -------------------------------------------------------------------- */
    poDS->iRank = 3;
    if ( CSLFetchNameValue( papszOptions, "RANK" ) != NULL &&
         EQUAL( CSLFetchNameValue( papszOptions, "RANK" ), "2" ) )
        poDS->iRank = 2;
    
    poDS->hSD = SDstart( pszFilename, DFACC_CREATE );
    if ( poDS->hSD == -1 )
    {
        CPLError( CE_Failure, CPLE_OpenFailed,
                  "Can't create HDF4 file %s", pszFilename );
        return NULL;
    }
    poDS->iXDim = 1;
    poDS->iYDim = 0;
    poDS->iBandDim = 2;
    aiDimSizes[poDS->iXDim] = nXSize;
    aiDimSizes[poDS->iYDim] = nYSize;
    aiDimSizes[poDS->iBandDim] = nBands;

    if ( poDS->iRank == 2 )
    {
        for ( iBand = 0; iBand < nBands; iBand++ )
        {
            pszSDSName = CPLSPrintf( "Band%d", iBand );
            switch ( eType )
            {
                case GDT_Float64:
                    iSDS = SDcreate( poDS->hSD, pszSDSName, DFNT_FLOAT64,
                                     poDS->iRank, aiDimSizes );
                    break;
                case GDT_Float32:
                    iSDS = SDcreate( poDS-> hSD, pszSDSName, DFNT_FLOAT32,
                                     poDS->iRank, aiDimSizes );
                    break;
                case GDT_UInt32:
                    iSDS = SDcreate( poDS->hSD, pszSDSName, DFNT_UINT32,
                                     poDS->iRank, aiDimSizes );
                    break;
                case GDT_UInt16:
                    iSDS = SDcreate( poDS->hSD, pszSDSName, DFNT_UINT16,
                                     poDS->iRank, aiDimSizes );
                    break;
                case GDT_Int32:
                    iSDS = SDcreate( poDS->hSD, pszSDSName, DFNT_INT32,
                                     poDS->iRank, aiDimSizes );
                    break;
                case GDT_Int16:
                    iSDS = SDcreate( poDS->hSD, pszSDSName, DFNT_INT16,
                                     poDS->iRank, aiDimSizes );
                    break;
                case GDT_Byte:
                default:
                    iSDS = SDcreate( poDS->hSD, pszSDSName, DFNT_UINT8,
                                     poDS->iRank, aiDimSizes );
                    break;
            }
            SDendaccess( iSDS );
        }
    }
    else if ( poDS->iRank == 3 )
    {
        pszSDSName = "3-dimensional Scientific Dataset";
        poDS->iDataset = 0;
        switch ( eType )
        {
            case GDT_Float64:
                iSDS = SDcreate( poDS->hSD, pszSDSName, DFNT_FLOAT64,
                                 poDS->iRank, aiDimSizes );
                break;
            case GDT_Float32:
                iSDS = SDcreate( poDS->hSD, pszSDSName, DFNT_FLOAT32,
                                 poDS->iRank, aiDimSizes );
                break;
            case GDT_UInt32:
                iSDS = SDcreate( poDS->hSD, pszSDSName, DFNT_UINT32,
                                 poDS->iRank, aiDimSizes );
                break;
            case GDT_UInt16:
                iSDS = SDcreate( poDS->hSD, pszSDSName, DFNT_UINT16,
                                 poDS->iRank, aiDimSizes );
                break;
            case GDT_Int32:
                iSDS = SDcreate( poDS->hSD, pszSDSName, DFNT_INT32,
                                 poDS->iRank, aiDimSizes );
                break;
            case GDT_Int16:
                iSDS = SDcreate( poDS->hSD, pszSDSName, DFNT_INT16,
                                 poDS->iRank, aiDimSizes );
                break;
            case GDT_Byte:
            default:
                iSDS = SDcreate( poDS->hSD, pszSDSName, DFNT_UINT8,
                                 poDS->iRank, aiDimSizes );
                break;
        }
    }
    else                                            // Should never happen
        return NULL;

    if ( iSDS < 0 )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "Can't create SDS with rank %d for file %s",
                  poDS->iRank, pszFilename );
        return NULL;
    }

    poDS->nRasterXSize = nXSize;
    poDS->nRasterYSize = nYSize;
    poDS->eAccess = GA_Update;
    poDS->iDatasetType = HDF4_SDS;
    poDS->iSubdatasetType = GDAL_HDF4;
    poDS->nBands = nBands;

/* -------------------------------------------------------------------- */
/*      Create band information objects.                                */
/* -------------------------------------------------------------------- */
    for( iBand = 1; iBand <= nBands; iBand++ )
        poDS->SetBand( iBand, new HDF4ImageRasterBand( poDS, iBand, eType ) );

    SDsetattr( poDS->hSD, "Signature", DFNT_CHAR8, strlen(pszGDALSignature) + 1,
               pszGDALSignature );
    
    return (GDALDataset *) poDS;
}

/************************************************************************/
/*                        GDALRegister_HDF4Image()                      */
/************************************************************************/

void GDALRegister_HDF4Image()

{
    GDALDriver  *poDriver;

    if( GDALGetDriverByName( "HDF4Image" ) == NULL )
    {
        poDriver = new GDALDriver();
        
        poDriver->SetDescription( "HDF4Image" );
        poDriver->SetMetadataItem( GDAL_DMD_LONGNAME, 
                                   "HDF4 Dataset" );
        poDriver->SetMetadataItem( GDAL_DMD_HELPTOPIC, 
                                   "frmt_hdf4.html" );
        poDriver->SetMetadataItem( GDAL_DMD_CREATIONDATATYPES, 
                                   "Byte Int16 UInt16 Int32 UInt32 Float32 Float64" );
        poDriver->SetMetadataItem( GDAL_DMD_CREATIONOPTIONLIST, 
"<CreationOptionList>"
"   <Option name='RANK' type='int' description='Rank of output SDS'/>"
"</CreationOptionList>" );

        poDriver->pfnOpen = HDF4ImageDataset::Open;
        poDriver->pfnCreate = HDF4ImageDataset::Create;

        GetGDALDriverManager()->RegisterDriver( poDriver );
    }
}

