#include "pcidsk.h"
#include <cppunit/extensions/HelperMacros.h>
#include <cstring>

using namespace PCIDSK;

class PCIDSKUpdateTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE( PCIDSKUpdateTest );
 
    CPPUNIT_TEST( updateBandInterleaved );
    CPPUNIT_TEST( updatePixelInterleaved );
    CPPUNIT_TEST( testReadonly );

    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();
    void updateBandInterleaved();
    void updatePixelInterleaved();
    void testReadonly();
};

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( PCIDSKUpdateTest );

void PCIDSKUpdateTest::setUp()
{
}

void PCIDSKUpdateTest::tearDown()
{
}

/************************************************************************/
/*                       updateBandInterleaved()                        */
/************************************************************************/

void PCIDSKUpdateTest::updateBandInterleaved()
{
/* -------------------------------------------------------------------- */
/*      Create a simple pcidsk file.                                    */
/* -------------------------------------------------------------------- */
    PCIDSKFile *band_file;
    eChanType channel_types[4] = {CHN_8U, CHN_8U, CHN_8U, CHN_32R};

    band_file = PCIDSK::Create( "band_update.pix", 300, 200, 4, channel_types, 
                                 "BAND", NULL );

    CPPUNIT_ASSERT( band_file != NULL );
    
/* -------------------------------------------------------------------- */
/*      Update channel 2.                                               */
/* -------------------------------------------------------------------- */
    PCIDSKChannel *chan;

    uint8 data_line_8[300];
    int i;

    for( i = 0; i < 300; i++ )
        data_line_8[i] = i % 256;

    chan = band_file->GetChannel(2);
    chan->WriteBlock( 3, data_line_8 );

/* -------------------------------------------------------------------- */
/*      Update channel 4.                                               */
/* -------------------------------------------------------------------- */

    float data_line_32[300];

    for( i = 0; i < 300; i++ )
        data_line_32[i] = i * 1.5;

    chan = band_file->GetChannel(4);
    chan->WriteBlock( 3, data_line_32 );

/* -------------------------------------------------------------------- */
/*      Close and reopen file.                                          */
/* -------------------------------------------------------------------- */
    delete band_file;
    band_file = PCIDSK::Open( "band_update.pix", "r", NULL );
    
/* -------------------------------------------------------------------- */
/*      Read and check channel 2.                                       */
/* -------------------------------------------------------------------- */
    chan = band_file->GetChannel(2);
    chan->ReadBlock( 3, data_line_8 );

    for( i = 0; i < 300; i++ )
        CPPUNIT_ASSERT( data_line_8[i] == i % 256 );
    
/* -------------------------------------------------------------------- */
/*      Read and check channel 4.                                       */
/* -------------------------------------------------------------------- */
    chan = band_file->GetChannel(4);
    chan->ReadBlock( 3, data_line_32 );

    for( i = 0; i < 300; i++ )
        CPPUNIT_ASSERT( data_line_32[i] == i * 1.5 );

    delete band_file;

    unlink( "band_update.pix" );
}

/************************************************************************/
/*                       updatePixelInterleaved()                       */
/************************************************************************/

void PCIDSKUpdateTest::updatePixelInterleaved()
{
/* -------------------------------------------------------------------- */
/*      Create a simple pcidsk file.                                    */
/* -------------------------------------------------------------------- */
    PCIDSKFile *pixel_file;
    eChanType channel_types[4] = {CHN_8U, CHN_8U, CHN_8U, CHN_32R};

    pixel_file = PCIDSK::Create( "pixel_update.pix", 300, 200, 4, channel_types,
                                 "PIXEL", NULL );

    CPPUNIT_ASSERT( pixel_file != NULL );
    
/* -------------------------------------------------------------------- */
/*      Update channel 2.                                               */
/* -------------------------------------------------------------------- */
    PCIDSKChannel *chan;

    uint8 data_line_8[300];
    int i;

    for( i = 0; i < 300; i++ )
        data_line_8[i] = i % 256;

    chan = pixel_file->GetChannel(2);
    chan->WriteBlock( 3, data_line_8 );

/* -------------------------------------------------------------------- */
/*      Update channel 4.                                               */
/* -------------------------------------------------------------------- */

    float data_line_32[300];

    for( i = 0; i < 300; i++ )
        data_line_32[i] = i * 1.5;

    chan = pixel_file->GetChannel(4);
    chan->WriteBlock( 3, data_line_32 );

/* -------------------------------------------------------------------- */
/*      Close and reopen file.                                          */
/* -------------------------------------------------------------------- */
    delete pixel_file;
    pixel_file = PCIDSK::Open( "pixel_update.pix", "r", NULL );
    
/* -------------------------------------------------------------------- */
/*      Read and check channel 2.                                       */
/* -------------------------------------------------------------------- */
    chan = pixel_file->GetChannel(2);
    chan->ReadBlock( 3, data_line_8 );

    for( i = 0; i < 300; i++ )
        CPPUNIT_ASSERT( data_line_8[i] == i % 256 );
    
/* -------------------------------------------------------------------- */
/*      Read and check channel 4.                                       */
/* -------------------------------------------------------------------- */
    chan = pixel_file->GetChannel(4);
    chan->ReadBlock( 3, data_line_32 );

    for( i = 0; i < 300; i++ )
        CPPUNIT_ASSERT( data_line_32[i] == i * 1.5 );

    delete pixel_file;

    unlink( "pixel_update.pix" );
}

/************************************************************************/
/*                            testReadonly()                            */
/************************************************************************/

void PCIDSKUpdateTest::testReadonly()
{
    PCIDSKFile *file = PCIDSK::Open( "eltoro.pix", "r", NULL );

    CPPUNIT_ASSERT( !file->GetUpdatable() );

    try 
    {
        uint8 line_buffer[1024];

        file->GetChannel(1)->WriteBlock( 1, line_buffer );
        CPPUNIT_ASSERT( false );
    } 
    catch( PCIDSK::PCIDSKException ex )
    {
        CPPUNIT_ASSERT( strstr(ex.what(),"update") != NULL );
    }

    delete file;
}

