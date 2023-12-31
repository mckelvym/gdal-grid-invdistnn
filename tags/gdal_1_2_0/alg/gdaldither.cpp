/******************************************************************************
 * $Id$
 *
 * Project:  CIETMap Phase 2
 * Purpose:  Convert RGB (24bit) to a pseudo-colored approximation using
 *           Floyd-Steinberg dithering (error diffusion). 
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2001, Frank Warmerdam
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
 * Notes:
 *
 * [1] Floyd-Steinberg dither:
 *  I should point out that the actual fractions we used were, assuming
 *  you are at X, moving left to right:
 *
 *		    X     7/16
 *	     3/16   5/16  1/16    
 *
 *  Note that the error goes to four neighbors, not three.  I think this
 *  will probably do better (at least for black and white) than the
 *  3/8-3/8-1/4 distribution, at the cost of greater processing.  I have
 *  seen the 3/8-3/8-1/4 distribution described as "our" algorithm before,
 *  but I have no idea who the credit really belongs to.
 *  -- 
 *					    Lou Steinberg
 *
 * $Log$
 * Revision 1.5  2003/07/08 15:27:47  warmerda
 * avoid warnings
 *
 * Revision 1.4  2003/03/02 05:24:53  warmerda
 * fixed comment
 *
 * Revision 1.3  2003/02/06 04:56:35  warmerda
 * added documentation
 *
 * Revision 1.2  2001/07/18 04:43:13  warmerda
 * added CPL_CVSID
 *
 * Revision 1.1  2001/01/22 22:30:59  warmerda
 * New
 *
 */

#include "gdal_priv.h"
#include "gdal_alg.h"

CPL_CVSID("$Id$");

#define C_LEVELS	32
#define C_SHIFT  	3

static void FindNearestColor( int nColors, int *panPCT, GByte *pabyColorMap );

/************************************************************************/
/*                         GDALDitherRGB2PCT()                          */
/************************************************************************/

/**
 * 24bit to 8bit conversion with dithering.
 *
 * This functions utilizes Floyd-Steinbert dithering in the process of 
 * converting a 24bit RGB image into a pseudocolored 8bit image using a
 * provided color table.  
 *
 * The red, green and blue input bands do not necessarily need to come
 * from the same file, but they must be the same width and height.  They will
 * be clipped to 8bit during reading, so non-eight bit bands are generally
 * inappropriate.  Likewise the hTarget band will be written with 8bit values
 * and must match the width and height of the source bands. 
 *
 * @param hRed Red input band. 
 * @param hGreen Green input band. 
 * @param hBlue Blue input band. 
 * @param hTarget Output band. 
 * @param hColorTable the color table to use with the output band. 
 * @param pfnProgress callback for reporting algorithm progress matching the
 * GDALProgressFunc() semantics.  May be NULL.
 * @param pProgressArg callback argument passed to pfnProgress.
 *
 * @return CE_None on success or CE_Failure if an error occurs. 
 */

int GDALDitherRGB2PCT( GDALRasterBandH hRed, 
                       GDALRasterBandH hGreen, 
                       GDALRasterBandH hBlue, 
                       GDALRasterBandH hTarget, 
                       GDALColorTableH hColorTable,
                       GDALProgressFunc pfnProgress, 
                       void * pProgressArg )

