#include "pcidsk.h"
#include <cppunit/extensions/HelperMacros.h>

using namespace PCIDSK;

class PCIDSKFileTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE( PCIDSKFileTest );
 
    CPPUNIT_TEST( testOpenEltoro );
    CPPUNIT_TEST( testReadImage );
    CPPUNIT_TEST( testReadPixelInterleavedImage );
    CPPUNIT_TEST( testReadTiledImage );

    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();
    void testOpenEltoro();
    void testReadImage();
    void testReadPixelInterleavedImage();
    void testReadTiledImage();
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

    // Test subwindowing.
    data_line[5] = 255;
    chan1->ReadBlock( 3, data_line, 2, 0, 5, 0 );
    CPPUNIT_ASSERT( data_line[0] == 38 );
    CPPUNIT_ASSERT( data_line[5] == 255 );
    
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

    // test windowed access.    
    channel->ReadBlock( 511, data_line, 510, 0, 2, 1 );
    CPPUNIT_ASSERT( ((short *) data_line)[1] == 304 );

    // Test the pixel interleaved reads on the file itself.
    CPPUNIT_ASSERT(irvine->GetPixelGroupSize() == 13);
    CPPUNIT_ASSERT(irvine->GetBlockSize() == 6656);
    
    uint8 *interleaved_line = (uint8 *) irvine->ReadAndLockBlock( 254 );
    CPPUNIT_ASSERT(interleaved_line[13*7+0] == 66 );
    CPPUNIT_ASSERT(interleaved_line[13*7+1] == 25 );
    CPPUNIT_ASSERT(interleaved_line[13*7+2] == 28 );
    irvine->UnlockBlock();
    
    interleaved_line = (uint8 *) irvine->ReadAndLockBlock( 254, 7, 5 );
    CPPUNIT_ASSERT(interleaved_line[0] == 66 );
    CPPUNIT_ASSERT(interleaved_line[1] == 25 );
    CPPUNIT_ASSERT(interleaved_line[2] == 28 );
    irvine->UnlockBlock();
    
    delete irvine;
}

void PCIDSKFileTest::testReadTiledImage()
{
    PCIDSKFile *irvine;
    PCIDSKChannel *channel;
    uint8 data_line[127*127];

    irvine = PCIDSK::Open( "irvtiled.pix", "r", NULL );

    CPPUNIT_ASSERT( irvine != NULL );
    CPPUNIT_ASSERT( strcmp(irvine->GetInterleaving(),"FILE") == 0 );

    channel = irvine->GetChannel(1);

    CPPUNIT_ASSERT( channel->GetBlockWidth() == 127 );
    CPPUNIT_ASSERT( channel->GetBlockHeight() == 127 );

    channel->ReadBlock( 6, data_line );

    CPPUNIT_ASSERT( data_line[128] == 74 );


    CPPUNIT_ASSERT( channel->GetBlockWidth() == 127 );
    CPPUNIT_ASSERT( channel->GetBlockHeight() == 127 );
    
    data_line[4] = 255;
    channel->ReadBlock( 6, data_line, 1, 1, 2, 2 );
    CPPUNIT_ASSERT( data_line[0] == 74 );
    CPPUNIT_ASSERT( data_line[4] == 255 );

    delete irvine;
}

