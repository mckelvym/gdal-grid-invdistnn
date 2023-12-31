/******************************************************************************
 * $Id$
 *
 * Project:  GeoTIFF Driver
 * Purpose:  Implement system hook functions for libtiff on top of CPL/VSI,
 *           including > 2GB support.  Based on tif_unix.c from libtiff
 *           distribution.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2005, Frank Warmerdam, warmerdam@pobox.com
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

/*
 * TIFF Library UNIX-specific Routines.
 */
#include "tiffio.h"
#include "cpl_vsi.h"

// We avoid including xtiffio.h since it drags in the libgeotiff version
// of the VSI functions.

CPL_C_START
extern TIFF CPL_DLL * XTIFFClientOpen(const char* name, const char* mode, 
                                      thandle_t thehandle,
                                      TIFFReadWriteProc, TIFFReadWriteProc,
                                      TIFFSeekProc, TIFFCloseProc,
                                      TIFFSizeProc,
                                      TIFFMapFileProc, TIFFUnmapFileProc);
CPL_C_END

static tsize_t
_tiffReadProc(thandle_t fd, tdata_t buf, tsize_t size)
{
    return VSIFReadL( buf, 1, size, (FILE *) fd );
}

static tsize_t
_tiffWriteProc(thandle_t fd, tdata_t buf, tsize_t size)
{
    return VSIFWriteL( buf, 1, size, (FILE *) fd );
}

static toff_t
_tiffSeekProc(thandle_t fd, toff_t off, int whence)
{
    if( VSIFSeekL( (FILE *) fd, off, whence ) == 0 )
        return (toff_t) VSIFTellL( (FILE *) fd );
    else
        return (toff_t) -1;
}

static int
_tiffCloseProc(thandle_t fd)
{
    return VSIFCloseL( (FILE *) fd );
}

static toff_t
_tiffSizeProc(thandle_t fd)
{
    vsi_l_offset  old_off;
    toff_t        file_size;

    old_off = VSIFTellL( (FILE *) fd );
    VSIFSeekL( (FILE *) fd, 0, SEEK_END );
    
    file_size = (toff_t) VSIFTellL( (FILE *) fd );
    VSIFSeekL( (FILE *) fd, old_off, SEEK_SET );

    return file_size;
}

static int
_tiffMapProc(thandle_t fd, tdata_t* pbase, toff_t* psize)
{
	(void) fd; (void) pbase; (void) psize;
	return (0);
}

static void
_tiffUnmapProc(thandle_t fd, tdata_t base, toff_t size)
{
	(void) fd; (void) base; (void) size;
}

/*
 * Open a TIFF file for read/writing.
 */
TIFF* VSI_TIFFOpen(const char* name, const char* mode)
{
    static const char module[] = "TIFFOpen";
    int           i, a_out;
    char          access[32];
    FILE          *fp;
    TIFF          *tif;

    a_out = 0;
    access[0] = '\0';
    for( i = 0; mode[i] != '\0'; i++ )
    {
        if( mode[i] == 'r'
            || mode[i] == 'w'
            || mode[i] == '+'
            || mode[i] == 'a' )
        {
            access[a_out++] = mode[i];
            access[a_out] = '\0';
        }
    }

    strcat( access, "b" );
                    
    fp = VSIFOpenL( name, access );
    if (fp == NULL) {
        if( errno >= 0 )
            TIFFError(module,"%s: %s", name, VSIStrerror( errno ) );
        else
            TIFFError(module, "%s: Cannot open", name);
        return ((TIFF *)0);
    }

    tif = XTIFFClientOpen(name, mode,
                          (thandle_t) fp,
                          _tiffReadProc, _tiffWriteProc,
                          _tiffSeekProc, _tiffCloseProc, _tiffSizeProc,
                          _tiffMapProc, _tiffUnmapProc);

    if( tif == NULL )
        VSIFCloseL( fp );
        
    return tif;
}
