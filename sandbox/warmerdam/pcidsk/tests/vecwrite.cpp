/******************************************************************************
 *
 * Purpose:  Commandline utility to copy a PCIDSK vector file.
 * 
 ******************************************************************************
 * Copyright (c) 2010
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
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <assert.h>

using namespace PCIDSK;

/************************************************************************/
/*                               Usage()                                */
/************************************************************************/

static void Usage()

{
    printf( "Usage: vecwrite src_file dst_file\n" );
    exit( 1 );
}

/************************************************************************/
/*                                main()                                */
/************************************************************************/

int main( int argc, char **argv)
{

/* -------------------------------------------------------------------- */
/*      Process options.                                                */
/* -------------------------------------------------------------------- */
    const char *src_filename = NULL;
    const char *dst_filename = NULL;
    int i_arg;

    for( i_arg = 1; i_arg < argc; i_arg++ )
    {
        if( argv[i_arg][0] == '-' )
            Usage();
        else if( src_filename == NULL )
            src_filename = argv[i_arg];
        else if( dst_filename == NULL )
            dst_filename = argv[i_arg];
        else
            Usage();
    }

    if( dst_filename == NULL )
        Usage();

/* -------------------------------------------------------------------- */
/*      Open source file.                                               */
/* -------------------------------------------------------------------- */
    try
    {
        PCIDSKFile *src_file, *dst_file;

        src_file = Open( src_filename, "r", NULL );

/* -------------------------------------------------------------------- */
/*      Create output file.                                             */
/* -------------------------------------------------------------------- */
        dst_file = Create( dst_filename, 
                           src_file->GetWidth(), src_file->GetHeight(),
                           0, NULL, "BAND", NULL );

/* -------------------------------------------------------------------- */
/*      Copy georeferencing.                                            */
/* -------------------------------------------------------------------- */
        PCIDSKGeoref*src_geo = dynamic_cast<PCIDSKGeoref*>(src_file->GetSegment(1));

        if( src_geo != NULL )
        {
            double a1, a2, xrot, b1, yrot, b3;

            PCIDSKGeoref *dst_geo = 
                dynamic_cast<PCIDSKGeoref*>(dst_file->GetSegment(1));
            std::vector<double> parms;

            src_geo->GetTransform( a1, a2, xrot, b1, yrot, b3 );
        
            dst_geo->WriteSimple( src_geo->GetGeosys(), 
                                  a1, a2, xrot, b1, yrot, b3 );

            parms = src_geo->GetParameters();
            dst_geo->WriteParameters( parms );
        }
    
/* -------------------------------------------------------------------- */
/*      Create an output vector segment.                                */
/* -------------------------------------------------------------------- */
        int vec_seg_number = 
            dst_file->CreateSegment( "VECWORK", "Test output vectors",
                                     SEG_VEC, 0L );

        PCIDSKSegment *seg = dst_file->GetSegment( vec_seg_number );
        PCIDSKVectorSegment *vec = dynamic_cast<PCIDSKVectorSegment*>( seg );

/* -------------------------------------------------------------------- */
/*      check empty works ok.                                           */
/* -------------------------------------------------------------------- */

        assert( vec->GetFieldCount() == 0 );

        ShapeId id = vec->FindFirst();

        assert( id == NullShapeId );

/* -------------------------------------------------------------------- */
/*      Add one field.                                                  */
/* -------------------------------------------------------------------- */
        std::vector<double> dparms;

        vec->SetProjection( "UTM 11 D000", dparms );

        vec->AddField( "ID", FieldTypeInteger, "Key", "%d" );
        vec->AddField( "FIELD1", FieldTypeString, "Dummy Description", "" );

        dparms.push_back( 6370000 );
        dparms.push_back( 6375000.2 );

        vec->SetProjection( "LONG D000", dparms );

        ShapeField area_default;
        area_default.SetValue( (double) -1.0 );
        vec->AddField( "AREA", FieldTypeDouble, "", "%.2f", &area_default );

/* -------------------------------------------------------------------- */
/*      Add two empty records.                                          */
/* -------------------------------------------------------------------- */
        id = vec->CreateShape();

        assert( id == 0 );
        
        id = vec->CreateShape();

        assert( id == 1 );

/* -------------------------------------------------------------------- */
/*      Write vertices.                                                 */
/* -------------------------------------------------------------------- */
        std::vector<ShapeVertex> vertex_list;
        ShapeVertex v;

        v.x = 1000.0;
        v.y = 2000.0;
        v.z = 10;
        
        vertex_list.push_back( v );

        v.x = 1200.0;
        v.y = 1300.0;
        v.z = 20;

        vertex_list.push_back( v );

        vec->SetVertices( id, vertex_list );

/* -------------------------------------------------------------------- */
/*      Write fields.                                                   */
/* -------------------------------------------------------------------- */
        std::vector<ShapeField> field_list;
        
        field_list.resize(3);

        field_list[0].SetValue( 17 );
        field_list[1].SetValue( "Test Value" );
        field_list[2].SetValue( 10000.0 );

        vec->SetFields( id, field_list );

/* -------------------------------------------------------------------- */
/*      Cleanup                                                         */
/* -------------------------------------------------------------------- */
        delete src_file;
        delete dst_file;
    }
/* ==================================================================== */
/*      Catch any exception and report the message.                     */
/* ==================================================================== */

    catch( PCIDSK::PCIDSKException ex )
    {
        fprintf( stderr, "PCIDSKException:\n%s\n", ex.what() );
        exit( 1 );
    }
}
