/******************************************************************************
 *
 * Purpose:  Implementation of the CPCIDSKVectorSegment class.
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

#include "pcidsk_exception.h"
#include "core/pcidsk_utils.h"
#include "segment/cpcidskvectorsegment.h"
#include <cassert>
#include <cstring>

#define CHUNK_SIZE 8192

using namespace PCIDSK;

/************************************************************************/
/*                        CPCIDSKVectorSegment()                        */
/************************************************************************/

CPCIDSKVectorSegment::CPCIDSKVectorSegment( PCIDSKFile *file, int segment,
                                            const char *segment_pointer )
        : CPCIDSKSegment( file, segment, segment_pointer )

{
    base_initialized = false;
    
    last_shapes_id = NullShapeId;
    last_shapes_index = -1;

    raw_loaded_data_offset = 0;
    vert_loaded_data_offset = 0;
    record_loaded_data_offset = 0;

    Initialize();
}

/************************************************************************/
/*                       ~CPCIDSKVectorSegment()                        */
/************************************************************************/

CPCIDSKVectorSegment::~CPCIDSKVectorSegment()

{
}

/************************************************************************/
/*                             Initialize()                             */
/*                                                                      */
/*      Initialize minimum information from the vector segment          */
/*      header.  We defer this till an actual vector related action     */
/*      is taken.                                                       */
/************************************************************************/

void CPCIDSKVectorSegment::Initialize()

{
    if( base_initialized )
        return;

    fprintf( stderr, "Initialize Vector Segment\n" );

    base_initialized = true;

    // vector segment data is always big endian. 

    needs_swap = !BigEndianSystem();

/* -------------------------------------------------------------------- */
/*      Check fixed portion of the header to ensure this is a V6        */
/*      style vector segment.                                           */
/* -------------------------------------------------------------------- */
    static const unsigned char magic[24] = 
        { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
          0, 0, 0, 21, 0, 0, 0, 4, 0, 0, 0, 19, 0, 0, 0, 69 };

    if( memcmp( GetData( sec_raw, 0, NULL, 24 ), magic, 24 ) != 0 )
    {
        ThrowPCIDSKException( "Unexpected vector header values, possibly it is not a V6 vector segment?" );
    }
    
/* -------------------------------------------------------------------- */
/*      Load section offsets.                                           */
/* -------------------------------------------------------------------- */
    memcpy( section_offsets, GetData( sec_raw, 72, NULL, 16 ), 16 );
    if( needs_swap )
        SwapData( section_offsets, 4, 4 );

/* -------------------------------------------------------------------- */
/*      Fetch the vertex block basics.                                  */
/* -------------------------------------------------------------------- */
    uint32 next_off = section_offsets[3];
    vertex_block_initialized = false;

    memcpy( &vertex_block_count, GetData(sec_raw,next_off,NULL,4), 4);
    memcpy( &vertex_bytes, GetData(sec_raw,next_off+4,NULL,4), 4);

    if( needs_swap )
    {
        SwapData( &vertex_block_count, 4, 1 );
        SwapData( &vertex_bytes, 4, 1 );
    }

    next_off += 8 + 4 * vertex_block_count;

/* -------------------------------------------------------------------- */
/*      Fetch the record block basics.                                  */
/* -------------------------------------------------------------------- */
    record_block_initialized = false;

    memcpy( &record_block_count, GetData(sec_raw,next_off,NULL,4), 4);
    memcpy( &record_bytes, GetData(sec_raw,next_off+4,NULL,4), 4);

    if( needs_swap )
    {
        SwapData( &record_block_count, 4, 1 );
        SwapData( &record_bytes, 4, 1 );
    }

    next_off += 8 + 4 * record_block_count;

/* -------------------------------------------------------------------- */
/*      Fetch the shapeid basics.                                       */
/* -------------------------------------------------------------------- */
    memcpy( &shape_count, GetData(sec_raw,next_off,NULL,4), 4);
    if( needs_swap )
        SwapData( &shape_count, 4, 1 );

    next_off += 4;
    shape_index_byte_offset = next_off;
    
    shape_index_start = 0;
}

/************************************************************************/
/*                              GetData()                               */
/************************************************************************/

char *CPCIDSKVectorSegment::GetData( int section, uint32 offset, 
                                     int *bytes_available, int min_bytes )

