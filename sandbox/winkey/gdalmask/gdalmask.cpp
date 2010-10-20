/******************************************************************************
 * gdalmask.cpp 
 *
 * python version written by Schuyler, ported to C by Brian Case
 * 
 * ****************************************************************************
 * nearblack functions Copyright (c) 2006, MapShots Inc (www.mapshots.com)
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

#include "gdal.h"
#include "cpl_conv.h"
#include "cpl_string.h"


static void ProcessLine(
    GByte **papabyBufm,
    GByte *pabyMask,
    int iStart,
    int iEnd,
    int nNearDist,
    int nMaxNonBlack,
    int bNearWhite,
    int *panLastLineCounts, 
    int bDoHorizontalCheck, 
    int bDoVerticalCheck
);

void do_regular(
    GDALRasterBandH *pahSrcBands,
    GDALRasterBandH *pahDstBands,
    GDALRasterBandH hMaskBand,
    int bQuiet,
    int bHasAlpha,
    int nXDstSize,
    int nYDstSize
);

void do_nearblack(
    GDALRasterBandH *pahSrcBands,
    GDALRasterBandH *pahDstBands,
    GDALRasterBandH hMaskBand,
    int bQuiet,
    int bNearWhite,
    int bNearBlack,
    int nMaxNonBlack,
    int nNearDist,
    int nXDstSize,
    int nYDstSize
);



/******************************************************************************
 usage
******************************************************************************/


void Usage (void ) {
    printf("gdalmask [-q] [-internal] [-nearwhite] [-nearblack] [-near dist]\n"
           "         [-nb non_black_pixels] [-co \"NAME=VALUE\"]*\n"
           "         <infile> <outfile>\n");
    
    exit( 1 );
    
}

/******************************************************************************
 main
******************************************************************************/

