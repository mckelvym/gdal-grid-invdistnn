/******************************************************************************
 *
 * Purpose:  CPPUnit test for writing vector segments.
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
#include "pcidsk_vectorsegment.h"
#include <cmath>
#include <cppunit/extensions/HelperMacros.h>
#include <vector>

using namespace PCIDSK;

class VectorWriteTest : public CppUnit::TestFixture
{
    CPPUNIT_TEST_SUITE( VectorWriteTest );
 
    CPPUNIT_TEST( testProjection );
    CPPUNIT_TEST( testSchema );

    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();
    void testProjection();
    void testSchema();
};

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( VectorWriteTest );

void VectorWriteTest::setUp()
{
}

void VectorWriteTest::tearDown()
{
}

void VectorWriteTest::testProjection()
{
    PCIDSKFile *file;
    eChanType channel_types[1] = {CHN_8U};
    
    file = PCIDSK::Create( "vectestproj.pix", 50, 40, 1, channel_types,
                           "BAND", NULL );

    CPPUNIT_ASSERT( file != NULL );

    int seg_num = file->CreateSegment("VTEST", "Test write", SEG_VEC, 0);
    
    PCIDSKSegment* seg = file->GetSegment(seg_num);
    
    CPPUNIT_ASSERT(seg != NULL);
    
    PCIDSKVectorSegment* vec_seg = dynamic_cast<PCIDSKVectorSegment*>(seg);
    
    CPPUNIT_ASSERT(vec_seg != NULL);

    std::string geosys;
    std::vector<double> parms = vec_seg->GetProjection( geosys );

    CPPUNIT_ASSERT( geosys == "METRE           " );
    for( unsigned int i = 0; i < 18; i++ )
    {
        if( i == 17 )
            CPPUNIT_ASSERT( parms[i] == UNIT_METER );
        else
            CPPUNIT_ASSERT( parms[i] == 0 );
    }

    // Try setting a simple projection.
    vec_seg->SetProjection( "LONG        D000", parms );

    parms = vec_seg->GetProjection( geosys );

    CPPUNIT_ASSERT( geosys == "LONG        D000" );
    for( unsigned int i = 0; i < 18; i++ )
    {
        if( i == 17 )
            CPPUNIT_ASSERT( parms[i] == -1 );
        else
            CPPUNIT_ASSERT( parms[i] == 0 );
    }

    // Try a complex projection with parameters.

    parms[2] = -117.0;
    parms[3] = 33;
    parms[6] = 200000;
    parms[7] = 100000;
    parms[8] = 0.998;
    parms[17] = 2;

    vec_seg->SetProjection( "TM          D000", parms );

    // Close and reopen.
    delete file;

    file = PCIDSK::Open( "vectestproj.pix", "r", NULL );

    seg = file->GetSegment(seg_num);
    
    CPPUNIT_ASSERT(seg != NULL);
    
    vec_seg = dynamic_cast<PCIDSKVectorSegment*>(seg);
    
    CPPUNIT_ASSERT(vec_seg != NULL);

    // Confirm changes stuck.

    std::vector<double> old_parms = parms;
    parms.clear();

    parms = vec_seg->GetProjection( geosys );

    CPPUNIT_ASSERT( geosys == "TM          D000" );
    for( unsigned int i = 0; i < 17; i++ )
    {
        CPPUNIT_ASSERT( parms[i] == old_parms[i] );
    }

    CPPUNIT_ASSERT( parms[17] == -1 );

    delete file;

    unlink( "vectestproj.pix" );
}

// Ensure we can create every type of field in the schema, and
// instantiate them in a record and that defaults work.

void VectorWriteTest::testSchema()
{
    PCIDSKFile *file;
    eChanType channel_types[1] = {CHN_8U};
    
    file = PCIDSK::Create( "vectestschema.pix", 50, 40, 1, channel_types,
                           "BAND", NULL );

    CPPUNIT_ASSERT( file != NULL );

    int seg_num = file->CreateSegment("VTEST", "Test write", SEG_VEC, 0);
    
    PCIDSKSegment* seg = file->GetSegment(seg_num);
    
    CPPUNIT_ASSERT(seg != NULL);
    
    PCIDSKVectorSegment* vec_seg = dynamic_cast<PCIDSKVectorSegment*>(seg);
    
    CPPUNIT_ASSERT(vec_seg != NULL);

    // Create each field type with provided defaults.

    CPPUNIT_ASSERT( vec_seg->GetFieldCount() == 0 );

    ShapeField dfield;

    dfield.SetValue( 100 );
    vec_seg->AddField( "FIELD1", FieldTypeInteger, "Field 1 Description",
                       "%6d", &dfield );

    dfield.SetValue( (float) 32.5 );
    vec_seg->AddField( "FIELD2", FieldTypeFloat, "Field 2 Description",
                       "%6.3f", &dfield );

    dfield.SetValue( (double) 16.25 );
    vec_seg->AddField( "FIELD3", FieldTypeDouble, "Field 3 Description",
                       "%15.3f", &dfield );

    dfield.SetValue( "NOT_SET" );
    vec_seg->AddField( "FIELD4", FieldTypeString, "Field 4 Description",
                       "%-13s", &dfield );

    std::vector<int32> x123;
    x123.push_back( 1 );
    x123.push_back( 2 );
    x123.push_back( 3 );

    dfield.SetValue( x123 );
    vec_seg->AddField( "FIELD5", FieldTypeCountedInt, "Field 5 Description",
                       "%4d", &dfield );

    // Check that they seem to be defined properly

    CPPUNIT_ASSERT( vec_seg->GetFieldCount() == 5 );

    CPPUNIT_ASSERT( vec_seg->GetFieldName(0) == "FIELD1" );
    CPPUNIT_ASSERT( vec_seg->GetFieldType(0) == FieldTypeInteger );
    CPPUNIT_ASSERT( vec_seg->GetFieldDescription(0) == "Field 1 Description" );
    CPPUNIT_ASSERT( vec_seg->GetFieldFormat(0) == "%6d" );
    CPPUNIT_ASSERT( vec_seg->GetFieldDefault(0).GetType() == FieldTypeInteger);
    CPPUNIT_ASSERT( vec_seg->GetFieldDefault(0).GetValueInteger() == 100 );

    CPPUNIT_ASSERT( vec_seg->GetFieldName(1) == "FIELD2" );
    CPPUNIT_ASSERT( vec_seg->GetFieldType(1) == FieldTypeFloat );
    CPPUNIT_ASSERT( vec_seg->GetFieldDescription(1) == "Field 2 Description" );
    CPPUNIT_ASSERT( vec_seg->GetFieldFormat(1) == "%6.3f" );
    CPPUNIT_ASSERT( vec_seg->GetFieldDefault(1).GetType() == FieldTypeFloat);
    CPPUNIT_ASSERT( vec_seg->GetFieldDefault(1).GetValueFloat() == 32.5 );

    CPPUNIT_ASSERT( vec_seg->GetFieldName(2) == "FIELD3" );
    CPPUNIT_ASSERT( vec_seg->GetFieldType(2) == FieldTypeDouble );
    CPPUNIT_ASSERT( vec_seg->GetFieldDescription(2) == "Field 3 Description" );
    CPPUNIT_ASSERT( vec_seg->GetFieldFormat(2) == "%15.3f" );
    CPPUNIT_ASSERT( vec_seg->GetFieldDefault(2).GetType() == FieldTypeDouble);
    CPPUNIT_ASSERT( vec_seg->GetFieldDefault(2).GetValueDouble() == 16.25 );

    CPPUNIT_ASSERT( vec_seg->GetFieldName(3) == "FIELD4" );
    CPPUNIT_ASSERT( vec_seg->GetFieldType(3) == FieldTypeString );
    CPPUNIT_ASSERT( vec_seg->GetFieldDescription(3) == "Field 4 Description" );
    CPPUNIT_ASSERT( vec_seg->GetFieldFormat(3) == "%-13s" );
    CPPUNIT_ASSERT( vec_seg->GetFieldDefault(3).GetType() == FieldTypeString);
    CPPUNIT_ASSERT( vec_seg->GetFieldDefault(3).GetValueString() == "NOT_SET");

    CPPUNIT_ASSERT( vec_seg->GetFieldName(4) == "FIELD5" );
    CPPUNIT_ASSERT( vec_seg->GetFieldType(4) == FieldTypeCountedInt );
    CPPUNIT_ASSERT( vec_seg->GetFieldDescription(4) == "Field 5 Description" );
    CPPUNIT_ASSERT( vec_seg->GetFieldFormat(4) == "%4d" );

    ShapeField d2 = vec_seg->GetFieldDefault(4);
    CPPUNIT_ASSERT( d2.GetType() == FieldTypeCountedInt );
    CPPUNIT_ASSERT( d2.GetValueCountedInt().size() == 3 );
    CPPUNIT_ASSERT( d2.GetValueCountedInt()[0] == 1 );
    CPPUNIT_ASSERT( d2.GetValueCountedInt()[1] == 2 );
    CPPUNIT_ASSERT( d2.GetValueCountedInt()[2] == 3 );

    // Create a feature with defaulted fields.

    ShapeId first_id = vec_seg->CreateShape();

    // Create a feature with defined fields.

    std::vector<ShapeField> field_list;
    std::vector<int32>      int_list;

    ShapeId id = vec_seg->CreateShape();

    field_list.resize( vec_seg->GetFieldCount() );
    field_list[0].SetValue( 321 );
    field_list[1].SetValue( (float) 701.5 );
    field_list[2].SetValue( (double) 350.25 );
    field_list[3].SetValue( "A String" );

    int_list.push_back( 777 );
    int_list.push_back( 1000 );
    field_list[4].SetValue( int_list );

    vec_seg->SetFields( id, field_list );

    // Check the defaults are returned for the first shape.

    std::vector<ShapeField> fields_ret;

    vec_seg->GetFields( first_id, fields_ret );

    CPPUNIT_ASSERT( fields_ret[0].GetValueInteger() == 100 );
    CPPUNIT_ASSERT( fields_ret[1].GetValueFloat() == 32.5 );
    CPPUNIT_ASSERT( fields_ret[2].GetValueDouble() == 16.25 );
    CPPUNIT_ASSERT( fields_ret[3].GetValueString() == "NOT_SET" );
    CPPUNIT_ASSERT( fields_ret[4].GetValueCountedInt().size() == 3 );
    CPPUNIT_ASSERT( fields_ret[4].GetValueCountedInt()[0] == 1 );
    CPPUNIT_ASSERT( fields_ret[4].GetValueCountedInt()[1] == 2 );
    CPPUNIT_ASSERT( fields_ret[4].GetValueCountedInt()[2] == 3 );

    // Check the second record with real values. 
    unsigned int i;

    vec_seg->GetFields( id, fields_ret );
    for( i = 0; i < 5; i++ )
    {
        CPPUNIT_ASSERT( fields_ret[i] == field_list[i] );
    }

    // Close and reopen the file.
    
    delete file;

    file = PCIDSK::Open( "vectestschema.pix", "r", NULL );

    seg = file->GetSegment(seg_num);
    
    CPPUNIT_ASSERT(seg != NULL);
    
    vec_seg = dynamic_cast<PCIDSKVectorSegment*>(seg);
    
    CPPUNIT_ASSERT(vec_seg != NULL);

    // Check the defaults are returned for the first shape.

    vec_seg->GetFields( first_id, fields_ret );

    CPPUNIT_ASSERT( fields_ret[0].GetValueInteger() == 100 );
    CPPUNIT_ASSERT( fields_ret[1].GetValueFloat() == 32.5 );
    CPPUNIT_ASSERT( fields_ret[2].GetValueDouble() == 16.25 );
    CPPUNIT_ASSERT( fields_ret[3].GetValueString() == "NOT_SET" );
    CPPUNIT_ASSERT( fields_ret[4].GetValueCountedInt().size() == 3 );
    CPPUNIT_ASSERT( fields_ret[4].GetValueCountedInt()[0] == 1 );
    CPPUNIT_ASSERT( fields_ret[4].GetValueCountedInt()[1] == 2 );
    CPPUNIT_ASSERT( fields_ret[4].GetValueCountedInt()[2] == 3 );

    // Check the second record with real values. 
    vec_seg->GetFields( id, fields_ret );
    for( i = 0; i < 5; i++ )
    {
        CPPUNIT_ASSERT( fields_ret[i] == field_list[i] );
    }

    delete file;

    unlink( "vectestschema.pix" );
}
