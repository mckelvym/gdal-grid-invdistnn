/* $Header: /cvsroot/osrs/libtiff/libtiff/tif_close.c,v 1.3 2002/04/03 20:13:31 warmerda Exp $ */

/*
 * Copyright (c) 1988-1997 Sam Leffler
 * Copyright (c) 1991-1997 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */

/*
 * TIFF Library.
 */
#include "tiffiop.h"

void
TIFFClose(TIFF* tif)
{
    if (tif->tif_mode != O_RDONLY)
        /*
         * Flush buffered data and directory (if dirty).
         */
        TIFFFlush(tif);
    (*tif->tif_cleanup)(tif);
    TIFFFreeDirectory(tif);
        
    /* Clean up client info links */
    while( tif->tif_clientinfo )
    {
        TIFFClientInfoLink *link = tif->tif_clientinfo;

        tif->tif_clientinfo = link->next;
        _TIFFfree( link->name );
        _TIFFfree( link );
    }

    if (tif->tif_rawdata && (tif->tif_flags&TIFF_MYBUFFER))
        _TIFFfree(tif->tif_rawdata);
    if (isMapped(tif))
        TIFFUnmapFileContents(tif, tif->tif_base, tif->tif_size);
    (void) TIFFCloseFile(tif);
    if (tif->tif_fieldinfo)
        _TIFFfree(tif->tif_fieldinfo);
    _TIFFfree(tif);
}