int main( int argc, char ** argv ) {
    
    /***** Check that we are running against at      *****/
    /***** least GDAL 1.8 (probably older in fact !) *****/
    
    /***** Note to developers : if we use newer *****/
    /***** API, please change the requirement   *****/
    
    if (atoi(GDALVersionInfo("VERSION_NUM")) < 1800)
    {
        fprintf(stderr, "At least, GDAL >= 1.8.0 is required for this version "
                "of %s, which was compiled against GDAL %s\n", argv[0],
                GDAL_RELEASE_NAME);
        exit(1);
    }

    /***** get args *****/
    
    int i;
    int bQuiet = FALSE;
    int bNearWhite = FALSE;
    int bNearBlack = FALSE;
    int nMaxNonBlack = 2;
    int nNearDist = 15;
    int bInternal  = FALSE;
    const char *pszOutFile = NULL;
    const char *pszInFile = NULL;
    char** papszCreationOptions = NULL;
    
    for( i = 1; i < argc; i++ ) {

        if( EQUAL(argv[i], "--utility_version") )
        {
            printf("%s was compiled against GDAL %s and is running against "
                   "GDAL %s\n", argv[0], GDAL_RELEASE_NAME,
                   GDALVersionInfo("RELEASE_NAME"));
            return 0;
        }
        
        else if( EQUAL(argv[i], "-q") || EQUAL(argv[i], "-quiet") )
            bQuiet = TRUE;

        else if( EQUAL(argv[i], "-internal") )
            bInternal = TRUE;

        else if( EQUAL(argv[i], "-nearwhite") )
            bNearWhite = TRUE;
        
        else if( EQUAL(argv[i], "-nearblack") )
            bNearBlack = TRUE;

        else if( EQUAL(argv[i], "-nb") && i < argc-1 )
            nMaxNonBlack = atoi(argv[++i]);

        else if( EQUAL(argv[i], "-near") && i < argc-1 )
            nNearDist = atoi(argv[++i]);

        else if( EQUAL(argv[i], "-co") && i < argc-1 )
            papszCreationOptions = CSLAddString(papszCreationOptions, argv[++i]);
        
        else if ( argv[i][0] == '-' )
            Usage();

        else if ( pszInFile == NULL )
            pszInFile = argv[i];

        else if ( pszOutFile == NULL )
            pszOutFile = argv[i];
        
        else
            Usage();
    }

    if( ! pszInFile || ! pszOutFile )
        Usage();

    /***** turn on the internal mask option *****/
    if (bInternal)
        CPLSetConfigOption( "GDAL_TIFF_INTERNAL_MASK", "YES" );
    else
        CPLSetConfigOption( "GDAL_TIFF_INTERNAL_MASK", "NO" );

    /***** reg the drivers *****/

    GDALAllRegister();

    /***** open the input file *****/

    GDALDatasetH hInDS = NULL;

    if (! (hInDS = GDALOpen( pszInFile, GA_ReadOnly )))
        exit( 1 );

    /***** get the x, y, b size of the input raster *****/

    int nXSrcSize = GDALGetRasterXSize( hInDS );
    int nYSrcSize = GDALGetRasterYSize( hInDS );
    int nBands = GDALGetRasterCount( hInDS );

    GDALRasterBandH ahSrcBands[4] = {NULL};
    
    /***** loop over and test input bands *****/

    int bHasRed = FALSE, bHasGreen = FALSE, bHasBlue = FALSE, bHasAlpha = FALSE;
    
    for (int iBand = 0; iBand < nBands; iBand++) {
        GDALRasterBandH hBand = GDALGetRasterBand ( hInDS, iBand + 1 );

        if ( GCI_RedBand == GDALGetRasterColorInterpretation( hBand ) &&
             GDT_Byte == GDALGetRasterDataType ( hBand ) )
        {
            ahSrcBands[0] = hBand;
            bHasRed = TRUE;
        }
        else if (GCI_GreenBand == GDALGetRasterColorInterpretation( hBand ) &&
             GDT_Byte == GDALGetRasterDataType ( hBand ) )
        {
            ahSrcBands[1] = hBand;
            bHasGreen = TRUE;
        }
        else if (GCI_BlueBand == GDALGetRasterColorInterpretation( hBand ) &&
             GDT_Byte == GDALGetRasterDataType ( hBand ) )
        {
            ahSrcBands[2] = hBand;
            bHasBlue = TRUE;
        }
        else if (GCI_AlphaBand == GDALGetRasterColorInterpretation( hBand ) &&
             GDT_Byte == GDALGetRasterDataType ( hBand ) )
        {
            ahSrcBands[3] = hBand;
            bHasAlpha = TRUE;
        }

    }

    /***** ensure we have rgb bands of type byte *****/

    if (bHasRed == FALSE || bHasGreen == FALSE || bHasBlue == FALSE) {
        CPLError(CE_Failure, CPLE_AppDefined,
                 "Input datasource must have red green and blue bands of type"
                 " byte");
        exit( 1 );
    }

    /***** get the output driver *****/
    
    GDALDriverH hDriver = GDALGetDriverByName( "GTiff" );
    if (hDriver == NULL) {
        exit( 1 );
    }

    /***** create output ds *****/

    GDALDatasetH hOutDS = NULL;



    hOutDS = GDALCreate( hDriver, pszOutFile, nXSrcSize, nYSrcSize, 3, GDT_Byte,
                         papszCreationOptions );
    if ( ! hOutDS )
        exit( 1 );

    int nXDstSize = GDALGetRasterXSize( hOutDS );
    int nYDstSize = GDALGetRasterYSize( hOutDS );

    /***** get the output bands *****/

    GDALRasterBandH ahDstBands[3] = {NULL};
    
    ahDstBands[0] = GDALGetRasterBand(hOutDS, 1);
    ahDstBands[1] = GDALGetRasterBand(hOutDS, 2);
    ahDstBands[2] = GDALGetRasterBand(hOutDS, 3);
    
    /***** get the block size of the output bands *****/

    int nXBlockSize, nYBlockSize;
    GDALGetBlockSize( ahDstBands[0], &nXBlockSize, &nYBlockSize );

    /***** copy geotrasnsform and projection *****/

    double adfGeoTransform[6];

    if ( CE_None == GDALGetGeoTransform( hInDS, adfGeoTransform ) ) {
        GDALSetGeoTransform( hOutDS, adfGeoTransform );
        GDALSetProjection( hOutDS, GDALGetProjectionRef( hInDS ) );
    }

    /***** create the mask band *****/

    if ( CE_None != GDALCreateDatasetMaskBand(hOutDS, GMF_PER_DATASET) ) {
        CPLError(CE_Failure, CPLE_AppDefined,
                 "Failed to create mask band on output DS");
        exit( 1 );
    }

    GDALRasterBandH hMaskBand = GDALGetMaskBand(ahDstBands[0]);

    /***** what method do we use *****/

    if (!bNearWhite && !bNearBlack) {
        do_regular( ahSrcBands, ahDstBands, hMaskBand, bQuiet,
                     bHasAlpha, nXDstSize, nYDstSize );
    }

    else {
        do_nearblack( ahSrcBands, ahDstBands, hMaskBand, bQuiet,
                      bNearWhite, bNearBlack, nMaxNonBlack,
                      nNearDist, nXDstSize, nYDstSize );
    }
    
    /***** cleanup *****/

    GDALClose (hOutDS);
    GDALClose (hInDS);

    GDALDumpOpenDatasets( stderr );
    CSLDestroy( papszCreationOptions );
}