{
    int		nXSize, nYSize;
    
/* -------------------------------------------------------------------- */
/*      Validate parameters.                                            */
/* -------------------------------------------------------------------- */
    nXSize = GDALGetRasterBandXSize( hRed );
    nYSize = GDALGetRasterBandYSize( hRed );

    if( GDALGetRasterBandXSize( hGreen ) != nXSize 
        || GDALGetRasterBandYSize( hGreen ) != nYSize 
        || GDALGetRasterBandXSize( hBlue ) != nXSize 
        || GDALGetRasterBandYSize( hBlue ) != nYSize )
    {
        CPLError( CE_Failure, CPLE_IllegalArg,
                  "Green or blue band doesn't match size of red band.\n" );

        return CE_Failure;
    }

    if( GDALGetRasterBandXSize( hTarget ) != nXSize 
        || GDALGetRasterBandYSize( hTarget ) != nYSize )
    {
        CPLError( CE_Failure, CPLE_IllegalArg,
                  "GDALDitherRGB2PCT(): "
                  "Target band doesn't match size of source bands.\n" );

        return CE_Failure;
    }

    if( pfnProgress == NULL )
        pfnProgress = GDALDummyProgress;

/* -------------------------------------------------------------------- */
/*      Setup more direct colormap.                                     */
/* -------------------------------------------------------------------- */
    int		nColors, anPCT[768], iColor;

    nColors = GDALGetColorEntryCount( hColorTable );
    for( iColor = 0; iColor < nColors; iColor++ )
    {
        GDALColorEntry	sEntry;

        GDALGetColorEntryAsRGB( hColorTable, iColor, &sEntry );
        
        anPCT[iColor    ] = sEntry.c1;
        anPCT[iColor+256] = sEntry.c2;
        anPCT[iColor+512] = sEntry.c3;
    }
    
/* -------------------------------------------------------------------- */
/*      Build a 24bit to 8 bit color mapping.                           */
/* -------------------------------------------------------------------- */
    GByte	*pabyColorMap;

    pabyColorMap = (GByte *) CPLMalloc(C_LEVELS * C_LEVELS * C_LEVELS 
                                       * sizeof(int));
    
    FindNearestColor( nColors, anPCT, pabyColorMap );

/* -------------------------------------------------------------------- */
/*      Setup various variables.                                        */
/* -------------------------------------------------------------------- */
    GByte	*pabyRed, *pabyGreen, *pabyBlue, *pabyIndex;
    int		*panError;

    pabyRed = (GByte *) CPLMalloc(nXSize);
    pabyGreen = (GByte *) CPLMalloc(nXSize);
    pabyBlue = (GByte *) CPLMalloc(nXSize);

    pabyIndex = (GByte *) CPLMalloc(nXSize);

    panError = (int *) CPLCalloc(sizeof(int),(nXSize+2) * 3);

/* ==================================================================== */
/*      Loop over all scanlines of data to process.                     */
/* ==================================================================== */
    int		iScanline;

    for( iScanline = 0; iScanline < nYSize; iScanline++ )
    {
        int	nLastRedError, nLastGreenError, nLastBlueError, i;

/* -------------------------------------------------------------------- */
/*      Report progress                                                 */
/* -------------------------------------------------------------------- */
        if( !pfnProgress( iScanline / (double) nYSize, NULL, pProgressArg ) )
        {
            CPLFree( pabyRed );
            CPLFree( pabyGreen );
            CPLFree( pabyBlue );
            CPLFree( panError );
            CPLFree( pabyIndex );
            CPLFree( pabyColorMap );

            CPLError( CE_Failure, CPLE_UserInterrupt, "User Terminated" );
            return CE_Failure;
        }

/* -------------------------------------------------------------------- */
/*      Read source data.                                               */
/* -------------------------------------------------------------------- */
        GDALRasterIO( hRed, GF_Read, 0, iScanline, nXSize, 1, 
                      pabyRed, nXSize, 1, GDT_Byte, 0, 0 );
        GDALRasterIO( hGreen, GF_Read, 0, iScanline, nXSize, 1, 
                      pabyGreen, nXSize, 1, GDT_Byte, 0, 0 );
        GDALRasterIO( hBlue, GF_Read, 0, iScanline, nXSize, 1, 
                      pabyBlue, nXSize, 1, GDT_Byte, 0, 0 );

/* -------------------------------------------------------------------- */
/*	Apply the error from the previous line to this one.		*/
/* -------------------------------------------------------------------- */
        for( i = 0; i < nXSize; i++ )
        {
            pabyRed[i] = (GByte)
                MAX(0,MIN(255,(pabyRed[i]   + panError[i*3+0+3])));
            pabyGreen[i] = (GByte)
                MAX(0,MIN(255,(pabyGreen[i] + panError[i*3+1+3])));
            pabyBlue[i] =  (GByte)
                MAX(0,MIN(255,(pabyBlue[i]  + panError[i*3+2+3])));
        }

        memset( panError, 0, sizeof(int) * (nXSize+2) * 3 );

/* -------------------------------------------------------------------- */
/*	Figure out the nearest color to the RGB value.			*/
/* -------------------------------------------------------------------- */
        nLastRedError = 0;
        nLastGreenError = 0;
        nLastBlueError = 0;

        for( i = 0; i < nXSize; i++ )
        {
            int		iIndex, nError, nSixth, iRed, iGreen, iBlue;
            int		nRedValue, nGreenValue, nBlueValue;

            nRedValue =   MAX(0,MIN(255, pabyRed[i]   + nLastRedError));
            nGreenValue = MAX(0,MIN(255, pabyGreen[i] + nLastGreenError));
            nBlueValue =  MAX(0,MIN(255, pabyBlue[i]  + nLastBlueError));

            iRed   = nRedValue *   C_LEVELS   / 256;
            iGreen = nGreenValue * C_LEVELS / 256;
            iBlue  = nBlueValue *  C_LEVELS  / 256;
            
            iIndex = pabyColorMap[iRed + iGreen * C_LEVELS 
                                 + iBlue * C_LEVELS * C_LEVELS];
	
            pabyIndex[i] = (GByte) iIndex;

/* -------------------------------------------------------------------- */
/*      Compute Red error, and carry it on to the next error line.      */
/* -------------------------------------------------------------------- */
            nError = nRedValue - anPCT[iIndex    ];
            nSixth = nError / 6;
            
            panError[i*3    ] += nSixth;
            panError[i*3+6  ] = nSixth;
            panError[i*3+3  ] += nError - 5 * nSixth;
            
            nLastRedError = 2 * nSixth;

/* -------------------------------------------------------------------- */
/*      Compute Green error, and carry it on to the next error line.    */
/* -------------------------------------------------------------------- */
            nError = nGreenValue - anPCT[iIndex+256];
            nSixth = nError / 6;

            panError[i*3  +1] += nSixth;
            panError[i*3+6+1] = nSixth;
            panError[i*3+3+1] += nError - 5 * nSixth;
            
            nLastGreenError = 2 * nSixth;

/* -------------------------------------------------------------------- */
/*      Compute Blue error, and carry it on to the next error line.     */
/* -------------------------------------------------------------------- */
            nError = nBlueValue - anPCT[iIndex+512];
            nSixth = nError / 6;
            
            panError[i*3  +2] += nSixth;
            panError[i*3+6+2] = nSixth;
            panError[i*3+3+2] += nError - 5 * nSixth;
            
            nLastBlueError = 2 * nSixth;
        }

/* -------------------------------------------------------------------- */
/*      Write results.                                                  */
/* -------------------------------------------------------------------- */
        GDALRasterIO( hTarget, GF_Write, 0, iScanline, nXSize, 1, 
                      pabyIndex, nXSize, 1, GDT_Byte, 0, 0 );
    }

/* -------------------------------------------------------------------- */
/*      Cleanup                                                         */
/* -------------------------------------------------------------------- */
    CPLFree( pabyRed );
    CPLFree( pabyGreen );
    CPLFree( pabyBlue );
    CPLFree( pabyIndex );
    CPLFree( panError );

    CPLFree( pabyColorMap );

    pfnProgress( 1.0, NULL, pProgressArg );

    return CE_None;
}

