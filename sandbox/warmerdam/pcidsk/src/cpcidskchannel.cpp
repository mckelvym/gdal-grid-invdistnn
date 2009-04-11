/******************************************************************************
 *
 * Purpose:  Implementation of the CPCIDSKChannel class.
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
/*                           CPCIDSKChannel()                           */
/************************************************************************/

CPCIDSKChannel::CPCIDSKChannel( PCIDSKBuffer &image_header, 
                                CPCIDSKFile *file )

{
    this->file = file;

    width = file->GetWidth();
    height = file->GetHeight();

    block_width = width;
    block_height = 1;

    const char *pixel_type_string = image_header.Get( 160, 8 );
    
    if( strcmp(pixel_type_string,"8U      ") == 0 )
        pixel_type = CHN_8U;
    else if( strcmp(pixel_type_string,"16S     ") == 0 )
        pixel_type = CHN_16S;
    else if( strcmp(pixel_type_string,"16U     ") == 0 )
        pixel_type = CHN_16U;
    else if( strcmp(pixel_type_string,"32R     ") == 0 )
        pixel_type = CHN_32R;
    else
        pixel_type = CHN_UNKNOWN; // should we throw an exception?  
}

/************************************************************************/
/*                          ~CPCIDSKChannel()                           */
/************************************************************************/

CPCIDSKChannel::~CPCIDSKChannel()

{
}


/************************************************************************/
/*                          GetOverviewCount()                          */
/************************************************************************/

int CPCIDSKChannel::GetOverviewCount()

{
    return 0;
}

/************************************************************************/
/*                            GetOverview()                             */
/************************************************************************/

PCIDSKChannel *CPCIDSKChannel::GetOverview( int overview_index )

{
    return NULL;
}

