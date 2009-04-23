#include "pcidsk.h"
#include <cppunit/extensions/HelperMacros.h>

using namespace PCIDSK;

class PCIDSKFileTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE( PCIDSKFileTest );
 
    CPPUNIT_TEST( testOpenEltoro );
    CPPUNIT_TEST( testReadImage );
    CPPUNIT_TEST( testReadPixelInterleavedImage );

    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();
    void testOpenEltoro();
    void testReadImage();
    void testReadPixelInterleavedImage();
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
    CPPUNIT_ASSERT( strcmp(eltoro->GetInterleaving(),"BAND") == 0 );

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
    CPPUNIT_ASSERT( chan1->GetType() == PCIDSK::CHN_8U );
    
    delete eltoro;

}

void PCIDSKFileTest::testReadPixelInterleavedImage()
{
    PCIDSKFile *irvine;
    PCIDSKChannel *channel;
    uint8 data_line[512*4];

    irvine = PCIDSK::Open( "irvine.pix", "r", NULL );

    CPPUNIT_ASSERT( irvine != NULL );
    CPPUNIT_ASSERT( strcmp(irvine->GetInterleaving(),"PIXEL") == 0 );

    channel = irvine->GetChannel(2);
    channel->ReadBlock( 511, data_line );

    CPPUNIT_ASSERT( data_line[511] == 22 );
    
    channel = irvine->GetChannel(10);
    channel->ReadBlock( 511, data_line );

    CPPUNIT_ASSERT( ((short *) data_line)[511] == 304 );
    CPPUNIT_ASSERT( channel->GetType() == PCIDSK::CHN_16S );
    
    delete irvine;
}