/************************************************************************/
/*                          FindNearestColor()                          */
/*                                                                      */
/*      Finear near PCT color for any RGB color.                        */
/************************************************************************/

static void FindNearestColor( int nColors, int *panPCT, GByte *pabyColorMap )

{
    int		iBlue, iGreen, iRed;
    int		iColor;

/* -------------------------------------------------------------------- */
/*	Loop over all the cells in the high density cube.		*/
/* -------------------------------------------------------------------- */
    for( iBlue = 0; iBlue < C_LEVELS; iBlue++ )
    {
	for( iGreen = 0; iGreen < C_LEVELS; iGreen++ )
	{
	    for( iRed = 0; iRed < C_LEVELS; iRed++ )
	    {
		int  	nRedValue, nGreenValue, nBlueValue;
		int	nBestDist = 768, nBestIndex = 0;

		nRedValue   = (iRed * 255) / (C_LEVELS-1);
		nGreenValue = (iGreen * 255) / (C_LEVELS-1);
		nBlueValue  = (iBlue * 255) / (C_LEVELS-1);

		for( iColor = 0; iColor < nColors; iColor++ )
		{
		    int		nThisDist;

		    nThisDist = ABS(nRedValue   - panPCT[iColor    ]) 
		              + ABS(nGreenValue - panPCT[iColor+256])
		              + ABS(nBlueValue  - panPCT[iColor+512]);

		    if( nThisDist < nBestDist )
		    {
			nBestIndex = iColor;
			nBestDist = nThisDist;
		    }
		}
		
		pabyColorMap[iRed + iGreen*C_LEVELS 
                            + iBlue*C_LEVELS*C_LEVELS] = (GByte)nBestIndex;
	    }
	}
    }
}

