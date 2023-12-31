/******************************************************************************
 * $Id: hdf5imagedataset.cpp 17802 2009-10-13 04:48:48Z chaitanya $
 *
 * Project:  Hierarchical Data Format Release 5 (HDF5)
 * Purpose:  Read BAG datasets.
 * Author:   Frank Warmerdam <warmerdam@pobox.com>
 *
 ******************************************************************************
 * Copyright (c) 2009, Frank Warmerdam <warmerdam@pobox.com>
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

#include "gh5_convenience.h"

#include "gdal_pam.h"
#include "gdal_priv.h"
#include "ogr_spatialref.h"
#include "cpl_string.h"

CPL_CVSID("$Id: hdf5imagedataset.cpp 17802 2009-10-13 04:48:48Z chaitanya $");

CPL_C_START
void    GDALRegister_BAG(void);
CPL_C_END

OGRErr OGR_SRS_ImportFromISO19115( OGRSpatialReference *poThis, 
                                   const char *pszISOXML );

/************************************************************************/
/* ==================================================================== */
/*                               BAGDataset                             */
/* ==================================================================== */
/************************************************************************/
class BAGDataset : public GDALPamDataset
{

    friend class BAGRasterBand;

    hid_t        hHDF5;

    char        *pszProjection;
    double       adfGeoTransform[6];

    void         LoadMetadata();

    char        *pszXMLMetadata;
    char        *apszMDList[2];
    
public:
    BAGDataset();
    ~BAGDataset();
    
    virtual CPLErr GetGeoTransform( double * );
    virtual const char *GetProjectionRef(void);
    virtual char      **GetMetadata( const char * pszDomain = "" );

    static GDALDataset  *Open( GDALOpenInfo * );
    static int          Identify( GDALOpenInfo * );
};

/************************************************************************/
/* ==================================================================== */
/*                               BAGRasterBand                          */
/* ==================================================================== */
/************************************************************************/
class BAGRasterBand : public GDALPamRasterBand
{
    friend class BAGDataset;

    hid_t       hDatasetID;
    hid_t       native;
    hid_t       dataspace;

    bool        bMinMaxSet;
    double      dfMinimum;
    double      dfMaximum;

public:
  
    BAGRasterBand( BAGDataset *, int );
    ~BAGRasterBand();

    bool                    Initialize( hid_t hDataset, const char *pszName );

    virtual CPLErr          IReadBlock( int, int, void * );
    virtual double	    GetNoDataValue( int * ); 

    virtual double GetMinimum( int *pbSuccess = NULL );
    virtual double GetMaximum(int *pbSuccess = NULL );
};

/************************************************************************/
/*                           BAGRasterBand()                            */
/************************************************************************/
BAGRasterBand::BAGRasterBand( BAGDataset *poDS, int nBand )

{
    this->poDS       = poDS;
    this->nBand      = nBand;
    
    hDatasetID = -1;
    bMinMaxSet = false;
}

/************************************************************************/
/*                           ~BAGRasterBand()                           */
/************************************************************************/

BAGRasterBand::~BAGRasterBand()
{
}

/************************************************************************/
/*                             Initialize()                             */
/************************************************************************/

bool BAGRasterBand::Initialize( hid_t hDatasetID, const char *pszName )

{
    SetDescription( pszName );

    this->hDatasetID = hDatasetID;

    hid_t datatype     = H5Dget_type( hDatasetID );
    dataspace          = H5Dget_space( hDatasetID );
    hid_t n_dims       = H5Sget_simple_extent_ndims( dataspace );
    native             = H5Tget_native_type( datatype, H5T_DIR_ASCEND );
    hsize_t dims[3], maxdims[3];

    eDataType = GH5_GetDataType( native );

    if( n_dims == 2 )
    {
        H5Sget_simple_extent_dims( dataspace, dims, maxdims );

        nRasterXSize = dims[1];
        nRasterYSize = dims[0];
    }
    else
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "Dataset not of rank 2." );
        return false;
    }

    nBlockXSize   = nRasterXSize;
    nBlockYSize   = 1;

