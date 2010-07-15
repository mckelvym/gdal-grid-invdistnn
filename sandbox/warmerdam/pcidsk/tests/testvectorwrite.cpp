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
    CPPUNIT_TEST( testGeometry );
    CPPUNIT_TEST( testStress );
    CPPUNIT_TEST( testShapeId );

    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();
    void tearDown();

    void testProjection();
    void testSchema();
    void testGeometry();
    void testShapeId();
    void testStress();
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
    
    CPPUNIT_ASSERT( seg->ConsistencyCheck() == "" );

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

    CPPUNIT_ASSERT( seg->ConsistencyCheck() == "" );

    delete file;

    unlink( "vectestschema.pix" );
}


// Test assignment of specific shapeids. 

void VectorWriteTest::testShapeId()
{
    PCIDSKFile *file;
    eChanType channel_types[1] = {CHN_8U};
    
    file = PCIDSK::Create( "vectestid.pix", 50, 40, 1, channel_types,
                           "BAND", NULL );

    CPPUNIT_ASSERT( file != NULL );

    int seg_num = file->CreateSegment("VTEST", "Test write", SEG_VEC, 0);
    
    PCIDSKSegment* seg = file->GetSegment(seg_num);
    
    CPPUNIT_ASSERT(seg != NULL);
    
    PCIDSKVectorSegment* vec_seg = dynamic_cast<PCIDSKVectorSegment*>(seg);
    
    CPPUNIT_ASSERT(vec_seg != NULL);

    // Create a feature with a specific id.

    ShapeId id = vec_seg->CreateShape( 20 );

    CPPUNIT_ASSERT( id == 20 );

    // Create a feature with defaulted id.

    id = vec_seg->CreateShape();

    CPPUNIT_ASSERT( id == 21 );

    // Create a feature with set id mixed in the existing range.

    id = vec_seg->CreateShape(18);

    CPPUNIT_ASSERT( id == 18 );

    // Create a feature with an already used id, and check for exception.

    try 
    {
        vec_seg->CreateShape( 20 );
        CPPUNIT_ASSERT( false );
    } 
    catch( PCIDSK::PCIDSKException ex )
    {
        CPPUNIT_ASSERT( strstr(ex.what(),"20") != NULL );
    }

    // Close and reopen the file.
    
    CPPUNIT_ASSERT( seg->ConsistencyCheck() == "" );

    delete file;

    file = PCIDSK::Open( "vectestid.pix", "r+", NULL );

    seg = file->GetSegment(seg_num);
    
    CPPUNIT_ASSERT(seg != NULL);
    
    vec_seg = dynamic_cast<PCIDSKVectorSegment*>(seg);
    
    CPPUNIT_ASSERT(vec_seg != NULL);

    CPPUNIT_ASSERT( seg->ConsistencyCheck() == "" );

    // Create a new feature with a specific id.

    id = vec_seg->CreateShape( 45 );

    CPPUNIT_ASSERT( id == 45 );

    CPPUNIT_ASSERT( seg->ConsistencyCheck() == "" );

    delete file;

    unlink( "vectestid.pix" );
}

// Test minimal geometry writing and reading back.