/******************************************************************************
 function to make a mask band from alpha band or nodata values
******************************************************************************/

void do_regular(
    GDALRasterBandH *pahSrcBands,
    GDALRasterBandH *pahDstBands,
    GDALRasterBandH hMaskBand,
    int bQuiet,
    int bHasAlpha,
    int nXDstSize,
    int nYDstSize
)
{
    int iBand = 0;
    
    /***** get the block size of the output bands *****/

    int nXBlockSize, nYBlockSize;
    GDALGetBlockSize( pahDstBands[0], &nXBlockSize, &nYBlockSize );

    /***** loop over the input bands and get there nodata values *****/

    int nodata[3];
    int abHasNodata[3];
    
    for (int iBand = 0; iBand < 3; iBand++) { 
        nodata[iBand] = GDALGetRasterNoDataValue( pahSrcBands[iBand],
                                                  &(abHasNodata[iBand]));
    }

    /***** use alpha, notada vales, or black? *****/
    
    int bHasNodata = FALSE;

    if (abHasNodata[0] && abHasNodata[1] && abHasNodata[2])
        bHasNodata = TRUE;
    
    fprintf(stderr, "Generating mask from ");
        
    if (bHasAlpha)
        fprintf(stderr, "alpha band.\n");
    
    else if (bHasNodata)
        fprintf(stderr, "NODATA values.\n");
    
    else {
        nodata[0] = 0;
        nodata[1] = 0;
        nodata[2] = 0;
        bHasNodata = TRUE;
        fprintf(stderr, "Black values.\n");
    }

    /***** track number of blocks done for progress meter *****/
    
    int nBlocks = (nXDstSize * nYDstSize) / float(nXBlockSize * nYBlockSize);
    int nBlocksWritten = 0;

    /***** alloc memory for the read buffer) *****/

    GByte **papabyBuf = (GByte **) CPLMalloc( 3 * sizeof(GByte *));
    papabyBuf[0] = (GByte *) CPLMalloc(nXBlockSize * nYBlockSize * sizeof(GByte));
    papabyBuf[1] = (GByte *) CPLMalloc(nXBlockSize * nYBlockSize * sizeof(GByte));
    papabyBuf[2] = (GByte *) CPLMalloc(nXBlockSize * nYBlockSize * sizeof(GByte));

    GByte *pabyAlpha;
    GByte *pabyMask;
    
    if (bHasAlpha)
        pabyAlpha = (GByte *) CPLMalloc(nXBlockSize * nYBlockSize * sizeof(GByte));

    else
        pabyMask = (GByte *) CPLMalloc(1 + nXBlockSize * nYBlockSize * sizeof(GByte));

    /***** loop over the columns in incr of nXBlockSize *****/

    for (int nXOff = 0; nXOff < nXDstSize; nXOff += nXBlockSize) {

        /***** loop over the rows in incr of nYBlockSize *****/
    
        for (int nYOff = 0; nYOff < nYDstSize; nYOff += nYBlockSize, nBlocksWritten += 1) {

            /***** update progres meter *****/
            
            if (!bQuiet)
                GDALTermProgress( (nBlocksWritten / (double) nBlocks), NULL, NULL );

            /***** get the width and height of this block *****/
            
            int nXSize = MIN(nXDstSize - nXOff, nXBlockSize);
            int nYSize = MIN(nYDstSize - nYOff, nYBlockSize);
            
            for (iBand = 0; iBand < 3; iBand++) {
                
                CPLErr eErr;
                eErr = GDALRasterIO ( pahSrcBands[iBand],
                                      GF_Read,
                                      nXOff, nYOff,   /* offset of the start of the block */
                                      nXSize, nYSize, /* current block size */
                                      papabyBuf[iBand],
        		                      nXBlockSize,    /* y size of buffer */
                                      nYBlockSize,    /* y size of buffer */
                                      GDT_Byte,           
                                      0, 0            /* pixel size */
                                     );

                if( eErr != CE_None )
                    exit(1);
            }

            /***** use the alpha band? *****/

            if (bHasAlpha) {

                
                CPLErr eErr;
                
                eErr = GDALRasterIO ( pahSrcBands[3],
                                      GF_Read,
                                      nXOff, nYOff,   /* offset of the start of the block */
                                      nXSize, nYSize, /* current block size */
                                      pabyAlpha,
        		                      nXBlockSize,    /* y size of buffer */
                                      nYBlockSize,    /* y size of buffer */
                                      GDT_Byte,           
                                      0, 0            /* pixel size */
                                     );
                
                if( eErr != CE_None )
                    exit(1);
                
                eErr = GDALRasterIO ( hMaskBand,
                                      GF_Write,
                                      nXOff, nYOff,   /* offset of the start of the block */
                                      nXSize, nYSize, /* current block size */
                                      pabyAlpha,
        		                      nXBlockSize,    /* y size of buffer */
                                      nYBlockSize,    /* y size of buffer */
                                      GDT_Byte,           
                                      0, 0            /* pixel size */
                                     );
                
                if( eErr != CE_None )
                    exit(1);
                
            }

            /***** use the nodata values *****/
            
            else {
              
                for (int nX = 0; nX < nXSize; nX++) {
                    for (int nY = 0; nY < nYSize; nY++) {
    
                        if ( papabyBuf[0][nX + (nXSize * nY)] == nodata[0] &&
                             papabyBuf[1][nX + (nXSize * nY)] == nodata[1] &&
                             papabyBuf[2][nX + (nXSize * nY)] == nodata[2] )
                        {
                            pabyMask[nX + (nXSize * nY)] = 0;
                        }
                        else {
                            pabyMask[nX + (nXSize * nY)] = 255;
                        }
                    }
                }

                CPLErr eErr;
                
                eErr = GDALRasterIO ( hMaskBand,
                                      GF_Write,
                                      nXOff, nYOff,   /* offset of the start of the block */
                                      nXSize, nYSize, /* current block size */
                                      pabyMask,
        		                      nXBlockSize,    /* y size of buffer */
                                      nYBlockSize,    /* y size of buffer */
                                      GDT_Byte,           
                                      0, 0            /* pixel size */
                                     );
                             
                                
                if( eErr != CE_None )
                    exit(1);
                
            }

            /***** write out the data *****/
            
            for (iBand = 0; iBand < 3; iBand++) {
                
                CPLErr eErr;
                eErr = GDALRasterIO ( pahDstBands[iBand],
                                      GF_Write,
                                      nXOff, nYOff,   /* offset of the start of the block */
                                      nXSize, nYSize, /* current block size */
                                      papabyBuf[iBand],
        		                      nXBlockSize,    /* y size of buffer */
                                      nYBlockSize,    /* y size of buffer */
                                      GDT_Byte,           
                                      0, 0            /* pixel size */
                                     );

            if( eErr != CE_None )
                exit(1);
            }
        }
    }

    /***** update progres meter *****/
            
    if (!bQuiet)
        GDALTermProgress( (nBlocksWritten / (double) nBlocks), NULL, NULL );        

    /***** cleanup *****/
    
    CPLFree(papabyBuf[0]);
    CPLFree(papabyBuf[1]);
    CPLFree(papabyBuf[2]);

    CPLFree(papabyBuf);

    if (bHasAlpha)
        CPLFree(pabyAlpha);
    else
        CPLFree(pabyMask);
    
} 


