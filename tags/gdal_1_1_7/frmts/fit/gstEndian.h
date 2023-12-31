/******************************************************************************
 * $Id$
 *
 * Project:  FIT Driver
 * Purpose:  Implement FIT Support - not using the SGI iflFIT library.
 * Author:   Philip Nemec, nemec@keyholecorp.com
 *
 ******************************************************************************
 * Copyright (c) 2001, Keyhole, Inc.
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
 ******************************************************************************
 * 
 * $Log$
 * Revision 1.2  2001/07/06 18:46:25  nemec
 * Cleanup files - improve Windows build, make proper copyright notice
 *
 *
 */

#ifndef _gstEndian_h_
#define _gstEndian_h_

// endian swapping tools

#include <stdio.h>
#include <cpl_port.h>

#include "gstTypes.h"

// have to do swapping on Linux and Windows
#ifdef CPL_LSB
#define swapping
#else
#endif

#ifdef swapping
size_t swapped_fread(void *ptr, size_t size, size_t nitems, FILE *stream);
size_t swapped_fwrite(const void *ptr, size_t size, size_t nitems, FILE
                      *stream);

static inline void swap64(void * value)
{
    // 0x1122334455667788 --> 0x8877665544332211

	*(uint64 *)(value) =
		   ( ((*(uint64 *)(value) & 0x00000000000000ff) << 56) |
        	 ((*(uint64 *)(value) & 0x000000000000ff00) << 40)  |
        	 ((*(uint64 *)(value) & 0x0000000000ff0000) << 24)  |
        	 ((*(uint64 *)(value) & 0x00000000ff000000) << 8)  |
        	 ((*(uint64 *)(value) >> 8) & 0x00000000ff000000)  |
        	 ((*(uint64 *)(value) >> 24) & 0x0000000000ff0000)  |
        	 ((*(uint64 *)(value) >> 40) & 0x000000000000ff00)  |
        	 ((*(uint64 *)(value) >> 56) & 0x00000000000000ff) );
}

static inline void swap32(void * value)
{
    // 0x12 34 56 78 --> 0x78 56 34 12

	*(uint32 *)(value) =
	       ( ((*(uint32 *)(value) & 0x000000ff) << 24) |
        	 ((*(uint32 *)(value) & 0x0000ff00) << 8)  |
        	 ((*(uint32 *)(value) >> 8) & 0x0000ff00)  |
        	 ((*(uint32 *)(value) >> 24) & 0x000000ff) );
}

static inline void swap16(void * value)
{
    *(uint16 *)(value) = 
		   ( ((*(uint16 *)(value) & 0x00ff) << 8) |
             ((*(uint16 *)(value) >> 8) & 0x00ff) );
}

static inline void swapbytes(void * value, int size)
{
    switch (size) {
    case 1:
        // do nothing
        break;
    case 2:
        swap16(value);
        break;
    case 4:
        swap32(value);
        break;
    case 8:
        swap64(value);
        break;
    default:
        fprintf(stderr, "swapbytes unsupported size %i - not swapping\n",
                size);
        break;
    } // switch
}

#define swapb( value )  swapbytes( &value, sizeof(value))

#else // swapping

#define swapped_fread(ptr, size, nitems, stream) \
	fread(ptr, size, nitems, stream)
#define swapped_fwrite(ptr, size, nitems, stream) \
	fwrite(ptr, size, nitems, stream)

#define swap64( vlaue )
#define swap32( vlaue )
#define swap16( vlaue )
#define swapbytes( value, size )
#define swapb( value )

#endif // swapping

#endif // ! _gstEndian_h_