/* -------------------------------------------------------------------- */
/*      Check for chunksize, and use it as blocksize for optimized      */
/*      reading.                                                        */
/* -------------------------------------------------------------------- */
    hid_t listid = H5Dget_create_plist( hDatasetID );
    if (listid>0)
    {
        if(H5Pget_layout(listid) == H5D_CHUNKED)
        {
            hsize_t panChunkDims[3];
            int nDimSize = H5Pget_chunk(listid, 3, panChunkDims);
            nBlockXSize   = panChunkDims[nDimSize-1];
            nBlockYSize   = panChunkDims[nDimSize-2];
        }
        H5Pclose(listid);
    }

/* -------------------------------------------------------------------- */
/*      Load min/max information.                                       */
/* -------------------------------------------------------------------- */
    if( EQUAL(pszName,"elevation") 
        && GH5_FetchAttribute( hDatasetID, "Maximum Elevation Value", 
                            dfMaximum ) 
        && GH5_FetchAttribute( hDatasetID, "Minimum Elevation Value", 
                               dfMinimum ) )
        bMinMaxSet = true;
    else if( EQUAL(pszName,"uncertainty") 
             && GH5_FetchAttribute( hDatasetID, "Maximum Uncertainty Value", 
                                    dfMaximum ) 
             && GH5_FetchAttribute( hDatasetID, "Minimum Uncertainty Value", 
                                    dfMinimum ) )
        bMinMaxSet = true;
    else if( EQUAL(pszName,"nominal_elevation") 
             && GH5_FetchAttribute( hDatasetID, "max_value", 
                                    dfMaximum ) 
             && GH5_FetchAttribute( hDatasetID, "min_value", 
                                    dfMinimum ) )
        bMinMaxSet = true;

    return true;
}

/************************************************************************/
/*                             GetMinimum()                             */
/************************************************************************/

double BAGRasterBand::GetMinimum( int * pbSuccess )

{
    if( bMinMaxSet )
    {
        if( pbSuccess )
            *pbSuccess = TRUE;
        return dfMinimum;
    }
    else
        return GDALRasterBand::GetMinimum( pbSuccess );
}

/************************************************************************/
/*                             GetMaximum()                             */
/************************************************************************/

double BAGRasterBand::GetMaximum( int * pbSuccess )

{
    if( bMinMaxSet )
    {
        if( pbSuccess )
            *pbSuccess = TRUE;
        return dfMaximum;
    }
    else
        return GDALRasterBand::GetMaximum( pbSuccess );
}

/************************************************************************/
/*                           GetNoDataValue()                           */
/************************************************************************/
double BAGRasterBand::GetNoDataValue( int * pbSuccess )

{
    if( pbSuccess )
        *pbSuccess = TRUE;

    if( EQUAL(GetDescription(),"elevation") )
        return  1000000.0;
    else if( EQUAL(GetDescription(),"uncertainty") )
        return 0.0;
    else if( EQUAL(GetDescription(),"nominal_elevation") )
        return 1000000.0;
    else
        return GDALPamRasterBand::GetNoDataValue( pbSuccess );
}

/************************************************************************/
/*                             IReadBlock()                             */
/************************************************************************/
CPLErr BAGRasterBand::IReadBlock( int nBlockXOff, int nBlockYOff,
                                  void * pImage )
{
    herr_t      status;
    hsize_t     count[3];
    H5OFFSET_TYPE offset[3];
    hid_t       memspace;
    hsize_t     col_dims[3];
    hsize_t     rank = 2;

    offset[0] = nRasterYSize - nBlockYOff*nBlockYSize - 1;
    offset[1] = nBlockXOff*nBlockXSize;
    count[0]  = nBlockYSize;
    count[1]  = nBlockXSize;

/* -------------------------------------------------------------------- */
/*      Select block from file space                                    */
/* -------------------------------------------------------------------- */
    status =  H5Sselect_hyperslab( dataspace,
                                   H5S_SELECT_SET, 
                                   offset, NULL, 
                                   count, NULL );
   
/* -------------------------------------------------------------------- */
/*      Create memory space to receive the data                         */
/* -------------------------------------------------------------------- */
    col_dims[0]=nBlockYSize;
    col_dims[1]=nBlockXSize;
    memspace = H5Screate_simple( rank, col_dims, NULL );
    H5OFFSET_TYPE mem_offset[3] = {0, 0, 0};
    status =  H5Sselect_hyperslab(memspace,
                                  H5S_SELECT_SET,
                                  mem_offset, NULL,
                                  count, NULL);

    status = H5Dread ( hDatasetID,
                       native,
                       memspace,
                       dataspace,
                       H5P_DEFAULT, 
                       pImage );

    return CE_None;
}

