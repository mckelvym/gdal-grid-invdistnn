/**********************************************************************************************//**
 * \file  gdalurlproxy.h
 *
 * \brief Declares the gdalurlproxy class.
 **************************************************************************************************/

#if !defined( GDAL_URL_PROXY_DRIVER )
#define GDAL_URL_PROXY_DRIVER

#include "gdal_pam.h"

class GdalUrlProxyDataset : public GDALPamDataset
{
public:
  GdalUrlProxyDataset();
  virtual ~GdalUrlProxyDataset();

  static int Identify( GDALOpenInfo* openInfo );
  static GDALDataset* Open( GDALOpenInfo* openInfo );

protected:
};

#endif
