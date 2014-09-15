/**********************************************************************************************//**
 * \file  painter.h
 *
 * \brief Painter driver.
 **************************************************************************************************/
#if !defined( GDAL_PAINTER_DRIVER )
#define GDAL_PAINTER_DRIVER

#include "gdal_pam.h"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/uniform_real_distribution.hpp>
#include "pixelgenerator.h"

class PainterDataset;

class PainterRasterBand : public GDALPamRasterBand
{
public:
    PainterRasterBand( PainterDataset* poPainterDataset, GUIntBig nIndex );
    virtual ~PainterRasterBand();

    virtual double GetNoDataValue( int *pbSuccess = NULL );

protected:
    virtual CPLErr IReadBlock( int    x,
                               int    y,
                               void*  pBuffer );
};

class PainterDataset : public GDALPamDataset
{
public:
    PainterDataset();
    virtual ~PainterDataset();

    static int          Identify( GDALOpenInfo *poOpenInfo );
    static GDALDataset *Open( GDALOpenInfo *poOpenInfo );

    CPLErr              GetGeoTransform( double *padfTransform );
    virtual const char *GetProjectionRef();

    inline PixelGenerator*     GetPixelGenerator() const { return poPixelGenerator; }
    bool                ParseXML( const char *pszFilename );

protected:
    PixelGenerator      *poPixelGenerator;
    double              adfGeoTransform[6];
    int                 bGeoTransformValid;
    char               *pszProjection;
};

#endif