{
    if( min_bytes == 0 )
        min_bytes = 1;

/* -------------------------------------------------------------------- */
/*      Select the section to act on.                                   */
/* -------------------------------------------------------------------- */
    PCIDSKBuffer *pbuf;
    uint32       *pbuf_offset;

    if( section == sec_raw )
    {
        pbuf = &raw_loaded_data;
        pbuf_offset = &raw_loaded_data_offset;
    }
    else if( section == sec_vert )
    {
        pbuf = &vert_loaded_data;
        pbuf_offset = &vert_loaded_data_offset;
    }
    else if( section == sec_record )
    {
        pbuf = &record_loaded_data;
        pbuf_offset = &record_loaded_data_offset;
    }

/* -------------------------------------------------------------------- */
/*      If the desired data is not within our loaded section, reload    */
/*      one or more blocks around the request.                          */
/* -------------------------------------------------------------------- */
    if( offset < *pbuf_offset
        || offset+min_bytes > *pbuf_offset + pbuf->buffer_size )
    {
        // read whole 8K blocks around the target region.
        uint32 load_offset = offset - (offset % CHUNK_SIZE);
        int size = (offset + min_bytes - load_offset + CHUNK_SIZE - 1);
        
        size -= (size % CHUNK_SIZE);

        *pbuf_offset = load_offset;
        pbuf->SetSize( size );

        ReadSecFromFile( section, pbuf->buffer, 
                         load_offset / CHUNK_SIZE, size / CHUNK_SIZE );
    }

/* -------------------------------------------------------------------- */
/*      Return desired info.                                            */
/* -------------------------------------------------------------------- */
    if( bytes_available != NULL )
        *bytes_available = *pbuf_offset + pbuf->buffer_size - offset;

    return pbuf->buffer + offset - *pbuf_offset;
}

/************************************************************************/
/*                          ReadSecFromFile()                           */
/*                                                                      */
/*      Read one or more blocks from the desired "section" of the       */
/*      segment data, going through the block pointer map for           */
/*      vect/record sections.                                           */
/************************************************************************/

void CPCIDSKVectorSegment::ReadSecFromFile( int section, char *buffer, 
                                            int block_offset, 
                                            int block_count )

{
/* -------------------------------------------------------------------- */
/*      Raw is a simple case, directly gulp.                            */
/* -------------------------------------------------------------------- */
    if( section == sec_raw )
    {
        ReadFromFile( buffer, block_offset*CHUNK_SIZE, block_count*CHUNK_SIZE );
        return;
    }

/* -------------------------------------------------------------------- */
/*      Load vertex block map if needed.                                */
/* -------------------------------------------------------------------- */
    if( section == sec_vert && !vertex_block_initialized )
    {
        vertex_block_index.resize( vertex_block_count );
        ReadFromFile( &(vertex_block_index[0]), 
                      section_offsets[3] + 8,
                      4 * vertex_block_count );
        if( needs_swap )
            SwapData( &(vertex_block_index[0]), 4, vertex_block_count );

        vertex_block_initialized = true;
    }

/* -------------------------------------------------------------------- */
/*      Load record block map if needed.                                */
/* -------------------------------------------------------------------- */
    if( section == sec_record && !record_block_initialized )
    {
        record_block_index.resize( record_block_count );
        ReadFromFile( &(record_block_index[0]), 
                      section_offsets[3] + 16 + 4*vertex_block_count,
                      4 * record_block_count );
        if( needs_swap )
            SwapData( &(record_block_index[0]), 4, record_block_count );

        record_block_initialized = true;
    }

/* -------------------------------------------------------------------- */
/*      Process one 8K block at a time in case they are discontigous    */
/*      which they often are.                                           */
/* -------------------------------------------------------------------- */
    int i;
    std::vector<uint32> *block_map;

    if( section == sec_vert )
        block_map = &vertex_block_index;
    else
        block_map = &record_block_index;

    for( i = 0; i < block_count; i++ )
    {
        ReadFromFile( buffer + i * CHUNK_SIZE, 
                      CHUNK_SIZE * (*block_map)[block_offset+i], 
                      CHUNK_SIZE );
    }
}

/************************************************************************/
/*                          IndexFromShapeId()                          */
/*                                                                      */
/*      Translate a shapeid into a shape index.  Several mechanisms     */
/*      are used to accelerate this when possible.                      */
/************************************************************************/

int CPCIDSKVectorSegment::IndexFromShapeId( ShapeId id )

{
    if( id == NullShapeId )
        return -1;

/* -------------------------------------------------------------------- */
/*      Does this match our last lookup?                                */
/* -------------------------------------------------------------------- */
    if( id == last_shapes_id )
        return last_shapes_index;

/* -------------------------------------------------------------------- */
/*      Is this the next shapeid in sequence, and is it in our          */
/*      loaded index cache?                                             */
/* -------------------------------------------------------------------- */
    if( id == last_shapes_id + 1 
        && last_shapes_index + 1 >= shape_index_start
        && last_shapes_index + 1 < shape_index_start + (int) shape_index_ids.size() )
    {
        last_shapes_index++;
        last_shapes_id++;
        return last_shapes_index;
    }

/* -------------------------------------------------------------------- */
/*      Put off the rest for a bit.                                     */
/* -------------------------------------------------------------------- */
    ThrowPCIDSKException( "generalized shapeid lookup not yet implemented." );

    return -1;
}

