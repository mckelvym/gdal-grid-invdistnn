#include "pcidsk.h"
#include <cppunit/extensions/HelperMacros.h>

using namespace PCIDSK;

class PCIDSKCreateTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE( PCIDSKCreateTest );
 
    CPPUNIT_TEST( simplePixelInterleaved );
    CPPUNIT_TEST( testErrors );

    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();
    void simplePixelInterleaved();
    void testErrors();
};

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( PCIDSKCreateTest );

void PCIDSKCreateTest::setUp()
{
}

void PCIDSKCreateTest::tearDown()
{
}

/************************************************************************/
/*                       simplePixelInterleaved()                       */
/************************************************************************/
void PCIDSKCreateTest::simplePixelInterleaved()
{
    PCIDSKFile *pixel_file;
    eChanType channel_types[4] = {CHN_8U, CHN_8U, CHN_8U, CHN_32R};

    pixel_file = PCIDSK::Create( "pixel_file.pix", 300, 200, 4, channel_types,
                                 "PIXEL", NULL );

    CPPUNIT_ASSERT( pixel_file != NULL );
    CPPUNIT_ASSERT( pixel_file->GetUpdatable() );

    PCIDSKGeoref *geo = dynamic_cast<PCIDSKGeoref*>(pixel_file->GetSegment(1));

    CPPUNIT_ASSERT( geo != NULL );
    CPPUNIT_ASSERT( geo->GetGeosys() == "PIXEL" );
    
    double a1, a2, a3, b1, b2, b3;
    geo->GetTransform( a1, a2, a3, b1, b2, b3 );

    CPPUNIT_ASSERT( a1 == 0.0 );
    CPPUNIT_ASSERT( a2 == 1.0 );
    CPPUNIT_ASSERT( a3 == 0.0 );
    CPPUNIT_ASSERT( b1 == 0.0 );
    CPPUNIT_ASSERT( b2 == 0.0 );
    CPPUNIT_ASSERT( b3 == 1.0 );

    delete pixel_file;

    unlink( "pixel_file.pix" );
}

/************************************************************************/
/*                             testErrors()                             */
/************************************************************************/

void PCIDSKCreateTest::testErrors()
{
    PCIDSKFile *pixel_file;

/* -------------------------------------------------------------------- */
/*      Illegal order for mixture of pixel types.                       */
/* -------------------------------------------------------------------- */
    try
    {
        eChanType channel_types[4] = {CHN_8U, CHN_8U, CHN_32R,CHN_8U};

        pixel_file = PCIDSK::Create( "pixel_file.pix", 300, 200, 
                                     4, channel_types, "PIXEL", NULL );
        CPPUNIT_ASSERT( false );
    }
    catch( PCIDSKException &ex )
    {
        CPPUNIT_ASSERT( strstr(ex.what(),"mixture") != NULL );
    }

/* -------------------------------------------------------------------- */
/*      Illegal options.                                                */
/* -------------------------------------------------------------------- */
    try
    {
        pixel_file = PCIDSK::Create( "pixel_file.pix", 300, 200, 
                                     4, NULL, "SOMETILES", NULL );
        CPPUNIT_ASSERT( false );
    }
    catch( PCIDSKException &ex )
    {
        CPPUNIT_ASSERT( strstr(ex.what(),"options") != NULL );
    }
}

