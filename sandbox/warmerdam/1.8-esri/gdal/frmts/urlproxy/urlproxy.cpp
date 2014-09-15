/**********************************************************************************************//**
 * \file  gdalurlproxy.cpp
 *
 * \brief Implements the gdalurlproxy class.
 **************************************************************************************************/

#include "urlproxy.h"

GdalUrlProxyDataset::GdalUrlProxyDataset()
{

}

GdalUrlProxyDataset::~GdalUrlProxyDataset()
{
}

int GdalUrlProxyDataset::Identify( GDALOpenInfo* openInfo )
{
  if ( openInfo->fp == NULL || openInfo->nHeaderBytes < 20 ) {
    if ( EQUALN( CPLGetExtension( openInfo->pszFilename ), "gurl", 4 ) )
      return 1;
    return 0;
  }
  if ( EQUALN( reinterpret_cast< char* >( openInfo->pabyHeader ), "<GDAL_URL>", 9 ) )
    return 1;
  return 0;

}

GDALDataset* GdalUrlProxyDataset::Open( GDALOpenInfo* openInfo )
{
  if ( !GdalUrlProxyDataset::Identify( openInfo ) )
    return 0;
  CPLXMLNode* xmlNode = CPLParseXMLFile( openInfo->pszFilename );
  if ( !xmlNode )
    return 0;
  CPLString url = CPLGetXMLValue( xmlNode, "URL", NULL );
  if ( !url ) {
    return 0;
  }
  CPLString driverName = CPLGetXMLValue( xmlNode, "DRIVER", NULL );
  CPLDestroyXMLNode( xmlNode );
  if ( driverName ) {
    GDALDriver* gdalDriver = reinterpret_cast< GDALDriver* >( GDALGetDriverByName( driverName.c_str() ) );
    if ( !gdalDriver ) {
      return 0;
    }
    GDALOpenInfo info( url.c_str(), openInfo->eAccess );
    GDALDataset* dataset = reinterpret_cast< GDALDataset* >( (*gdalDriver->pfnOpen)( &info ) );
    if ( !dataset ) {
      return 0;
    }
    return dataset;
  } else {
    GDALDataset* dataset = reinterpret_cast< GDALDataset* >( GDALOpen( url, openInfo->eAccess ) );
    if ( !dataset ) {
      return 0;
    }
    return dataset;
  }
  return 0;
}

void GDALRegister_urlproxy()
{
  GDALDriver *gdalDriver = 0;
  if ( GDALGetDriverByName( "urlproxy" ) == NULL ) {
    gdalDriver = new GDALDriver();
    gdalDriver->SetDescription( "urlproxy" );
    gdalDriver->SetMetadataItem( GDAL_DMD_LONGNAME, "URL Proxy XML (.urlproxy)" );
    gdalDriver->SetMetadataItem( GDAL_DMD_EXTENSION, "gurl" );
    gdalDriver->pfnOpen = GdalUrlProxyDataset::Open;
    gdalDriver->pfnIdentify = GdalUrlProxyDataset::Identify;
    GetGDALDriverManager()->RegisterDriver( gdalDriver );
  }
}