/******************************************************************************
 function to create a mask band with the nearblack method
******************************************************************************/

void do_nearblack(
    GDALRasterBandH *pahSrcBands,
    GDALRasterBandH *pahDstBands,
    GDALRasterBandH hMaskBand,
    int bQuiet,
    int bNearWhite,
    int bNearBlack,
    int nMaxNonBlack,
    int nNearDist,
    int nXDstSize,
    int nYDstSize
)
{
    
    /***** alloc memory for the read buffer) *****/

    int *panLastLineCounts = (int *) CPLCalloc(sizeof(int),nXDstSize);

    GByte **papabyBuf = (GByte **) CPLMalloc( 3 * sizeof(GByte *));
    papabyBuf[0] = (GByte *) CPLMalloc(nXDstSize * sizeof(GByte));
    papabyBuf[1] = (GByte *) CPLMalloc(nXDstSize * sizeof(GByte));
    papabyBuf[2] = (GByte *) CPLMalloc(nXDstSize * sizeof(GByte));

    GByte *pabyMask = (GByte *) CPLMalloc(nXDstSize * sizeof(GByte));
    
    /***** loop over the image 1 line at a time *****/
    
    int iLine;

    for( iLine = 0; iLine < nYDstSize; iLine++ )
    {
        for (int iBand = 0; iBand < 3; iBand++) { 
                
            CPLErr eErr;
            eErr = GDALRasterIO ( pahSrcBands[iBand],
                                  GF_Read,
                                  0, iLine,       /* offset of the start of the line */
                                  nXDstSize, 1,   /* current line size */
                                  papabyBuf[iBand],
        	                      nXDstSize,      /* y size of buffer */
                                  1,              /* y size of buffer */
                                  GDT_Byte,           
                                  0, 0            /* pixel size */
                                 );
        
            if( eErr != CE_None )
                exit(1);
                
        }

        /***** init the mask line *****/

        int iCol;
        for(iCol = 0; iCol < nXDstSize; iCol ++)
            pabyMask[iCol] = 255;

        /***** proccess the line *****/
        
        ProcessLine( papabyBuf, pabyMask, 0, nXDstSize-1, nNearDist, nMaxNonBlack,
                     bNearWhite, panLastLineCounts, TRUE, TRUE );

        ProcessLine( papabyBuf, pabyMask, nXDstSize-1, 0, nNearDist, nMaxNonBlack,
                     bNearWhite, NULL, TRUE, FALSE );

        /***** write the line out *****/

        for (int iBand = 0; iBand < 3; iBand++) { 
                
            CPLErr eErr;
            eErr = GDALRasterIO ( pahDstBands[iBand],
                                  GF_Write,
                                  0, iLine,       /* offset of the start of the line */
                                  nXDstSize, 1,   /* current line size */
                                  papabyBuf[iBand],
        	                      nXDstSize,      /* x size of buffer */
                                  1,              /* y size of buffer */
                                  GDT_Byte,           
                                  0, 0            /* pixel size */
                                 );

            if( eErr != CE_None )
                exit(1);
                
        }
        
        /***** write the mask line out *****/

        CPLErr eErr;
        eErr = GDALRasterIO ( hMaskBand,
                              GF_Write,
                              0, iLine,       /* offset of the start of the line */
                              nXDstSize, 1,   /* current line size */
                              pabyMask,
                              nXDstSize,      /* x size of buffer */
                              1,              /* y size of buffer */
                              GDT_Byte,           
                              0, 0            /* pixel size */
                             );
        if( eErr != CE_None )
            exit(1);
        
        if (!bQuiet)
            GDALTermProgress( 0.5 * ((iLine+1) / (double) nYDstSize), NULL, NULL );
    }

/* -------------------------------------------------------------------- */
/*      Now process from the bottom back up, doing only the vertical pass.*/
/* -------------------------------------------------------------------- */
    memset( panLastLineCounts, 0, sizeof(int) * nXDstSize);
    
    for( iLine = nYDstSize-1; iLine >= 0; iLine-- )
    {

        /***** read the line back from the dst file *****/
        
        for (int iBand = 0; iBand < 3; iBand++) { 
             
            CPLErr eErr;
            eErr = GDALRasterIO ( pahDstBands[iBand],
                                  GF_Read,
                                  0, iLine,       /* offset of the start of the line */
                                  nXDstSize, 1,   /* current line size */
                                  papabyBuf[iBand],
        	                      nXDstSize,      /* x size of buffer */
                                  1,              /* y size of buffer */
                                  GDT_Byte,           
                                  0, 0            /* pixel size */
                                 );
        
            if( eErr != CE_None )
                exit(1);
        }

        CPLErr eErr;

        /***** read the mask back from the dst file *****/

                eErr = GDALRasterIO ( hMaskBand,
                              GF_Read,
                              0, iLine,       /* offset of the start of the line */
                              nXDstSize, 1,   /* current line size */
                              pabyMask,
                              nXDstSize,      /* x size of buffer */
                              1,              /* y size of buffer */
                              GDT_Byte,           
                              0, 0            /* pixel size */
                             );
        
        if( eErr != CE_None )
            exit(1);

        /***** proccess the line *****/
        
        ProcessLine( papabyBuf, pabyMask, 0, nXDstSize-1, nNearDist, nMaxNonBlack,
                     bNearWhite, panLastLineCounts, FALSE, TRUE );
        
        /***** write the line out *****/

        for (int iBand = 0; iBand < 3; iBand++) { 
                
            CPLErr eErr;
            eErr = GDALRasterIO ( pahDstBands[iBand],
                                  GF_Write,
                                  0, iLine,       /* offset of the start of the line */
                                  nXDstSize, 1,   /* current line size */
                                  papabyBuf[iBand],
        	                      nXDstSize,      /* x size of buffer */
                                  1,              /* y size of buffer */
                                  GDT_Byte,           
                                  0, 0            /* pixel size */
                                 );
            if( eErr != CE_None )
                exit(1);
                
        }
        
        /***** write the mask line out *****/
            
        eErr = GDALRasterIO ( hMaskBand,
                              GF_Write,
                              0, iLine,       /* offset of the start of the line */
                              nXDstSize, 1,   /* current line size */
                              pabyMask,
                              nXDstSize,      /* x size of buffer */
                              1,              /* y size of buffer */
                              GDT_Byte,           
                              0, 0            /* pixel size */
                             );
        
        if( eErr != CE_None )
            exit(1);
        
        if (!bQuiet)
            GDALTermProgress( 0.5 + 0.5 * (nYDstSize-iLine) / (double) nYDstSize, 
                            NULL, NULL );
    }


    CPLFree( panLastLineCounts );
    CPLFree(papabyBuf[0]);
    CPLFree(papabyBuf[1]);
    CPLFree(papabyBuf[2]);

    CPLFree(papabyBuf);

    CPLFree(pabyMask);

}