/************************************************************************/
/* ==================================================================== */
/*                              BAGDataset                              */
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                             BAGDataset()                             */
/************************************************************************/

BAGDataset::BAGDataset()
{
    hHDF5 = -1;
    pszXMLMetadata = NULL;
    pszProjection = NULL;

    adfGeoTransform[0] = 0.0;
    adfGeoTransform[1] = 1.0;
    adfGeoTransform[2] = 0.0;
    adfGeoTransform[3] = 0.0;
    adfGeoTransform[4] = 0.0;
    adfGeoTransform[5] = 1.0;
}

/************************************************************************/
/*                            ~BAGDataset()                             */
/************************************************************************/
BAGDataset::~BAGDataset( )
{
    FlushCache();

    if( hHDF5 >= 0 )
        H5Fclose( hHDF5 );

    CPLFree( pszXMLMetadata );
    CPLFree( pszProjection );
}

/************************************************************************/
/*                              Identify()                              */
/************************************************************************/

int BAGDataset::Identify( GDALOpenInfo * poOpenInfo )

{
/* -------------------------------------------------------------------- */
/*      Is it an HDF5 file?                                             */
/* -------------------------------------------------------------------- */
    static const char achSignature[] = "\211HDF\r\n\032\n";

    if( poOpenInfo->pabyHeader == NULL
        || memcmp(poOpenInfo->pabyHeader,achSignature,8) != 0 )
        return FALSE;

/* -------------------------------------------------------------------- */
/*      Does it have the extension .bag?                                */
/* -------------------------------------------------------------------- */
    if( !EQUAL(CPLGetExtension(poOpenInfo->pszFilename),"bag") )
        return FALSE;

    return TRUE;
}

/************************************************************************/
/*                                Open()                                */
/************************************************************************/

GDALDataset *BAGDataset::Open( GDALOpenInfo * poOpenInfo )

