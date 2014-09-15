/**********************************************************************************************//**
 * \file  pixelgenerator.cpp
 *
 * \brief Implements classes for generating pixels class.
 **************************************************************************************************/
#include "painter.h"

PixelGenerator::PixelGenerator() : nRasterXSize( 0 ),
    nRasterYSize( 0 ),
    nBands( 0 ),
    nBlockXSize( 0 ),
    nBlockYSize( 0 ),
    nDataType( GDT_Unknown ),
    adfNoDataValues(),
    apBlockBuffers(),
    apBandBuffers(),
    nBlockSize( 0 )
{

}

PixelGenerator::~PixelGenerator()
{
    FreeBuffers();
}

bool PixelGenerator::Initialize( CPLXMLNode *poXmlNode )
{
    nRasterXSize = strtoull( CPLGetXMLValue( poXmlNode, "RasterXSize", "0" ), 0, 10 );
    nRasterYSize = strtoull( CPLGetXMLValue( poXmlNode, "RasterYSize", "0" ), 0, 10 );
    nBands = strtoull( CPLGetXMLValue( poXmlNode, "BandCount", "0" ), 0, 10 );
    nDataType = GDALGetDataTypeByName( CPLGetXMLValue( poXmlNode, "DataType", "Unknown" ) );
    if ( ( !nRasterXSize ) ||
         ( !nRasterYSize ) ||
         ( !nBands ) ||
         ( nDataType == GDT_Unknown ) )
        return false;
    return true;
}

double PixelGenerator::GetNoDataValue( GUIntBig nBand, int* pbSuccess /*= NULL */ )
{
    if ( ( adfNoDataValues.empty() ) ||
            ( nBand >= nBands ) ) {
        if ( pbSuccess )
            *pbSuccess = FALSE;
        return 0.00;
    }
    if ( ( adfNoDataValues.size() == 1 ) ||
            ( nBand >= adfNoDataValues.size() ) ) {
        if ( pbSuccess )
            *pbSuccess = TRUE;
        return adfNoDataValues[0];
    }
    if ( pbSuccess )
        *pbSuccess = TRUE;
    return adfNoDataValues[nBand];
}

bool PixelGenerator::AllocateBuffers( GUIntBig nCount )
{
    if ( ( !nCount ) ||
         ( !nRasterXSize ) ||
         ( !nRasterYSize ) ||
         ( !nBands ) ||
         ( !nBlockXSize ) ||
         ( !nBlockYSize ) ||
         ( nDataType == GDT_Unknown ) ||
         ( ( nCount != 1 ) && ( nCount != nBands ) ) )
        return false;
    nBlockSize = ( nBlockXSize * nBlockYSize * GDALGetDataTypeSize( nDataType ) ) / 8;
    if ( !nBlockSize )
        return false;
    for ( GUIntBig i = 0; i < nCount; i++ ) {
        void *pBuffer = VSIMalloc( nBlockSize );
        if ( !pBuffer )
            return false;
        apBlockBuffers.push_back( pBuffer );
    }
    if ( nCount == 1 ) {
        for ( GUIntBig i = 0; i < nBands; i++ )
            apBandBuffers.push_back( apBlockBuffers[0] );
    } else {
        for ( GUIntBig i = 0; i < nBands; i++ )
            apBandBuffers.push_back( apBlockBuffers[i] );
    }
    return true;
}

bool PixelGenerator::FreeBuffers()
{
    if ( !apBandBuffers.empty() )
        apBandBuffers.clear();
    if ( !apBlockBuffers.empty() ) {
        for ( GUIntBig i = 0; i < apBlockBuffers.size(); i++ ) {
            void* pBuffer = apBlockBuffers[i];
            VSIFree( pBuffer );
        }
        apBlockBuffers.clear();
    }
    return true;
}

