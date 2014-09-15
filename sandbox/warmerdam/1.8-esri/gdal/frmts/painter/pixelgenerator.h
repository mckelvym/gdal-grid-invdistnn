/**********************************************************************************************//**
 * \file  painter.h
 *
 * \brief Painter driver.
 **************************************************************************************************/
#if !defined( GDAL_PAINTER_PIXEL_GENERATOR )
#define GDAL_PAINTER_PIXEL_GENERATOR

#include <stdlib.h>
#include <complex>

#if ( !defined( strtoull ) ) && defined( _WIN32)
#define strtoull _strtoui64
#endif

template < typename T > inline bool RandomFillBlock( void *pBuffer,
                                                     const double &dfLow,
                                                     const double &dfHigh,
                                                     GUIntBig nBlockXSize,
                                                     GUIntBig nBlockYSize,
                                                     GUIntBig nBlockSize,
                                                     boost::random::mt19937& oEngine )
{
    T *pData = reinterpret_cast< T* >( pBuffer );
    boost::random::uniform_int_distribution< T > udist( static_cast< T >( dfLow ),
                                                        static_cast< T >( dfHigh ) );
    for ( GUIntBig j = 0; j < nBlockYSize; j++ ) 
    {
        GUIntBig yOffset = j * nBlockXSize;
        for ( GUIntBig i = 0; i < nBlockXSize; i++ ) 
        {
            pData[yOffset + i] = udist( oEngine );
        }
    }
    return true;
}

template < > inline bool RandomFillBlock< float >( void *pBuffer,
                                                   const double &dfLow,
                                                   const double &dfHigh,
                                                   GUIntBig nBlockXSize,
                                                   GUIntBig nBlockYSize,
                                                   GUIntBig nBlockSize,
                                                   boost::random::mt19937& oEngine )
{
    float *pData = reinterpret_cast< float* >( pBuffer );
    boost::random::uniform_real_distribution< float > udist( static_cast< float >( dfLow ),
                                                             static_cast< float >( dfHigh ) );
    for ( GUIntBig j = 0; j < nBlockYSize; j++ ) 
    {
        GUIntBig yOffset = j * nBlockXSize;
        for ( GUIntBig i = 0; i < nBlockXSize; i++ ) 
        {
            pData[yOffset + i] = udist( oEngine );
        }
    }
    return true;
}

template < > inline bool RandomFillBlock< double >( void *pBuffer,
                                                    const double &dfLow,
                                                    const double &dfHigh,
                                                    GUIntBig nBlockXSize,
                                                    GUIntBig nBlockYSize,
                                                    GUIntBig nBlockSize,
                                                    boost::random::mt19937& oEngine )
{
    double *pData = reinterpret_cast< double* >( pBuffer );
    boost::random::uniform_real_distribution< double > udist( dfLow, dfHigh );
    for ( GUIntBig j = 0; j < nBlockYSize; j++ ) 
    {
        GUIntBig yOffset = j * nBlockXSize;
        for ( GUIntBig i = 0; i < nBlockXSize; i++ ) 
        {
            pData[yOffset + i] = udist( oEngine );
        }
    }
    return true;
}

template < typename T > inline bool FillBlock( void *pBuffer,
                                               const T &tValue,
                                               GUIntBig nBlockXSize,
                                               GUIntBig nBlockYSize,
                                               GUIntBig nBlockSize )
{
    T *pData = reinterpret_cast< T* >( pBuffer );
    if ( tValue == static_cast < T >( 0 ) )
    {
        memset( pBuffer, 0, nBlockSize );
    } 
    else 
    {
        for ( GUIntBig j = 0; j < nBlockYSize; j++ ) 
        {
            GUIntBig yOffset = j * nBlockXSize;
            for ( GUIntBig i = 0; i < nBlockXSize; i++ ) 
            {
                pData[yOffset + i] = tValue;
            }
        }
    }
    return true;
}

template < > inline bool FillBlock< char >( void *pBuffer,
                                            const char &nValue,
                                            GUIntBig nBlockXSize,
                                            GUIntBig nBlockYSize,
                                            GUIntBig nBlockSize )
{
    memset( pBuffer, nValue, nBlockSize );
    return true;
}

template < > inline bool FillBlock< unsigned char >( void *pBuffer,
                                                     const unsigned char &nValue,
                                                     GUIntBig nBlockXSize,
                                                     GUIntBig nBlockYSize,
                                                     GUIntBig nBlockSize )
{
    memset( pBuffer, nValue, nBlockSize );
    return true;
}