{
/* -------------------------------------------------------------------- */
/*      Confirm that this appears to be a BAG file.                     */
/* -------------------------------------------------------------------- */
    if( !Identify( poOpenInfo ) )
        return NULL;

/* -------------------------------------------------------------------- */
/*      Confirm the requested access is supported.                      */
/* -------------------------------------------------------------------- */
    if( poOpenInfo->eAccess == GA_Update )
    {
        CPLError( CE_Failure, CPLE_NotSupported, 
                  "The BAG driver does not support update access." );
        return NULL;
    }
    
/* -------------------------------------------------------------------- */
/*      Open the file as an HDF5 file.                                  */
/* -------------------------------------------------------------------- */
    hid_t hHDF5 = H5Fopen( poOpenInfo->pszFilename, 
                           H5F_ACC_RDONLY, H5P_DEFAULT );

    if( hHDF5 < 0 )  
        return NULL;

/* -------------------------------------------------------------------- */
/*      Confirm it is a BAG dataset by checking for the                 */
/*      BAG_Root/Bag Version attribute.                                 */
/* -------------------------------------------------------------------- */
    hid_t hBagRoot = H5Gopen( hHDF5, "/BAG_root" );
    hid_t hVersion = -1;

    if( hBagRoot >= 0 )
        hVersion = H5Aopen_name( hBagRoot, "Bag Version" );

    if( hVersion < 0 )
    {
        H5Fclose( hHDF5 );
        return NULL;
    }
    H5Aclose( hVersion );

/* -------------------------------------------------------------------- */
/*      Create a corresponding dataset.                                 */
/* -------------------------------------------------------------------- */
    BAGDataset *poDS = new BAGDataset();

    poDS->hHDF5 = hHDF5;

/* -------------------------------------------------------------------- */
/*      Extract version as metadata.                                    */
/* -------------------------------------------------------------------- */
    CPLString osVersion;

    if( GH5_FetchAttribute( hBagRoot, "Bag Version", osVersion ) )
        poDS->SetMetadataItem( "BagVersion", osVersion );

/* -------------------------------------------------------------------- */
/*      Fetch the elevation dataset and attach as a band.               */
/* -------------------------------------------------------------------- */
    int nNextBand = 1;
    hid_t hElevation = H5Dopen( hHDF5, "/BAG_root/elevation" );
    if( hElevation < 0 )
    {
        delete poDS;
        return NULL;
    }

    BAGRasterBand *poElevBand = new BAGRasterBand( poDS, nNextBand );

    if( !poElevBand->Initialize( hElevation, "elevation" ) )
    {
        delete poElevBand;
        delete poDS;
        return NULL;
    }

    poDS->nRasterXSize = poElevBand->nRasterXSize;
    poDS->nRasterYSize = poElevBand->nRasterYSize;

    poDS->SetBand( nNextBand++, poElevBand );

/* -------------------------------------------------------------------- */
/*      Try to do the same for the uncertainty band.                    */
/* -------------------------------------------------------------------- */
    hid_t hUncertainty = H5Dopen( hHDF5, "/BAG_root/uncertainty" );
    BAGRasterBand *poUBand = new BAGRasterBand( poDS, nNextBand );

    if( hUncertainty >= 0 && poUBand->Initialize( hUncertainty, "uncertainty") )
    {
        poDS->SetBand( nNextBand++, poUBand );
    }
    else
        delete poUBand;

/* -------------------------------------------------------------------- */
/*      Try to do the same for the uncertainty band.                    */
/* -------------------------------------------------------------------- */
    hid_t hNominal = -1;

    H5E_BEGIN_TRY {
        hNominal = H5Dopen( hHDF5, "/BAG_root/nominal_elevation" );
    } H5E_END_TRY;

    BAGRasterBand *poNBand = new BAGRasterBand( poDS, nNextBand );
    if( hNominal >= 0 && poNBand->Initialize( hNominal,
                                              "nominal_elevation" ) )
    {
        poDS->SetBand( nNextBand++, poNBand );
    }
    else
        delete poNBand;
        
/* -------------------------------------------------------------------- */
/*      Load the XML metadata.                                          */
/* -------------------------------------------------------------------- */
    poDS->LoadMetadata();

/* -------------------------------------------------------------------- */
/*      Setup/check for pam .aux.xml.                                   */
/* -------------------------------------------------------------------- */
    poDS->SetDescription( poOpenInfo->pszFilename );
    poDS->TryLoadXML();

/* -------------------------------------------------------------------- */
/*      Setup overviews.                                                */
/* -------------------------------------------------------------------- */
    poDS->oOvManager.Initialize( poDS, poOpenInfo->pszFilename );

    return( poDS );
}

/************************************************************************/
/*                            LoadMetadata()                            */
/************************************************************************/

void BAGDataset::LoadMetadata()

