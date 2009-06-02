#include "pcidsk.h"
#include <cppunit/extensions/HelperMacros.h>

using namespace PCIDSK;

class PCIDSKCreateTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE( PCIDSKCreateTest );
 
    CPPUNIT_TEST( simplePixelInterleaved );

    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();
    void simplePixelInterleaved();
};

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( PCIDSKCreateTest );

void PCIDSKCreateTest::setUp()
{
}

void PCIDSKCreateTest::tearDown()
{
}

void PCIDSKCreateTest::simplePixelInterleaved()
{
    PCIDSKFile *pixel_file;
    int channels[4] = {3, 0, 0, 1};

    pixel_file = PCIDSK::Create( "pixel_file.pix", 300, 200, channels, 
                                 "PIXEL", NULL );

    CPPUNIT_ASSERT( pixel_file != NULL );

    delete pixel_file;
}