/******************************************************************************
    function to test a pixel for NON black

******************************************************************************/

int IsNonBlack (int bNearWhite, int nNearDist, GByte r, GByte g, GByte b) {
    int bIsNonBlack = FALSE;
    
    if ( bNearWhite ) {
         if ( 255 - r > nNearDist ||
              255 - g > nNearDist ||
              255 - b > nNearDist
            )
        {
            bIsNonBlack = TRUE;
        }
    }
            
    else if ( ! bNearWhite ) {
        if ( r > nNearDist ||
             g > nNearDist ||
             b > nNearDist
           )
        {
            bIsNonBlack = TRUE;
        }
    }

    //printf("%i %i %i %i\n", r, g, b, bIsNonBlack);

    return bIsNonBlack;
}

/************************************************************************/
/*                            ProcessLine()                             */
/*                                                                      */
/*      Process a single scanline of image data.                        */
/************************************************************************/

static void ProcessLine(
    GByte **papabyBufm,
    GByte *pabyMask,
    int iStart,
    int iEnd,
    int nNearDist,
    int nMaxNonBlack,
    int bNearWhite,
    int *panLastLineCounts, 
    int bDoHorizontalCheck, 
    int bDoVerticalCheck
)
{
    int iDir, i;

/* -------------------------------------------------------------------- */
/*      Vertical checking.                                              */
/* -------------------------------------------------------------------- */
    if( bDoVerticalCheck )
    {
        int nXSize = MAX(iStart+1,iEnd+1);

        for( i = 0; i < nXSize; i++ )
        {
            int iBand;
            int bIsNonBlack = FALSE;

            // are we already terminated for this column?
            if( panLastLineCounts[i] > nMaxNonBlack )
                continue;

            if ( IsNonBlack(bNearWhite, nNearDist, papabyBufm[0][i],
                 papabyBufm[1][i], papabyBufm[2][i]) )
            {

                panLastLineCounts[i]++;

                if( panLastLineCounts[i] > nMaxNonBlack )
                    continue; 
            }

            /***** at this point the pixel is conciderd to be nearblack *****/
        
            else
                panLastLineCounts[i] = 0;

            for( iBand = 0; iBand < 3; iBand++ )
            {
                if( bNearWhite )
                    papabyBufm[iBand][i] = 255;
                else
                    papabyBufm[iBand][i] = 0;
            }

            /***** set the mask value *****/
            
            pabyMask[i] = 0;
            
        }
    }

/* -------------------------------------------------------------------- */
/*      Horizontal Checking.                                            */
/* -------------------------------------------------------------------- */
    if( bDoHorizontalCheck )
    {
        int nNonBlackPixels = 0;

        if( iStart < iEnd )
            iDir = 1;
        else
            iDir = -1;

        for( i = iStart; i != iEnd; i += iDir )
        {
            int iBand;
            if ( IsNonBlack(bNearWhite, nNearDist, papabyBufm[0][i],
                 papabyBufm[1][i], papabyBufm[2][i]) )
            {
                nNonBlackPixels++;

                if( nNonBlackPixels > nMaxNonBlack )
                    return;
            }

            /***** at this point the pixel is conciderd to be nearblack *****/

            else
                nNonBlackPixels = 0;

            for( iBand = 0; iBand < 3; iBand++ )
            {
                if( bNearWhite )
                    papabyBufm[iBand][i] = 255;
                else
                    papabyBufm[iBand][i] = 0;
            }

            /***** set the mask value *****/
            
            pabyMask[i] = 0;
            

        }
    }

}