bool PixelGenerator::FillBuffers( std::vector< double > adfValues )
{
    GUIntBig count = std::min( adfValues.size(), apBlockBuffers.size() );
    for ( GUIntBig i = 0; i < count; i++ ) {
        switch ( nDataType )
        {
        case GDT_Byte:
            if ( !FillBlock< unsigned char >( apBlockBuffers[i], static_cast< unsigned char >( adfValues[i] ), nBlockXSize, nBlockYSize, nBlockSize ) )
                return false;
            break;
        case GDT_UInt16:
            if ( !FillBlock< unsigned short int >( apBlockBuffers[i], static_cast< unsigned short int >( adfValues[i] ), nBlockXSize, nBlockYSize, nBlockSize ) )
                return false;
            break;
        case GDT_Int16:
            if ( !FillBlock< short int >( apBlockBuffers[i], static_cast< short int >( adfValues[i] ), nBlockXSize, nBlockYSize, nBlockSize ) )
                return false;
            break;
        case GDT_UInt32:
            if ( !FillBlock< unsigned long int >( apBlockBuffers[i], static_cast< unsigned long int >( adfValues[i] ), nBlockXSize, nBlockYSize, nBlockSize ) )
                return false;
            break;
        case GDT_Int32:
            if ( !FillBlock< long int >( apBlockBuffers[i], static_cast< long int >( adfValues[i] ), nBlockXSize, nBlockYSize, nBlockSize ) )
                return false;
            break;
        case GDT_Float32:
            if ( !FillBlock< double >( apBlockBuffers[i], static_cast< double >( adfValues[i] ), nBlockXSize, nBlockYSize, nBlockSize ) )
                return false;
            break;
        case GDT_Float64:
            if ( !FillBlock< double >( apBlockBuffers[i], static_cast< double >( adfValues[i] ), nBlockXSize, nBlockYSize, nBlockSize ) )
                return false;
            break;
        case GDT_CInt16:
        case GDT_CInt32:
        case GDT_CFloat32:
        case GDT_CFloat64:
        case GDT_Unknown:
        default:
            return false;
            break;
        }
    }
    return true;
}

CPLErr PixelGenerator::CopyBuffer( int nBand, void* pBuffer )
{
    if ( ( nBand >= nBands ) ||
            ( !apBandBuffers[nBand] ) ) {
        return CE_Failure;
    }
    memcpy( pBuffer, apBandBuffers[nBand], nBlockSize );
    return CE_None;
}

CPLErr PixelGenerator::Generate( int nBand, int nX, int nY, void* pBuffer )
{
    return CopyBuffer( nBand, pBuffer );
}

SolidFillGenerator::SolidFillGenerator() : PixelGenerator(),
                                           adfValues()
{

}

SolidFillGenerator::~SolidFillGenerator()
{

}

bool SolidFillGenerator::Initialize( CPLXMLNode *poXmlNode )
{
    if ( !PixelGenerator::Initialize( poXmlNode ) ) {
        return false;
    }
    GUIntBig nValueCount = 1;
    const char* pixelValues = CPLGetXMLValue( poXmlNode, "Values", "0" );
    CPLStringList stringList( CSLTokenizeString2( pixelValues, ",", CSLT_STRIPLEADSPACES ) );
    if ( !stringList.size() ) 
    {
        nValueCount = 1;
        adfValues.resize( nBands, 0.00 );
    } 
    else if (stringList.size() == 1 ) 
    {
        nValueCount = 1;
        adfValues.resize( nBands, CPLAtofM( stringList[0] ) );
    } 
    else 
    {
        adfValues.resize( nBands, CPLAtofM( stringList[0] ) );
        for ( int i = 0; i < stringList.size(); i++ )
            adfValues[i] = CPLAtofM( stringList[i] );
        nValueCount = nBands;
    }
    nBlockXSize = std::min< GUIntBig >( 512, GetRasterXSize() );
    nBlockYSize = std::min< GUIntBig >( 512, GetRasterYSize() );
    if ( !PixelGenerator::AllocateBuffers( nValueCount ) ) 
    {
        return false;
    }
    if ( !PixelGenerator::FillBuffers( adfValues ) ) 
    {
        return false;
    }
    return true;
}

RandomFillGenerator::RandomFillGenerator() : PixelGenerator(),
                                             dfLow( 0.00 ),
                                             dfHigh( 255.00 ),
                                             oEngine()
{

}

RandomFillGenerator::~RandomFillGenerator()
{

}

