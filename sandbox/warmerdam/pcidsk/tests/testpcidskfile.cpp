#include "pcidsk.h"
#include <cppunit/extensions/HelperMacros.h>

using namespace PCIDSK;

class PCIDSKFileTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE( PCIDSKFileTest );
 
    CPPUNIT_TEST( testOpenEltoro );
    CPPUNIT_TEST( testReadImage );

    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();
    void testOpenEltoro();
    void testReadImage();
};

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( PCIDSKFileTest );

void PCIDSKFileTest::setUp()
{
}

void PCIDSKFileTest::tearDown()
{
}

void PCIDSKFileTest::testOpenEltoro()
{
    PCIDSKFile *eltoro;

    eltoro = PCIDSK::Open( "eltoro.pix", "r", NULL );

    CPPUNIT_ASSERT( eltoro != NULL );

    CPPUNIT_ASSERT( eltoro->GetWidth() == 1024 );
    CPPUNIT_ASSERT( eltoro->GetHeight() == 1024 );
    CPPUNIT_ASSERT( eltoro->GetChannels() == 1 );

    delete eltoro;
}

void PCIDSKFileTest::testReadImage()
{
    PCIDSKFile *eltoro;
    PCIDSKChannel *chan1;
    uint8 data_line[1024];

    eltoro = PCIDSK::Open( "eltoro.pix", "r", NULL );

    CPPUNIT_ASSERT( eltoro != NULL );

    chan1 = eltoro->GetChannel(1);

    chan1->ReadBlock( 3, data_line );

    CPPUNIT_ASSERT( data_line[2] == 38 );
    
    delete eltoro;

}