{
/* -------------------------------------------------------------------- */
/*      Load the metadata from the file.                                */
/* -------------------------------------------------------------------- */
    hid_t hMDDS = H5Dopen( hHDF5, "/BAG_root/metadata" );
    hid_t datatype     = H5Dget_type( hMDDS );
    hid_t dataspace    = H5Dget_space( hMDDS );
    hid_t native       = H5Tget_native_type( datatype, H5T_DIR_ASCEND );
    hsize_t dims[3], maxdims[3];

    H5Sget_simple_extent_dims( dataspace, dims, maxdims );

    pszXMLMetadata = (char *) CPLCalloc(dims[0]+1,1);

    H5Dread( hMDDS, native, H5S_ALL, dataspace, H5P_DEFAULT, pszXMLMetadata );

    H5Dclose( hMDDS );

    if( strlen(pszXMLMetadata) == 0 )
        return;

/* -------------------------------------------------------------------- */
/*      Try to get the geotransform.                                    */
/* -------------------------------------------------------------------- */
    CPLXMLNode *psRoot = CPLParseXMLString( pszXMLMetadata );

    if( psRoot == NULL )
        return;

    CPLStripXMLNamespace( psRoot, NULL, TRUE );

    CPLXMLNode *psGeo = CPLSearchXMLNode( psRoot, "=MD_Georectified" );

    if( psGeo != NULL )
    {
        char **papszCornerTokens = 
            CSLTokenizeStringComplex( 
                CPLGetXMLValue( psGeo, "cornerPoints.Point.coordinates", "" ),
                " ,", FALSE, FALSE );

        if( CSLCount(papszCornerTokens ) == 4 )
        {
            double dfLLX = atof( papszCornerTokens[0] );
            double dfLLY = atof( papszCornerTokens[1] );
            double dfURX = atof( papszCornerTokens[2] );
            double dfURY = atof( papszCornerTokens[3] );

            adfGeoTransform[0] = dfLLX;
            adfGeoTransform[1] = (dfURX - dfLLX) / (GetRasterXSize()-1);
            adfGeoTransform[3] = dfURY;
            adfGeoTransform[5] = (dfLLY - dfURY) / (GetRasterYSize()-1);

            adfGeoTransform[0] -= adfGeoTransform[1] * 0.5;
            adfGeoTransform[3] -= adfGeoTransform[5] * 0.5;
        }
        CSLDestroy( papszCornerTokens );
    }

    CPLDestroyXMLNode( psRoot );

/* -------------------------------------------------------------------- */
/*      Try to get the coordinate system.                               */
/* -------------------------------------------------------------------- */
    OGRSpatialReference oSRS;

    if( OGR_SRS_ImportFromISO19115( &oSRS, pszXMLMetadata )
        == OGRERR_NONE )
    {
        oSRS.exportToWkt( &pszProjection );
    }
}

/************************************************************************/
/*                          GetGeoTransform()                           */
/************************************************************************/

CPLErr BAGDataset::GetGeoTransform( double *padfGeoTransform )

{
    if( adfGeoTransform[0] != 0.0 || adfGeoTransform[3] != 0.0 )
    {
        memcpy( padfGeoTransform, adfGeoTransform, sizeof(double)*6 );
        return CE_None;
    }
    else
        return GDALPamDataset::GetGeoTransform( padfGeoTransform );
}

/************************************************************************/
/*                          GetProjectionRef()                          */
/************************************************************************/

const char *BAGDataset::GetProjectionRef()

{
    if( pszProjection )
        return pszProjection;
    else 
        return GDALPamDataset::GetProjectionRef();
}

/************************************************************************/
/*                            GetMetadata()                             */
/************************************************************************/

char **BAGDataset::GetMetadata( const char *pszDomain )

{
    if( pszDomain != NULL && EQUAL(pszDomain,"xml:BAG") )
    {
        apszMDList[0] = pszXMLMetadata;
        apszMDList[1] = NULL;

        return apszMDList;
    }
    else
        return GDALPamDataset::GetMetadata( pszDomain );
}

/************************************************************************/
/*                          GDALRegister_BAG()                          */
/************************************************************************/
void GDALRegister_BAG( )

{
    GDALDriver  *poDriver;
    
    if (! GDAL_CHECK_VERSION("BAG"))
        return;

    if(  GDALGetDriverByName( "BAG" ) == NULL )
    {
        poDriver = new GDALDriver();
        
        poDriver->SetDescription( "BAG" );
        poDriver->SetMetadataItem( GDAL_DMD_LONGNAME, 
                                   "Bathymetry Attributed Grid" );
        poDriver->SetMetadataItem( GDAL_DMD_HELPTOPIC, 
                                   "frmt_bag.html" );
        poDriver->pfnOpen = BAGDataset::Open;
        poDriver->pfnIdentify = BAGDataset::Identify;

        GetGDALDriverManager( )->RegisterDriver( poDriver );
    }
}
