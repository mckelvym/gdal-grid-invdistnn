/******************************************************************************
 *
 * Purpose:  CPPUnit test for file creation.
 * 
 ******************************************************************************
 * Copyright (c) 2009
 * PCI Geomatics, 50 West Wilmot Street, Richmond Hill, Ont, Canada
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#include "pcidsk.h"
#include <cppunit/extensions/HelperMacros.h>
#include <cstring>
#include <unistd.h>

using namespace PCIDSK;

class PCIDSKCreateTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE( PCIDSKCreateTest );
 
    CPPUNIT_TEST( simplePixelInterleaved );
    CPPUNIT_TEST( tiled );
    CPPUNIT_TEST( tiledRLE );
    CPPUNIT_TEST( tiledJPEG );
    CPPUNIT_TEST( testErrors );
    CPPUNIT_TEST( testFILE );
    CPPUNIT_TEST( testLongFILE );
    CPPUNIT_TEST( testEDB );
    
    // Complex creation and opening support
    CPPUNIT_TEST(createComplex16U);
    CPPUNIT_TEST(createComplex16S);
    CPPUNIT_TEST(createComplex32R);

    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();
    void simplePixelInterleaved();
    void tiled();
    void tiledRLE();
    void tiledJPEG();
    void testErrors();
    void testFILE();
    void testLongFILE();
    void testEDB();
    void createComplex16U();
    void createComplex16S();
    void createComplex32R();
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
/*                               tiled()                                */
/************************************************************************/

void PCIDSKCreateTest::tiled()
{
    PCIDSKFile *file;
    PCIDSKChannel *channel;
    eChanType channel_types[4] = {CHN_8U, CHN_32R};
    uint8 data[127*127*4];

    file = PCIDSK::Create( "tiled_file.pix", 600, 700, 2, channel_types,
                           "TILED", NULL );

    CPPUNIT_ASSERT( file != NULL );
    CPPUNIT_ASSERT( file->GetUpdatable() );
    CPPUNIT_ASSERT( file->GetInterleaving() == "FILE" );

    delete file;

    file = PCIDSK::Open( "tiled_file.pix", "r+", NULL );

    CPPUNIT_ASSERT( file->GetUpdatable() );

    channel = file->GetChannel(1);
    channel->ReadBlock( 2, data );

    CPPUNIT_ASSERT( data[500] == 0 );

    data[500] = 221;

    channel->WriteBlock( 2, data );

    delete file;

    file = PCIDSK::Open( "tiled_file.pix", "r", NULL );

    channel = file->GetChannel(1);
    channel->ReadBlock( 2, data );

    CPPUNIT_ASSERT( data[500] == 221 );
    CPPUNIT_ASSERT( data[501] == 0 );

    delete file;

    unlink( "tiled_file.pix" );
}

/************************************************************************/
/*                               tiledRLE()                             */
/************************************************************************/

void PCIDSKCreateTest::tiledRLE()
{
    PCIDSKFile *file;
    PCIDSKChannel *channel;
    eChanType channel_types[4] = {CHN_8U, CHN_32R};
    uint8 data[127*127*4];

    file = PCIDSK::Create( "tiledrle_file.pix", 600, 700, 2, channel_types,
                           "TILED RLE", NULL );

    CPPUNIT_ASSERT( file != NULL );
    CPPUNIT_ASSERT( file->GetUpdatable() );
    CPPUNIT_ASSERT( file->GetInterleaving() == "FILE" );

    delete file;

    file = PCIDSK::Open( "tiledrle_file.pix", "r+", NULL );

    CPPUNIT_ASSERT( file->GetUpdatable() );

    channel = file->GetChannel(1);
    channel->ReadBlock( 2, data );

    CPPUNIT_ASSERT( data[500] == 0 );

    data[500] = 221;

    channel->WriteBlock( 2, data );

    delete file;

    file = PCIDSK::Open( "tiledrle_file.pix", "r", NULL );

    channel = file->GetChannel(1);
    channel->ReadBlock( 2, data );

    CPPUNIT_ASSERT( data[500] == 221 );
    CPPUNIT_ASSERT( data[501] == 0 );

    delete file;

    unlink( "tiledrle_file.pix" );
}

/************************************************************************/
/*                             tiledJPEG()                              */
/************************************************************************/