bool RandomFillGenerator::Initialize( CPLXMLNode *poXmlNode )
{
    if ( !PixelGenerator::Initialize( poXmlNode ) ) {
        return false;
    }
    dfLow = CPLAtofM( CPLGetXMLValue( poXmlNode, "Low", "0.00" ) );
    dfHigh = CPLAtofM( CPLGetXMLValue( poXmlNode, "High", "255.00" ) );
    nBlockXSize = std::min< GUIntBig >( 512, GetRasterXSize() );
    nBlockYSize = std::min< GUIntBig >( 512, GetRasterYSize() );
    oEngine.seed( time( NULL ) );
    return true;
}

CPLErr RandomFillGenerator::Generate( int    nBand,
                                      int    nX,
                                      int    nY,
                                      void*  pBuffer )
{
    switch ( nDataType )
    {
    case GDT_Byte:
        if ( RandomFillBlock< unsigned char >( pBuffer,
                                               dfLow,
                                               dfHigh,
                                               nBlockXSize, nBlockYSize, nBlockSize,
                                               oEngine ) )
            return CE_None;
        break;
    case GDT_UInt16:
        if ( RandomFillBlock< unsigned short int >( pBuffer,
                                                    dfLow,
                                                    dfHigh,
                                                    nBlockXSize, nBlockYSize, nBlockSize,
                                                    oEngine ) )
            return CE_None;
        break;
    case GDT_Int16:
        if ( RandomFillBlock< signed short int >( pBuffer,
                                                  dfLow,
                                                  dfHigh,
                                                  nBlockXSize, nBlockYSize, nBlockSize,
                                                  oEngine ) )
            return CE_None;
        break;
    case GDT_UInt32:
        if ( RandomFillBlock< unsigned long int >( pBuffer,
                                                   dfLow,
                                                   dfHigh,
                                                   nBlockXSize, nBlockYSize, nBlockSize,
                                                   oEngine ) )
            return CE_None;
        break;
    case GDT_Int32:
        if ( RandomFillBlock< signed long int >( pBuffer,
                                                 dfLow,
                                                 dfHigh,
                                                 nBlockXSize, nBlockYSize, nBlockSize,
                                                 oEngine ) )
            return CE_None;
        break;
    case GDT_Float32:
        if ( RandomFillBlock< double >( pBuffer,
                                       dfLow,
                                       dfHigh,
                                       nBlockXSize, nBlockYSize, nBlockSize,
                                       oEngine ) )
            return CE_None;
        break;
    case GDT_Float64:
        if ( RandomFillBlock< double >( pBuffer,
                                        dfLow,
                                        dfHigh,
                                        nBlockXSize, nBlockYSize, nBlockSize,
                                        oEngine ) )
            return CE_None;
        break;
    case GDT_CInt16:
    case GDT_CInt32:
    case GDT_CFloat32:
    case GDT_CFloat64:
    case GDT_Unknown:
    default:
        return CE_Failure;
        break;
    }
    return CE_Failure;
}

GridGenerator::GridGenerator() : PixelGenerator(),
                                 adfGridValues(),
                                 adfFillValues()
{

}

GridGenerator::~GridGenerator()
{

}

bool GridGenerator::Initialize( CPLXMLNode *poXmlNode )
{
    if ( !PixelGenerator::Initialize( poXmlNode ) ) {
        return false;
    }
    GUIntBig nValueCount = 1;
    const char* pixelValues = CPLGetXMLValue( poXmlNode, "Values", "0" );
    CPLStringList stringList( CSLTokenizeString2( pixelValues, ",", CSLT_STRIPLEADSPACES ) );
    if ( !stringList.size() ) {
        nValueCount = 1;
        adfGridValues.resize( nBands, 0.00 );
    } 
    else if (stringList.size() == 1 ) 
    {
        nValueCount = 1;
        adfGridValues.resize( nBands, CPLAtofM( stringList[0] ) );
    } 
    else 
    {
        adfGridValues.resize( nBands, CPLAtofM( stringList[0] ) );
        for ( int i = 0; i < stringList.size(); i++ )
            adfGridValues[i] = CPLAtofM( stringList[i] );
        nValueCount = nBands;
    }
    nValueCount = 1;
    pixelValues = CPLGetXMLValue( poXmlNode, "BackgroundValues", "255" );
    stringList.Assign( CSLTokenizeString2( pixelValues, ",", CSLT_STRIPLEADSPACES ) );
    if ( !stringList.size() ) {
        nValueCount = 1;
        adfFillValues.resize( nBands, 0.00 );
    } 
    else if (stringList.size() == 1 )
    {
        nValueCount = 1;
        adfFillValues.resize( nBands, CPLAtofM( stringList[0] ) );
    } 
    else 
    {
        adfFillValues.resize( nBands, CPLAtofM( stringList[0] ) );
        for ( int i = 0; i < stringList.size(); i++ )
            adfFillValues[i] = CPLAtofM( stringList[i] );
        nValueCount = nBands;
    }
    nBlockXSize = std::min< GUIntBig >( 512, GetRasterXSize() );
    nBlockYSize = std::min< GUIntBig >( 512, GetRasterYSize() );
    if ( !PixelGenerator::AllocateBuffers( nBands ) ) {
        return false;
    }
    if ( !PixelGenerator::FillBuffers( adfGridValues ) ) {
        return false;
    }
    return true;
}