void VectorWriteTest::testGeometry()
{
    PCIDSKFile *file;
    eChanType channel_types[1] = {CHN_8U};
    
    file = PCIDSK::Create( "vectestgeom.pix", 50, 40, 1, channel_types,
                           "BAND", NULL );

    CPPUNIT_ASSERT( file != NULL );

    int seg_num = file->CreateSegment("VTEST", "Test write", SEG_VEC, 0);
    
    PCIDSKSegment* seg = file->GetSegment(seg_num);
    
    CPPUNIT_ASSERT(seg != NULL);
    
    PCIDSKVectorSegment* vec_seg = dynamic_cast<PCIDSKVectorSegment*>(seg);
    
    CPPUNIT_ASSERT(vec_seg != NULL);

    // Create a feature with no geometry.

    ShapeId first_id = vec_seg->CreateShape();

    // Create a feature with geometry.

    std::vector<ShapeVertex> vlist;

    ShapeId id = vec_seg->CreateShape();

    vlist.resize( 4 );
    
    vlist[0].x = 0;
    vlist[0].y = 0;
    vlist[0].z = 0;
    vlist[1].x = 100;
    vlist[1].y = 0;
    vlist[1].z = 0;
    vlist[2].x = 100;
    vlist[2].y = 100;
    vlist[2].z = 0;
    vlist[3].x = 0;
    vlist[3].y = 0;
    vlist[3].z = 25;

    vec_seg->SetVertices( id, vlist );

    // Check the first shape.

    std::vector<ShapeVertex> v2list;

    vec_seg->GetVertices( first_id, v2list );

    CPPUNIT_ASSERT( v2list.size() == 0 );

    // Check the second shape with real values. 
    unsigned int i;

    vec_seg->GetVertices( id, v2list );

    CPPUNIT_ASSERT( v2list.size() == vlist.size());
    
    for( i = 0; i < vlist.size(); i++ )
    {
        CPPUNIT_ASSERT( vlist[i].x == v2list[i].x );
        CPPUNIT_ASSERT( vlist[i].y == v2list[i].y );
        CPPUNIT_ASSERT( vlist[i].z == v2list[i].z );
    }

    // Close and reopen the file.
    
    CPPUNIT_ASSERT( seg->ConsistencyCheck() == "" );

    delete file;

    file = PCIDSK::Open( "vectestgeom.pix", "r+", NULL );

    seg = file->GetSegment(seg_num);
    
    CPPUNIT_ASSERT(seg != NULL);
    CPPUNIT_ASSERT( seg->ConsistencyCheck() == "" );
    
    vec_seg = dynamic_cast<PCIDSKVectorSegment*>(seg);
    
    CPPUNIT_ASSERT(vec_seg != NULL);

    // Check the first shape.

    vec_seg->GetVertices( first_id, v2list );

    CPPUNIT_ASSERT( v2list.size() == 0 );

    // Check the second shape with real values. 

    vec_seg->GetVertices( id, v2list );

    CPPUNIT_ASSERT( v2list.size() == vlist.size());
    
    for( i = 0; i < vlist.size(); i++ )
    {
        CPPUNIT_ASSERT( vlist[i].x == v2list[i].x );
        CPPUNIT_ASSERT( vlist[i].y == v2list[i].y );
        CPPUNIT_ASSERT( vlist[i].z == v2list[i].z );
    }
    
    // Set vertices on the first feature.
    
    vlist.resize(3);
    vec_seg->SetVertices( first_id, vlist );

    vec_seg->GetVertices( first_id, v2list );

    CPPUNIT_ASSERT( v2list.size() == 3 );
    CPPUNIT_ASSERT( v2list[2].y == 100 );

    // Grow the geometry for the second feature.
    vlist.resize(5);

    vlist[3].x = 0;
    vlist[3].y = 0;
    vlist[3].z = 25;
    vlist[4].x = -100;
    vlist[4].y = -50;
    vlist[4].z = 15;

    vec_seg->SetVertices( id, vlist );

    vec_seg->GetVertices( id, v2list );

    CPPUNIT_ASSERT( v2list.size() == 5 );
    CPPUNIT_ASSERT( v2list[2].y == 100 );
    CPPUNIT_ASSERT( v2list[4].x == -100 );

    CPPUNIT_ASSERT( seg->ConsistencyCheck() == "" );

    delete file;

    unlink( "vectestgeom.pix" );
}

//
// Try to stress test writing, update and reading on a moderately large
// layer.
//

