/******************************************************************************
 * $Id$
 *
 * Project:  GXF Reader
 * Purpose:  GXF-3 access function declarations.
 * Author:   Frank Warmerdam, warmerda@home.com
 *
 ******************************************************************************
 * Copyright (c) 1998, Global Geomatics
 * Copyright (c) 1998, Frank Warmerdam
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
 * Revision 1.7  1999/10/29 17:30:40  warmerda
 * added prototype
 *
 * Revision 1.6  1999/10/27 20:22:33  warmerda
 * Added Doxygen style documentation.
 * Added GXFGetPosition() function.
 *
 * Revision 1.5  1999/01/11 15:33:06  warmerda
 * Added function
 *
 * Revision 1.4  1998/12/15 19:07:40  warmerda
 * Add Close, move Readline, add zmin/max, add readscanline
 *
 * Revision 1.3  1998/12/14 04:51:30  warmerda
 * Added some functions, clarified raw vs. not raw.
 *
 * Revision 1.2  1998/12/06 02:54:26  warmerda
 * new functions
 *
 * Revision 1.1  1998/12/02 19:37:04  warmerda
 * New
 *
 */

#ifndef _GXFOPEN_H_INCLUDED
#define _GXFOPEN_H_INCLUDED

/**
 * \file gxfopen.h
 *
 * Public GXF-3 function definitions.
 */

/* -------------------------------------------------------------------- */
/*      Include standard portability stuff.                             */
/* -------------------------------------------------------------------- */
#include "cpl_conv.h"
#include "cpl_string.h"

CPL_C_START

typedef void *GXFHandle;

GXFHandle GXFOpen( const char * pszFilename );

CPLErr   GXFGetRawInfo( GXFHandle hGXF, int *pnXSize, int *pnYSize,
                        int *pnSense, double * pdfZMin, double * pdfZMax,
                        double * pdfDummy );
CPLErr   GXFGetInfo( GXFHandle hGXF, int *pnXSize, int *pnYSize );

CPLErr   GXFGetRawScanline( GXFHandle, int iScanline, double * padfLineBuf );
CPLErr   GXFGetScanline( GXFHandle, int iScanline, double * padfLineBuf );

char	**GXFGetMapProjection( GXFHandle );
char	**GXFGetMapDatumTransform( GXFHandle );
char	*GXFGetMapProjectionAsPROJ4( GXFHandle );
char	*GXFGetMapProjectionAsOGCWKT( GXFHandle );

CPLErr  GXFGetRawPosition( GXFHandle, double *, double *, double *, double *,
                           double * );
CPLErr  GXFGetPosition( GXFHandle, double *, double *, double *, double *,
                        double * );

CPLErr  GXFGetPROJ4Position( GXFHandle, double *, double *, double *, double *,
                             double * );

void     GXFClose( GXFHandle hGXF );

#define GXFS_LL_UP	-1
#define GXFS_LL_RIGHT	1
#define GXFS_UL_RIGHT	-2
#define GXFS_UL_DOWN	2
#define GXFS_UR_DOWN	-3
#define GXFS_UR_LEFT	3
#define GXFS_LR_LEFT	-4
#define GXFS_LR_UP	4

CPL_C_END

/* -------------------------------------------------------------------- */
/*      This is consider to be a private structure.                     */
/* -------------------------------------------------------------------- */
typedef struct {
    FILE	*fp;

    int		nRawXSize;
    int		nRawYSize;
    int		nSense;		/* GXFS_ codes */
    int		nGType;		/* 0 is uncompressed */

    double	dfXPixelSize;
    double	dfYPixelSize;
    double	dfRotation;
    double	dfXOrigin;	/* lower left corner */
    double	dfYOrigin;	/* lower left corner */

    char	szDummy[64];
    double	dfSetDummyTo;

    char	*pszTitle;

    double	dfTransformScale;
    double	dfTransformOffset;
    char	*pszTransformName;

    char	**papszMapProjection;
    char	**papszMapDatumTransform;

    char	*pszUnitName;
    double	dfUnitToMeter;
    

    double	dfZMaximum;
    double	dfZMinimum;

    long	*panRawLineOffset;
    
} GXFInfo_t;

#endif /* ndef _GXFOPEN_H_INCLUDED */