CPLErr GridGenerator::Generate( int    nBand,
                                int    nX,
                                int    nY,
                                void*  pBuffer )
{
    return CE_Failure;
}

JuliaGenerator::JuliaGenerator() : PixelGenerator(),
                                   dfUlx( -1.50 ), 
                                   dfUly( -1.50 ), 
                                   dfLrx( 1.50 ), 
                                   dfLry( 1.50 ),
                                   oCenter( -0.4, 0.6 ),
                                   //oCenter( 0.360284, 0.100376 ),
                                   nIterations( 255 )
{

}

JuliaGenerator::~JuliaGenerator()
{

}

bool JuliaGenerator::Initialize( CPLXMLNode *poXmlNode )
{
    if ( !PixelGenerator::Initialize( poXmlNode ) ) {
        return false;
    }
    nBlockXSize = std::min< GUIntBig >( 512, GetRasterXSize() );
    nBlockYSize = std::min< GUIntBig >( 512, GetRasterYSize() );
    return true;
}

CPLErr JuliaGenerator::Generate( int    nBand,
                                 int    nX,
                                 int    nY,
                                 void*  pBuffer )
{
    double fDx = ( ( dfLrx - dfUlx ) / static_cast< double >( GetRasterXSize() ) );
    double fDy = ( ( dfLry - dfUly ) / static_cast< double >( GetRasterYSize() ) );
    unsigned char *pData = reinterpret_cast< unsigned char* >( pBuffer );
   
    for ( GUIntBig j = 0; j < nBlockYSize; j++ ) 
    {
        double py = dfUly + fDy * static_cast< double >( ( nY * nBlockYSize ) + j );
        GUIntBig yOffset = j * nBlockXSize;
        for ( GUIntBig i = 0; i < nBlockXSize; i++ ) 
        {
            double px = dfUlx + fDx * static_cast< double >( ( nX * nBlockXSize ) + i );
            unsigned int pixelColor = IteratePoint( std::complex< double >( px, py ) );
            if (pixelColor > 100)
            {
                int ff = 0;
                ff++;
            }
            if (!nBand)
                pixelColor = (int) ((cos(pixelColor / 10.0f) + 1.0f) * 128.0f);
            if (nBand == 1)
                pixelColor = (int) ((cos(pixelColor / 20.0f) + 1.0f) * 128.0f);
            if ( nBand == 2 )
                pixelColor = (int) ((cos(pixelColor / 300.0f) + 1.0f) * 128.0f);
            pixelColor = std::min( pixelColor, static_cast< unsigned int >( 255 ) );
            pData[yOffset + i] = pixelColor;
        }
    }
    return CE_None;
}

unsigned int JuliaGenerator::IteratePoint( const std::complex< double > &oPoint )
{
    unsigned int nCounter = 0;
    std::complex< double > z( oPoint );
    
    while( ( ( ( z.real() * z.real() ) + ( z.imag() * z.imag() ) ) < 4.0 ) && ( nCounter < nIterations ) ) 
    {
        z = ( z * z ) + oCenter;
        nCounter++;
    }
    return nCounter;    
}

PixelGenerator* CreateGenerator( const char *pszName )
{
    if ( EQUALN( pszName, "SolidFill", 9 ) ) {
        return new SolidFillGenerator();
    } else if ( EQUALN( pszName, "RandomFill", 10 ) ) {
        return new RandomFillGenerator();
    } else if ( EQUALN( pszName, "Julia", 5 ) ) {
        return new JuliaGenerator();
    }
    return 0;
}