template < > inline bool FillBlock< float >( void *pBuffer,
                                             const float &fValue,
                                             GUIntBig nBlockXSize,
                                             GUIntBig nBlockYSize,
                                             GUIntBig nBlockSize )
{
    float *pfData = reinterpret_cast< float* >( pBuffer );
    if ( ARE_REAL_EQUAL( fValue, 0.00 ) )
    {
        memset( pBuffer, 0, nBlockSize );
    } 
    else 
    {
        for ( GUIntBig j = 0; j < nBlockYSize; j++ ) 
        {
            GUIntBig yOffset = j * nBlockXSize;
            for ( GUIntBig i = 0; i < nBlockXSize; i++ ) 
            {
                pfData[yOffset + i] = fValue;
            }
        }
    }
    return true;
}

template < > inline bool FillBlock< double >( void *pBuffer,
                                              const double &dfValue,
                                              GUIntBig nBlockXSize,
                                              GUIntBig nBlockYSize,
                                              GUIntBig nBlockSize )
{
    float *pdfData = reinterpret_cast< float* >( pBuffer );
    if ( ARE_REAL_EQUAL( dfValue, 0.00 ) )
    {
        memset( pBuffer, 0, nBlockSize );
    } 
    else 
    {
        for ( GUIntBig j = 0; j < nBlockYSize; j++ ) 
        {
            GUIntBig yOffset = j * nBlockXSize;
            for ( GUIntBig i = 0; i < nBlockXSize; i++ ) 
            {
                pdfData[yOffset + i] = dfValue;
            }
        }
    }
    return true;
}


class PixelGenerator
{
public:
    PixelGenerator();
    virtual ~PixelGenerator();

    virtual bool Initialize( CPLXMLNode* poXmlNode );

    virtual double GetNoDataValue( GUIntBig nBand, int *pbSuccess = NULL );

    virtual bool AllocateBuffers( GUIntBig nCount );
    virtual bool FreeBuffers();
    virtual bool FillBuffers( std::vector< double > adfValues );
    virtual CPLErr CopyBuffer( int    nBand,
                               void*  pBuffer );

    virtual CPLErr Generate( int    nBand,
                             int    nX,
                             int    nY,
                             void*  pBuffer );

    inline GUIntBig GetRasterXSize() const { return nRasterXSize; }
    inline GUIntBig GetRasterYSize() const { return nRasterYSize; }
    inline GUIntBig GetBandCount() const { return nBands; }
    inline GUIntBig GetBlockXSize() { return nBlockXSize; }
    inline GUIntBig GetBlockYSize() { return nBlockYSize; }
    inline GDALDataType GetDataType() const { return nDataType; }

protected:
    GUIntBig nRasterXSize;
    GUIntBig nRasterYSize;
    GUIntBig nBands;
    GUIntBig nBlockXSize;
    GUIntBig nBlockYSize;
    GDALDataType nDataType;
    std::vector< double > adfNoDataValues;
    std::vector< void* > apBlockBuffers;
    std::vector< void* > apBandBuffers;
    GUIntBig nBlockSize;
};

class SolidFillGenerator : public PixelGenerator
{
public:
    SolidFillGenerator();
    virtual ~SolidFillGenerator();

    virtual bool Initialize( CPLXMLNode* poXmlNode );

protected:
    std::vector< double > adfValues;
};

class RandomFillGenerator : public PixelGenerator
{
public:
    RandomFillGenerator();
    virtual ~RandomFillGenerator();

    virtual bool Initialize( CPLXMLNode *poXmlNode );

    virtual CPLErr Generate( int    nBand,
                             int    nX,
                             int    nY,
                             void*  pBuffer );
    
protected:
    double dfLow;
    double dfHigh;
    boost::random::mt19937 oEngine;
};

class GridGenerator : public PixelGenerator
{
public:
    GridGenerator();
    virtual ~GridGenerator();
    
    virtual bool Initialize( CPLXMLNode *poXmlNode );
    
    virtual CPLErr Generate( int    nBand,
                             int    nX,
                             int    nY,
                             void*  pBuffer );
    
protected:
    std::vector< double > adfGridValues;
    std::vector< double > adfFillValues;
};

class JuliaGenerator : public PixelGenerator
{
public:
    JuliaGenerator();
    virtual ~JuliaGenerator();
    
    virtual bool Initialize( CPLXMLNode *poXmlNode );
    
    virtual CPLErr Generate( int    nBand,
                             int    nX,
                             int    nY,
                             void*  pBuffer );
    
protected:
    double dfUlx, dfUly, dfLrx, dfLry;
    std::complex< double > oCenter;
    unsigned int nIterations;
    
    unsigned int IteratePoint( const std::complex< double >& oPoint );
};

PixelGenerator* CreateGenerator( const char *pszName );

#endif
