#include "pcidsk.h"
#include <cppunit/extensions/HelperMacros.h>
#include <cstring>

using PCIDSK::PCIDSKException;

class PCIDSKExceptionTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE( PCIDSKExceptionTest );
 
    CPPUNIT_TEST( testFormatting );

    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

    void testFormatting();
};

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( PCIDSKExceptionTest );

void PCIDSKExceptionTest::setUp()
{
}


void PCIDSKExceptionTest::tearDown()
{
}

void PCIDSKExceptionTest::testFormatting()
{
    PCIDSKException ex( "Illegal Value:%s", "value" );

    CPPUNIT_ASSERT( strcmp(ex.what(),"Illegal Value:value") == 0 );
}