void PCIDSKCreateTest::tiledJPEG()
{
    PCIDSKFile *file;
    PCIDSKChannel *channel;
    eChanType channel_types[1] = {CHN_8U};
    uint8 data[127*127*4];

    file = PCIDSK::Create( "tiledjpeg_file.pix", 600, 700, 1, channel_types,
                           "TILED JPEG60", NULL );

    CPPUNIT_ASSERT( file != NULL );

    delete file;

    file = PCIDSK::Open( "tiledjpeg_file.pix", "r+", NULL );

    CPPUNIT_ASSERT( file->GetUpdatable() );

    channel = file->GetChannel(1);
    channel->ReadBlock( 2, data );

    CPPUNIT_ASSERT( data[500] == 0 );

    data[500] = 221;

    channel->WriteBlock( 2, data );

    delete file;

    file = PCIDSK::Open( "tiledjpeg_file.pix", "r", NULL );

    channel = file->GetChannel(1);
    channel->ReadBlock( 2, data );

    CPPUNIT_ASSERT( data[500] > 128 );
    CPPUNIT_ASSERT( data[503] < 32 );

    delete file;

    unlink( "tiledjpeg_file.pix" );
}

/************************************************************************/
/*                         createComplex16U()                           */
/************************************************************************/

void PCIDSKCreateTest::createComplex16U()
{
    PCIDSKFile* file;
    PCIDSKChannel *chan;
    
    eChanType channel_types[2] = { CHN_C16U, CHN_C16U };
    file = PCIDSK::Create("complex_16u_file.pix", 512, 512, 2, channel_types, 
        "BAND", NULL);
    
    CPPUNIT_ASSERT(file != NULL);
    
    delete file;
    file = PCIDSK::Open("complex_16u_file.pix", "r+", NULL);
    
    CPPUNIT_ASSERT(file != NULL);
    
    // Try to get the chnannel
    chan = file->GetChannel(2);
    
    CPPUNIT_ASSERT(chan != NULL);
    CPPUNIT_ASSERT(chan->GetType() == CHN_C16U);
    CPPUNIT_ASSERT(DataTypeSize(chan->GetType()) == 4);
    
    // Write a phase gradient to the channel
    unsigned short* const data =
        new unsigned short[512 * 512 * DataTypeSize(chan->GetType())];
    
    for (std::size_t y = 0; y < 512; y++) {
        for (std::size_t x = 0; x < 512; x++) {
            std::size_t offset = 2 * x + (512 * 2) * y;
            data[offset] = x;
            data[offset + 1] = y; // x + yj
        }
    }
    
    for (std::size_t y = 0; y < 512; y++) {
        unsigned short* line = &data[y * 512 * 2];
        chan->WriteBlock(y, line);
    }
    
    delete[] data;
    delete file;
    
    unlink("complex_16u_file.pix");
}

/************************************************************************/
/*                         createComplex16S()                           */
/************************************************************************/

void PCIDSKCreateTest::createComplex16S()
{
    PCIDSKFile* file;
    PCIDSKChannel *chan;
    
    eChanType channel_types[2] = { CHN_C16S, CHN_C16S };
    file = PCIDSK::Create("complex_16s_file.pix", 512, 512, 2, channel_types, 
        "BAND", NULL);
    
    CPPUNIT_ASSERT(file != NULL);
    
    delete file;
    file = PCIDSK::Open("complex_16s_file.pix", "r+", NULL);
    
    CPPUNIT_ASSERT(file != NULL);
    
    // Try to get the chnannel
    chan = file->GetChannel(2);
    
    CPPUNIT_ASSERT(chan != NULL);
    CPPUNIT_ASSERT(chan->GetType() == CHN_C16S);
    CPPUNIT_ASSERT(DataTypeSize(chan->GetType()) == 4);
    
    // Write a phase gradient to the channel
    short* const data =
        new short[512 * 512 * DataTypeSize(chan->GetType())];
    
    for (std::size_t y = 0; y < 512; y++) {
        for (std::size_t x = 0; x < 512; x++) {
            std::size_t offset = 2 * x + (512 * 2) * y;
            data[offset] = x;
            data[offset + 1] = y; // x + yj
        }
    }
    
    for (std::size_t y = 0; y < 512; y++) {
        short* line = &data[y * 512 * 2];
        chan->WriteBlock(y, line);
    }
    
    delete[] data;
    delete file;
    
    unlink("complex_16s_file.pix");
}

/************************************************************************/
/*                         createComplex32R()                           */
/************************************************************************/

