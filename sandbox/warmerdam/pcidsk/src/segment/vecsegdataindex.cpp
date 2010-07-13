/******************************************************************************
 *
 * Purpose:  Implementation of the VecSegIndex class.  
 *
 * This class is used to manage a vector segment data block index.  There
 * will be two instances created, one for the record data (sec_record) and
 * one for the vertices (sec_vert).  This class is exclusively a private
 * helper class for VecSegHeader.
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
#include "core/pcidsk_utils.h"
#include "segment/cpcidskvectorsegment.h"
#include <cassert>
#include <cstring>

using namespace PCIDSK;

/************************************************************************/
/*                          VecSegDataIndex()                           */
/************************************************************************/

VecSegDataIndex::VecSegDataIndex()

{
    block_initialized = false;
    vs = NULL;
    dirty = false;
}

/************************************************************************/
/*                          ~VecSegDataIndex()                          */
/************************************************************************/

VecSegDataIndex::~VecSegDataIndex()

{
}

/************************************************************************/
/*                             Initialize()                             */
/************************************************************************/

void VecSegDataIndex::Initialize( CPCIDSKVectorSegment *vs, int section )

{
    this->section = section;
    this->vs = vs;

    uint32 offset = vs->di[section].GetLocation();

    memcpy( &block_count, vs->GetData(sec_raw,offset,NULL,4), 4);
    memcpy( &bytes, vs->GetData(sec_raw,offset+4,NULL,4), 4);

    bool needs_swap = !BigEndianSystem();

    if( needs_swap )
    {
        SwapData( &block_count, 4, 1 );
        SwapData( &bytes, 4, 1 );
    }
}

/************************************************************************/
/*                            GetLocation()                             */
/*                                                                      */
/*      Get the offset within the segment header where this index is    */
/*      kept.                                                           */
/************************************************************************/

uint32 VecSegDataIndex::GetLocation()

{
    if( section == sec_vert )
        return vs->vh.section_offsets[hsec_shape];
    else 
    {
        assert( section == sec_record );
        return vs->vh.section_offsets[hsec_shape]
            + vs->di[sec_vert].SerializedSize();
    }
}

/************************************************************************/
/*                           SerializedSize()                           */
/************************************************************************/

uint32 VecSegDataIndex::SerializedSize()

{
    return 8 + 4 * block_count;
}

/************************************************************************/
/*                           GetBlockIndex()                            */
/************************************************************************/

const std::vector<uint32> *VecSegDataIndex::GetIndex()

{
/* -------------------------------------------------------------------- */
/*      Load block map if needed.                                       */
/* -------------------------------------------------------------------- */
    if( !block_initialized )
    {
        block_index.resize( block_count );
        vs->ReadFromFile( &(block_index[0]), GetLocation() + 8, 
                          4 * block_count );

        bool needs_swap = !BigEndianSystem();

        if( needs_swap )
            SwapData( &(block_index[0]), 4, block_count );

        block_initialized = true;
    }

    return &block_index;
}
                             
/************************************************************************/
/*                               Flush()                                */
/************************************************************************/

void VecSegDataIndex::Flush()

{
    if( !dirty )
        return;

    GetIndex(); // force loading if not already loaded!

    uint32 offset = GetLocation();
    PCIDSKBuffer wbuf( SerializedSize() );

    memcpy( wbuf.buffer + 0, &block_count, 4 );
    memcpy( wbuf.buffer + 4, &bytes, 4 );
    memcpy( wbuf.buffer + 8, &(block_index[0]), 4*block_count );

    bool needs_swap = !BigEndianSystem();

    if( needs_swap )
        SwapData( wbuf.buffer, 4, block_count+2 );
    
    vs->WriteToFile( wbuf.buffer, offset, wbuf.buffer_size );

    dirty = false;
}

/************************************************************************/
/*                           GetSectionEnd()                            */
/************************************************************************/

uint32 VecSegDataIndex::GetSectionEnd()

{
    return bytes;
}

/************************************************************************/
/*                           SetSectionEnd()                            */
/************************************************************************/

void VecSegDataIndex::SetSectionEnd( uint32 new_end )

{
    // should we keep track of the need to write this back to disk?
    bytes = new_end;
}

/************************************************************************/
/*                          AddBlockToIndex()                           */
/************************************************************************/

void VecSegDataIndex::AddBlockToIndex( uint32 block )

{
    GetIndex(); // force loading.
        
    block_index.push_back( block );
    block_count++;
    dirty = true;
}

/************************************************************************/
/*                              SetDirty()                              */
/*                                                                      */
/*      This method is primarily used to mark the need to write the     */
/*      index when the location changes.                                */
/************************************************************************/

void VecSegDataIndex::SetDirty()

{
    dirty = true;
}