void VectorWriteTest::testStress()
{
    PCIDSKFile *file;
    eChanType channel_types[1] = {CHN_8U};
    
    file = PCIDSK::Create( "vecteststress.pix", 50, 40, 1, channel_types,
                           "BAND", NULL );

    CPPUNIT_ASSERT( file != NULL );

    int seg_num = file->CreateSegment("VTEST", "Test write", SEG_VEC, 0);
    
    PCIDSKSegment* seg = file->GetSegment(seg_num);
    
    CPPUNIT_ASSERT(seg != NULL);
    
    PCIDSKVectorSegment* vec_seg = dynamic_cast<PCIDSKVectorSegment*>(seg);
    
    CPPUNIT_ASSERT(vec_seg != NULL);

    // Create some simple attributes.

    ShapeField dfield;

    dfield.SetValue( 100 );
    vec_seg->AddField( "FIELD1", FieldTypeInteger, "Field 1 Description",
                       "%7d", &dfield );

    vec_seg->AddField( "FIELD2", FieldTypeString, "Field 2 Description",
                       "" );

    // Create a base set of features with predictable geometry
    // and fields. 

    const static unsigned int num_features = 15000;
    unsigned int i;
    std::vector<ShapeField> fields;

    fields.resize(2);

    for( i = 0; i < num_features; i++ )
    {
        ShapeId id = vec_seg->CreateShape();
        std::string f2;
        uint32 vcount = i % 30, v;

        fields[0].SetValue( (int32) i+100 );

        f2.append( i % 40, 'a' + (i / 100) % 10 );
        fields[1].SetValue( f2 );

        vec_seg->SetFields( id, fields );

        std::vector<ShapeVertex> vertices;

        vertices.resize(vcount);
        for( v = 0; v < vcount; v++ )
        {
            vertices[v].x = 100000 + v + i;
            vertices[v].y = 200000 + v + i*2;
            vertices[v].z = i/5.0 + v;
        }

        vec_seg->SetVertices( id, vertices );
    }
    
    CPPUNIT_ASSERT( seg->ConsistencyCheck() == "" );

    // Reopen the file.

    delete file;

    file = PCIDSK::Open( "vecteststress.pix", "r+", NULL );

    seg = file->GetSegment(seg_num);
    
    CPPUNIT_ASSERT(seg != NULL);
    CPPUNIT_ASSERT( seg->ConsistencyCheck() == "" );
    
    vec_seg = dynamic_cast<PCIDSKVectorSegment*>(seg);
    
    CPPUNIT_ASSERT(vec_seg != NULL);

    // Read the contents, confirming contents.

    ShapeId       counter = 0;

    for( ShapeIterator it = vec_seg->begin();
         it != vec_seg->end(); 
         it++, counter++ )
    {
        std::vector<ShapeField>  fields;
        std::vector<ShapeVertex> vertices;

        CPPUNIT_ASSERT( *it == counter );

        vec_seg->GetVertices( *it, vertices );
        
        uint32 vcount = (counter % 30);
        CPPUNIT_ASSERT( vertices.size() == vcount );
        if( vcount > 0 )
        {
            CPPUNIT_ASSERT( vertices[vcount-1].y 
                            == 200000 + vcount - 1 + counter * 2 );
        }

        vec_seg->GetFields( *it, fields );

        CPPUNIT_ASSERT( fields[0].GetValueInteger() == counter+100 );

        std::string f2;
        f2.append( counter % 40, 'a' + (counter / 100) % 10 );

        CPPUNIT_ASSERT( fields[1].GetValueString() == f2 );
    }
    
    // Now we will update selected feature geometries and fields. 
    
    ShapeId id;
    std::vector<ShapeVertex> uvertices;
    std::vector<ShapeField>  ufields;

    uvertices.resize(15);
    for( uint32 v = 0; v < uvertices.size(); v++ )
    {
        uvertices[v].x = 500000 + v;
        uvertices[v].y = 600000 + v;
        uvertices[v].z = -100 + v;
    }

    ufields.resize(2);
    ufields[0].SetValue( 1000 );
    ufields[1].SetValue( "FIXED LENGTH STRING" );

    for( id = 100; id < (ShapeId) num_features; id += 1200 )
    {
        vec_seg->SetVertices( id, uvertices );
        vec_seg->SetFields( id, ufields );
    }

    CPPUNIT_ASSERT( seg->ConsistencyCheck() == "" );
    
    // Confirm updates.
    
    counter = 0;
    for( ShapeIterator it = vec_seg->begin();
         it != vec_seg->end(); 
         it++, counter++ )
    {
        std::vector<ShapeField>  fields;
        std::vector<ShapeVertex> vertices;

        CPPUNIT_ASSERT( *it == counter );
        
        vec_seg->GetVertices( *it, vertices );
        vec_seg->GetFields( *it, fields );

        // special case override for modified features.
        if( (counter-100) % 1200 == 0 )
        {
            CPPUNIT_ASSERT( fields[0] == ufields[0] );
            CPPUNIT_ASSERT( fields[1] == ufields[1] );

            CPPUNIT_ASSERT( vertices.size() == uvertices.size() );
            for( uint32 v = 0; v < uvertices.size(); v++ )
            {
                CPPUNIT_ASSERT( uvertices[v].x == vertices[v].x );
                CPPUNIT_ASSERT( uvertices[v].y == vertices[v].y );
                CPPUNIT_ASSERT( uvertices[v].z == vertices[v].z );
            }

            continue;
        }
        
        // validate original features.
        uint32 vcount = (counter % 30);
        CPPUNIT_ASSERT( vertices.size() == vcount );
        if( vcount > 0 )
        {
            CPPUNIT_ASSERT( vertices[vcount-1].y 
                            == 200000 + vcount - 1 + counter * 2 );
        }

        CPPUNIT_ASSERT( fields[0].GetValueInteger() == counter+100 );

        std::string f2;
        f2.append( counter % 40, 'a' + (counter / 100) % 10 );

        CPPUNIT_ASSERT( fields[1].GetValueString() == f2 );
    }

    // Delete a few shapes

    int delta = 0;

    vec_seg->DeleteShape( (ShapeId) (counter - 1) );
    delta++;

    if( 3000 < num_features-1 )
    {
        vec_seg->DeleteShape( 3000 );
        delta++;
    }
    if( 9000 < num_features-2 )
    {
        vec_seg->DeleteShape( 9000 );
        delta++;
    }
    
    // Confirm deletes.
    
    counter = 0;
    for( ShapeIterator it = vec_seg->begin();
         it != vec_seg->end(); 
         it++, counter++ )
    {
        std::vector<ShapeField>  fields;
        std::vector<ShapeVertex> vertices;

        vec_seg->GetVertices( *it, vertices );
        vec_seg->GetFields( *it, fields );

        if( counter == 3000 || counter == 9000 )
        {
            CPPUNIT_ASSERT( *it != counter );
            continue;
        }

        if( *it != counter )
        {
            printf( "*it = %d, counter = %d\n", 
                    (int) *it, counter );
        }

        CPPUNIT_ASSERT( *it == counter );
        
        // special case override for modified features.
        if( (counter-100) % 1200 == 0 )
        {
            CPPUNIT_ASSERT( fields[0] == ufields[0] );
            CPPUNIT_ASSERT( fields[1] == ufields[1] );

            CPPUNIT_ASSERT( vertices.size() == uvertices.size() );
            for( uint32 v = 0; v < uvertices.size(); v++ )
            {
                CPPUNIT_ASSERT( uvertices[v].x == vertices[v].x );
                CPPUNIT_ASSERT( uvertices[v].y == vertices[v].y );
                CPPUNIT_ASSERT( uvertices[v].z == vertices[v].z );
            }

            continue;
        }
        
        // validate original features.
        uint32 vcount = (counter % 30);
        CPPUNIT_ASSERT( vertices.size() == vcount );
        if( vcount > 0 )
        {
            CPPUNIT_ASSERT( vertices[vcount-1].y 
                            == 200000 + vcount - 1 + counter * 2 );
        }

        CPPUNIT_ASSERT( fields[0].GetValueInteger() == counter+100 );

        std::string f2;
        f2.append( counter % 40, 'a' + (counter / 100) % 10 );

        CPPUNIT_ASSERT( fields[1].GetValueString() == f2 );
    }

    CPPUNIT_ASSERT( counter == (int) (num_features - delta) );


    CPPUNIT_ASSERT( seg->ConsistencyCheck() == "" );
    delete file;
//    unlink( "vecteststress.pix" );
}

