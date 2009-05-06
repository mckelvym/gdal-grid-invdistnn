/******************************************************************************
 *
 * Purpose:  Implementation of the MetadataSegment class.
 *
 * This class is used to manage access to the SYS virtual block map segment
 * (named SysBMap).  This segment is used to keep track of one or more 
 * virtual files stored in SysBData segments.  These virtual files are normally
 * used to hold tiled images for primary bands or overviews.  
 *
 * This class is closely partnered with the SysVirtualFile class, and the
 * primary client is the CTiledChannel class. 
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
#include <assert.h>

using namespace PCIDSK;

/************************************************************************/
/*                            SysBlockMap()                             */
/************************************************************************/

MetadataSegment::MetadataSegment( CPCIDSKFile *file, int segment,
                                  const char *segment_pointer )
        : CPCIDSKSegment( file, segment, segment_pointer )

{
    loaded = false;
}

/************************************************************************/
/*                            ~SysBlockMap()                            */
/************************************************************************/

MetadataSegment::~MetadataSegment()

{
}

/************************************************************************/
/*                                Load()                                */
/************************************************************************/

void MetadataSegment::Load()

{
    if( loaded )
        return;

    // TODO: this should likely be protected by a mutex. 

/* -------------------------------------------------------------------- */
/*      Load the segment contents into a buffer.                        */
/* -------------------------------------------------------------------- */
    seg_data.SetSize( data_size - 1024 );

    ReadFromFile( seg_data.buffer, 0, data_size - 1024 );
}

/************************************************************************/
/*                           FetchMetadata()                            */
/************************************************************************/

void MetadataSegment::FetchMetadata( const char *group, int id,
                                     std::map<std::string,std::string> &md_set)

{
/* -------------------------------------------------------------------- */
/*      Load the metadata segment if not already loaded.                */
/* -------------------------------------------------------------------- */
    Load();

/* -------------------------------------------------------------------- */
/*      Establish the key prefix we are searching for.                  */
/* -------------------------------------------------------------------- */
    char key_prefix[200];
    int  prefix_len;

    sprintf( key_prefix, "METADATA_%s_%d_", group, id );
    prefix_len = strlen(key_prefix);

/* -------------------------------------------------------------------- */
/*      Process all the metadata entries in this segment, searching     */
/*      for those that match our prefix.                                */
/* -------------------------------------------------------------------- */
    const char *pszNext;

    for( pszNext = (const char *) seg_data.buffer; *pszNext != '\0'; )
    {
/* -------------------------------------------------------------------- */
/*      Identify the end of this line, and the split character (:).     */
/* -------------------------------------------------------------------- */
        int i_split = -1, i;

        for( i=0; 
             pszNext[i] != 10 && pszNext[i] != 12 && pszNext[i] != 0; 
             i++) 
        {
            if( i_split == -1 && pszNext[i] == ':' )
                i_split = i;
        }

        if( pszNext[i] == '\0' )
            break;

/* -------------------------------------------------------------------- */
/*      If this matches our prefix, capture the key and value.          */
/* -------------------------------------------------------------------- */
        if( i_split != -1 && strncmp(pszNext,key_prefix,prefix_len) == 0 )
        {
            std::string key, value;

            key.assign( pszNext+prefix_len, i_split-prefix_len );
            value.assign( pszNext+i_split+1, i-i_split-1 );

            md_set[key] = value;
        }

/* -------------------------------------------------------------------- */
/*      Advance to start of next line.                                  */
/* -------------------------------------------------------------------- */
        pszNext = pszNext + i;
        while( *pszNext == 10 || *pszNext == 12 )
            pszNext++;
    }
}
