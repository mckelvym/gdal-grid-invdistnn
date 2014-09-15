/**********************************************************************************************//**
 * \file  painter.cpp
 *
 * \brief Implements the painter driver.
 **************************************************************************************************/
#include "painter.h"

#include <ogr_spatialref.h>

PainterRasterBand::PainterRasterBand( PainterDataset *poPainterDataset, GUIntBig nIndex )
{
    poDS = poPainterDataset;
    nBand = nIndex;
    nBlockXSize = poPainterDataset->GetPixelGenerator()->GetBlockXSize();
    nBlockYSize = poPainterDataset->GetPixelGenerator()->GetBlockYSize();
    eDataType = poPainterDataset->GetPixelGenerator()->GetDataType();
    nRasterXSize = poPainterDataset->GetPixelGenerator()->GetRasterXSize();
    nRasterYSize = poPainterDataset->GetPixelGenerator()->GetRasterYSize();
}

PainterRasterBand::~PainterRasterBand()
{

}

double PainterRasterBand::GetNoDataValue( int *pbSuccess )
{
    PainterDataset* painterDataset = reinterpret_cast< PainterDataset* >( poDS );
    if ( !painterDataset ) {
        if ( pbSuccess )
            pbSuccess = FALSE;
        return 0.00;
    }
    return painterDataset->GetPixelGenerator()->GetNoDataValue( nBand - 1, pbSuccess );
}

CPLErr PainterRasterBand::IReadBlock( int x, int y, void* pBuffer )
{
    PainterDataset* painterDataset = reinterpret_cast< PainterDataset* >( poDS );
    if ( !painterDataset ) {
        return CE_Failure;
    }
    return painterDataset->GetPixelGenerator()->Generate( nBand - 1, x, y, pBuffer );
}

PainterDataset::PainterDataset() : poPixelGenerator( 0 ), bGeoTransformValid( 0 ), pszProjection( 0 )
{

}

PainterDataset::~PainterDataset()
{
    if ( poPixelGenerator )
        delete poPixelGenerator;
    if ( pszProjection )
      CPLFree( pszProjection );
}

bool PainterDataset::ParseXML( const char *pszFilename )
{
    CPLXMLNode* xmlNode = CPLParseXMLFile( pszFilename );
    if ( !xmlNode )
    {
        return false;
    }

    const char* generatorType = CPLGetXMLValue( xmlNode, "Generator", "SolidFill" );
    poPixelGenerator = CreateGenerator( generatorType );
    if ( !poPixelGenerator ) {
        CPLDestroyXMLNode( xmlNode );
        return false;
    }

    const char* projection = CPLGetXMLValue( xmlNode, "Projection", NULL );
    if ( projection ) {
        if ( EQUALN( projection, "USER:", 5 ) )
        {
            OGRSpatialReference oSR;
            if ( oSR.SetFromUserInput( projection + 5 ) == OGRERR_NONE )
            {
                if ( pszProjection )
                  CPLFree( pszProjection );
                oSR.exportToWkt( &pszProjection );
            }
        }
        else
        {
            if ( pszProjection )
              CPLFree( pszProjection );
            pszProjection = CPLStrdup( projection );
        }
    }

    const char* geoTransform = CPLGetXMLValue( xmlNode, "GeoTransform", NULL );
    if ( geoTransform ) {
        CPLStringList stringList( CSLTokenizeString2( geoTransform, ",", CSLT_STRIPLEADSPACES ) );
        if ( stringList.size() >= 6 ) {
            for ( int i = 0; i < 6; i++ ) {
                adfGeoTransform[i] = CPLAtofM( stringList[i] );
            }
            bGeoTransformValid = TRUE;
        }
    }

    if ( !poPixelGenerator->Initialize( xmlNode ) ) {
        CPLDestroyXMLNode( xmlNode );
        return false;
    }

    this->nRasterXSize = poPixelGenerator->GetRasterXSize();
    this->nRasterYSize = poPixelGenerator->GetRasterYSize();
    this->nBands = poPixelGenerator->GetBandCount();

    for ( GUIntBig i = 0; i < poPixelGenerator->GetBandCount(); i++ ) {
        PainterRasterBand* painterBand = new PainterRasterBand( this, i + 1 );
        if ( !painterBand ) {
            return false;
        }
        this->SetBand( i + 1, painterBand );
    }

    CPLDestroyXMLNode( xmlNode );

    return true;
}

CPLErr PainterDataset::GetGeoTransform( double *padfTransform )
{
    if( bGeoTransformValid )
    {
        memcpy( padfTransform, adfGeoTransform, sizeof(adfGeoTransform[0])*6 );
        return CE_None;
    }

    if( GDALPamDataset::GetGeoTransform( padfTransform ) == CE_None)
        return CE_None;

    return CE_Failure;
}

const char *PainterDataset::GetProjectionRef()
{
    if ( pszProjection )  
        return pszProjection;
    return GDALPamDataset::GetProjectionRef();
}


int PainterDataset::Identify( GDALOpenInfo *poOpenInfo )
{
    if ( poOpenInfo->fp == NULL || poOpenInfo->nHeaderBytes < 20 ) {
        if ( EQUALN( CPLGetExtension( poOpenInfo->pszFilename ), ".painter", 8 ) )
            return 1;
        return 0;
    }
    if ( EQUALN( reinterpret_cast< char* >( poOpenInfo->pabyHeader ), "<Painter>", 9 ) )
        return 1;
    return 0;

}

GDALDataset* PainterDataset::Open( GDALOpenInfo* poOpenInfo )
{
    if ( !PainterDataset::Identify( poOpenInfo ) )
        return 0;
    PainterDataset* painterDataset  = new PainterDataset;
    if ( !painterDataset )
        return 0;
    if ( !painterDataset->ParseXML( poOpenInfo->pszFilename ) ) {
        delete painterDataset;
        return 0;
    }
    if( !painterDataset->bGeoTransformValid )
        painterDataset->bGeoTransformValid =
            GDALReadWorldFile( poOpenInfo->pszFilename, NULL,
                               painterDataset->adfGeoTransform );

    if( !painterDataset->bGeoTransformValid )
        painterDataset->bGeoTransformValid =
            GDALReadWorldFile( poOpenInfo->pszFilename, ".wld",
                               painterDataset->adfGeoTransform );
    painterDataset->SetDescription( poOpenInfo->pszFilename );
    painterDataset->TryLoadXML();
    painterDataset->oOvManager.Initialize( painterDataset, poOpenInfo->pszFilename );
    return painterDataset;
}

void GDALRegister_Painter()
{
    GDALDriver *gdalDriver = 0;
    if ( GDALGetDriverByName( "painter" ) == NULL ) {
        gdalDriver = new GDALDriver();
        gdalDriver->SetDescription( "painter" );
        gdalDriver->SetMetadataItem( GDAL_DMD_LONGNAME, "Painter (.painter)" );
        gdalDriver->SetMetadataItem( GDAL_DMD_EXTENSION, "painter" );
        gdalDriver->pfnOpen = PainterDataset::Open;
        gdalDriver->pfnIdentify = PainterDataset::Identify;
        GetGDALDriverManager()->RegisterDriver( gdalDriver );
    }
}
