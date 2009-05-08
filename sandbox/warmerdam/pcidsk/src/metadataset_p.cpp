/******************************************************************************
 *
 * Purpose:  Implementation of the MetadataSet class.  This is a container
 *           for a set of metadata, and used by the file, channel and segment
 *           classes to manage metadata for themselves.  It is not public
 *           to SDK users.
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

#include "pcidsk_p.h"

using namespace PCIDSK;

/************************************************************************/
/*                            MetadataSet()                             */
/************************************************************************/

MetadataSet::MetadataSet()


{
    this->id = id;
    this->group = group;
    this->file = file;
    
    loaded = false;
}

/************************************************************************/
/*                            ~MetadataSet()                            */
/************************************************************************/

MetadataSet::~MetadataSet()

{
}

/************************************************************************/
/*                             Initialize()                             */
/************************************************************************/

void MetadataSet::Initialize( CPCIDSKFile *file, const char *group, int id )

{
    this->file = file;
    this->group = group;
    this->id = id;
}

/************************************************************************/
/*                                Load()                                */
/************************************************************************/

void MetadataSet::Load()

{
    if( loaded )
        return;

    if( file == NULL )
        throw new PCIDSKException( "Load() on MetadataSet that is not initialized yet." );

    PCIDSKSegment *seg = file->GetSegment( SEG_SYS , "METADATA");

    if( seg == NULL )
    {
        loaded = true;
        return;
    }

    MetadataSegment *md_seg = dynamic_cast<MetadataSegment *>( seg );

    md_seg->FetchMetadata( group.c_str(), id, md_set );
    loaded = true;
}

/************************************************************************/
/*                          GetMetadataValue()                          */
/************************************************************************/

const char *MetadataSet::GetMetadataValue( const char *key )

{
    std::string o_key = key;

    if( !loaded )
        Load();

    if( md_set.count(o_key) == 0 )
        return NULL;
    else
        return md_set[o_key].c_str();
}

/************************************************************************/
/*                          GetMetadataKeys()                           */
/************************************************************************/

std::vector<std::string> MetadataSet::GetMetadataKeys()

{
    if( !loaded )
        Load();

    std::vector<std::string> keys;
    std::map<std::string,std::string>::iterator it;

    for( it = md_set.begin(); it != md_set.end(); it++ )
    {
        keys.push_back( (*it).first );
    }
         
    return keys;
}