/************************************************************************/
/*                         AccessShapeByIndex()                         */
/*                                                                      */
/*      This method is responsible for loading the set of               */
/*      information for shape "shape_index" into the shape_index data   */
/*      structures if it is not already there.                          */
/************************************************************************/

void CPCIDSKVectorSegment::AccessShapeByIndex( int shape_index )

{
/* -------------------------------------------------------------------- */
/*      Is the requested index already loaded?                          */
/* -------------------------------------------------------------------- */
    if( shape_index >= shape_index_start
        && shape_index < shape_index_start + (int) shape_index_ids.size() )
        return;

/* -------------------------------------------------------------------- */
/*      Load a chunk of shape index information into a                  */
/*      PCIDSKBuffer.                                                   */
/* -------------------------------------------------------------------- */
    const static int shapeid_page = 1024; // shapes in a bunch.
    int entries_to_load = shapeid_page;

    shape_index_start = shape_index - (shape_index % shapeid_page);
    if( shape_index_start + entries_to_load > shape_count )
        entries_to_load = shape_count - shape_index_start;

    PCIDSKBuffer wrk_index;
    wrk_index.SetSize( entries_to_load * 12 );
    
    ReadFromFile( wrk_index.buffer, 
                  shape_index_byte_offset + shape_index_start*12,
                  wrk_index.buffer_size );

/* -------------------------------------------------------------------- */
/*      Parse into the vectors for easier use.                          */
/* -------------------------------------------------------------------- */
    int i;

    shape_index_ids.resize( entries_to_load );
    shape_index_vertex_off.resize( entries_to_load );
    shape_index_record_off.resize( entries_to_load );

    for( i = 0; i < entries_to_load; i++ )
    {
        memcpy( &(shape_index_ids[i]), wrk_index.buffer + i*12, 4 );
        memcpy( &(shape_index_vertex_off[i]), wrk_index.buffer + i*12+4, 4 );
        memcpy( &(shape_index_record_off[i]), wrk_index.buffer + i*12+8, 4 );
    }

    if( needs_swap )
    {
        SwapData( &(shape_index_ids[0]), 4, entries_to_load );
        SwapData( &(shape_index_vertex_off[0]), 4, entries_to_load );
        SwapData( &(shape_index_record_off[0]), 4, entries_to_load );
    }
}

/************************************************************************/
/*                             FindFirst()                              */
/************************************************************************/

ShapeId CPCIDSKVectorSegment::FindFirst()
{ 
    if( shape_count == 0 )
        return NullShapeId;

    AccessShapeByIndex( 0 );

    last_shapes_id = shape_index_ids[0];
    last_shapes_index = 0;

    return last_shapes_id;
}

/************************************************************************/
/*                              FindNext()                              */
/************************************************************************/

ShapeId CPCIDSKVectorSegment::FindNext( ShapeId previous_id )
{ 
    if( previous_id == NullShapeId ) 
        return FindFirst();

    int previous_index = IndexFromShapeId( previous_id );
    
    if( previous_index == shape_count - 1 )
        return NullShapeId;

    AccessShapeByIndex( previous_index+1 );

    last_shapes_index = previous_index+1;
    last_shapes_id = shape_index_ids[last_shapes_index - shape_index_start];
    
    return last_shapes_id;
}

/************************************************************************/
/*                            GetVertices()                             */
/************************************************************************/

void CPCIDSKVectorSegment::GetVertices( ShapeId shape_id,
                                        std::vector<ShapeVertex> &vertices )

{
    int shape_index = IndexFromShapeId( shape_id );

    AccessShapeByIndex( shape_index );

    uint32 vert_off = shape_index_vertex_off[shape_index - shape_index_start];
    uint32 vertex_count;

    memcpy( &vertex_count, GetData( sec_vert, vert_off+4, NULL, 4 ), 4 );
    if( needs_swap )
        SwapData( &vertex_count, 4, 1 );

    vertices.resize( vertex_count );
    
    // We ought to change this to process the available data and
    // then request more. 
    memcpy( &(vertices[0]), 
            GetData( sec_vert, vert_off+8, NULL, vertex_count*24),
            vertex_count * 24 );
    if( needs_swap )
        SwapData( &(vertices[0]), 8, vertex_count*3 );
}

/************************************************************************/
/*                            GetRingStart()                            */
/************************************************************************/

void CPCIDSKVectorSegment::GetRingStart( ShapeId nShapeId, 
                                         std::vector<int32> &ring_start )

{
    ThrowPCIDSKException( "GetRingStart() not implemented." );
}