void PCIDSKCreateTest::createComplex32R()
{
    PCIDSKFile* file;
    PCIDSKChannel* chan;
    
    eChanType channel_types[2] = { CHN_C32R, CHN_C32R };
    file = PCIDSK::Create("complex_32r_file.pix", 512, 512, 2, channel_types, 
        "BAND", NULL);
    
    CPPUNIT_ASSERT(file != NULL);
    
    delete file;
    file = PCIDSK::Open("complex_32r_file.pix", "r+", NULL);
    
    CPPUNIT_ASSERT(file != NULL);
    
    // Add a fake GEOref segment
    PCIDSK::PCIDSKSegment* seg = file->GetSegment(SEG_GEO, "", 0);
    CPPUNIT_ASSERT(seg != NULL);
    
    PCIDSK::PCIDSKGeoref* georef = dynamic_cast<PCIDSK::PCIDSKGeoref*>(seg);
    CPPUNIT_ASSERT(georef != NULL);

    // Set some stuff
    georef->WriteSimple("LAT/LONG D000", 16, 4, 0, 16, 0, -4);
    
    file->Synchronize();
    
    // Try to get the chnannel
    chan = file->GetChannel(2);
    
    CPPUNIT_ASSERT(chan != NULL);
    CPPUNIT_ASSERT(chan->GetType() == CHN_C32R);
    CPPUNIT_ASSERT(DataTypeSize(chan->GetType()) == 8);
    
    // Write a phase gradient to the channel
    float* data =
        new float[512 * 512 * 2];
    
    for (std::size_t y = 0; y < 512; y++) {
        for (std::size_t x = 0; x < 512; x++) {
            std::size_t offset = 2 * x + 
                (512 * 2) * y;
            data[offset] = x;
            data[offset + 1] = y; // x + yj
        }
    }
    
    for (std::size_t y = 0; y < 512; y++) {
        float* line = &data[y * 512 * 2];
        chan->WriteBlock(y, line);
    }
    
    // Try to read back the GEOref information
    double a1, a2, xrot, y1, yrot, y3;
    georef->GetTransform(a1, a2, xrot, y1, yrot, y3);
    CPPUNIT_ASSERT_EQUAL(16.0, a1);
    CPPUNIT_ASSERT_EQUAL(4.0, a2);
    CPPUNIT_ASSERT_EQUAL(0.0, xrot);
    CPPUNIT_ASSERT_EQUAL(16.0, y1);
    CPPUNIT_ASSERT_EQUAL(0.0, yrot);
    CPPUNIT_ASSERT_EQUAL(-4.0, y3);
    
    delete[] data;
    delete file;
    
    // re-open the file, and load the first line
    file = PCIDSK::Open("complex_32r_file.pix", "r");
    CPPUNIT_ASSERT(file != NULL);
    
    chan = file->GetChannel(2);
    CPPUNIT_ASSERT(chan != NULL);
    CPPUNIT_ASSERT_EQUAL(CHN_C32R, chan->GetType());
    
    data = new float[512 * 2];
    
    chan->ReadBlock(1, data);
    
    // Check that the first line is equal, as expected.
    for (std::size_t i = 0; i < 512; i++)
    {
        CPPUNIT_ASSERT_EQUAL((float)i, data[i * 2]);
        CPPUNIT_ASSERT_EQUAL(1.0f, data[i * 2 + 1]);
    }
    
    delete[] data;
    delete file;
    
    unlink("complex_32r_file.pix");
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

/************************************************************************/
/*                              testFILE()                              */
/************************************************************************/

void PCIDSKCreateTest::testFILE()
{
    std::string sFilename = "TestFileOption.pix";
    
    unlink( sFilename.c_str() );

    std::string sOption = "FILE";
    PCIDSK::eChanType panTypes[1];

    panTypes[0] = PCIDSK::CHN_8U;

    PCIDSK::PCIDSKFile *poFile = PCIDSK::Create(sFilename, 500, 750,
                                                1, panTypes, sOption, NULL);

    PCIDSK::PCIDSKChannel* poChannel = poFile->GetChannel(1);

    int nWidth = poChannel->GetBlockWidth();
    int nHeight = poChannel->GetBlockHeight();
    PCIDSK::eChanType eType = poChannel->GetType();

    CPPUNIT_ASSERT_EQUAL(PCIDSK::CHN_8U,eType);

    int nBufSize = nWidth*nHeight;
    unsigned char* pBuf = new unsigned char[nBufSize];

    for(int i=0 ; i < nBufSize ; i++)
    {
        pBuf[i] = 10;
    }

    for(int block=0 ; block < poChannel->GetBlockCount(); block++)
    {
        poChannel->WriteBlock(block,pBuf);
    }

    uint64 image_offset, pixel_offset, line_offset;
    bool little_endian;
    std::string fetched_filename;

    poChannel->GetChanInfo( fetched_filename, image_offset, pixel_offset,
                            line_offset, little_endian );

    CPPUNIT_ASSERT( fetched_filename == "TestFileOption.001" );
    CPPUNIT_ASSERT( image_offset == 0 );
    CPPUNIT_ASSERT( pixel_offset == 1 );
    CPPUNIT_ASSERT( line_offset == 500 );
    CPPUNIT_ASSERT_EQUAL( little_endian, true );

    delete [] pBuf;
    delete poFile;

    unlink( sFilename.c_str() );
    unlink( "TestFileOption.001" );
}

/************************************************************************/
/*                            testLongFILE()                            */
/************************************************************************/

void PCIDSKCreateTest::testLongFILE()
{
    std::string sFilename = "ThisFilenameIsSoLongThatTheLinkedFileNamesWillNotFitInThe64CharacterField.pix";
    std::string sLinkFilename = sFilename.substr(0,sFilename.size()-4) + ".001";
    
    unlink( sFilename.c_str() );

    std::string sOption = "FILE";
    PCIDSK::eChanType panTypes[1];

    panTypes[0] = PCIDSK::CHN_8U;

    PCIDSK::PCIDSKFile *poFile = PCIDSK::Create(sFilename, 500, 750,
                                                1, panTypes, sOption, NULL);

    PCIDSK::PCIDSKChannel* poChannel = poFile->GetChannel(1);

    int nWidth = poChannel->GetBlockWidth();
    int nHeight = poChannel->GetBlockHeight();
    PCIDSK::eChanType eType = poChannel->GetType();

    CPPUNIT_ASSERT_EQUAL(PCIDSK::CHN_8U,eType);

    int nBufSize = nWidth*nHeight;
    unsigned char* pBuf = new unsigned char[nBufSize];

    for(int i=0 ; i < nBufSize ; i++)
    {
        pBuf[i] = 10;
    }

    for(int block=0 ; block < poChannel->GetBlockCount(); block++)
    {
        poChannel->WriteBlock(block,pBuf);
    }

    uint64 image_offset, pixel_offset, line_offset;
    bool little_endian;
    std::string fetched_filename;

    poChannel->GetChanInfo( fetched_filename, image_offset, pixel_offset,
                            line_offset, little_endian );

    CPPUNIT_ASSERT( fetched_filename == sLinkFilename );
    CPPUNIT_ASSERT( image_offset == 0 );
    CPPUNIT_ASSERT( pixel_offset == 1 );
    CPPUNIT_ASSERT( line_offset == 500 );
    CPPUNIT_ASSERT_EQUAL( little_endian, true );

    CPPUNIT_ASSERT( poFile->GetSegment( 1024 )->GetName() == "Link" );

    // Ensure that resetting the filename to something short will clear
    // the link segment. 
    
    poChannel->SetChanInfo( "short.001", image_offset, pixel_offset,
                            line_offset, little_endian );

    poChannel->GetChanInfo( fetched_filename, image_offset, pixel_offset,
                            line_offset, little_endian );

    CPPUNIT_ASSERT( poFile->GetSegment( 1024 ) == NULL );

    CPPUNIT_ASSERT( fetched_filename == "short.001" );

    // Ensure that resetting the filename to something long will recreate
    // a link segment.
    
    poChannel->SetChanInfo( sLinkFilename, image_offset, pixel_offset,
                            line_offset, little_endian );

    poChannel->GetChanInfo( fetched_filename, image_offset, pixel_offset,
                            line_offset, little_endian );

    CPPUNIT_ASSERT( poFile->GetSegment( 1024 )->GetName() == "Link" );

    CPPUNIT_ASSERT( fetched_filename == sLinkFilename );

    delete [] pBuf;
    delete poFile;

    unlink( sFilename.c_str() );
    unlink( sLinkFilename.c_str() );
}

/************************************************************************/
/*                              testEDB()                               */
/************************************************************************/

void PCIDSKCreateTest::testEDB()
{
    std::string sFilename = "test_edb.pix";
    std::string sLinkFilename = "eltoro.pix";
    
    unlink( sFilename.c_str() );

    std::string sOption = "FILELINK";
    PCIDSK::eChanType panTypes[1];

    panTypes[0] = PCIDSK::CHN_8U;

    PCIDSK::PCIDSKFile *poFile = PCIDSK::Create(sFilename, 512, 512, 
                                                1, panTypes, sOption, NULL);

    PCIDSK::PCIDSKChannel* poChannel = poFile->GetChannel(1);

    poChannel->SetEChanInfo( sLinkFilename, 1, 100, 200, 512, 512 );


    int echannel, exoff, eyoff, exsize, eysize;
    std::string fetched_filename;

    poChannel->GetEChanInfo( fetched_filename, echannel,
                             exoff, eyoff, exsize, eysize );

    CPPUNIT_ASSERT( fetched_filename == sLinkFilename );
    CPPUNIT_ASSERT( echannel == 1 );
    CPPUNIT_ASSERT( exoff == 100 );
    CPPUNIT_ASSERT( eyoff == 200 );
    CPPUNIT_ASSERT( exsize == 512 );
    CPPUNIT_ASSERT( eysize == 512 );

    // Fetch one line of imagery and ensure it is as expected.

    int nBufSize = 512;
    unsigned char* pBuf = new unsigned char[nBufSize];

    poChannel->ReadBlock( 2, pBuf );

    CPPUNIT_ASSERT( pBuf[0] == 60 );
    CPPUNIT_ASSERT( pBuf[511] == 36 );
    
    delete [] pBuf;
    delete poFile;

    unlink( sFilename.c_str() );
}
