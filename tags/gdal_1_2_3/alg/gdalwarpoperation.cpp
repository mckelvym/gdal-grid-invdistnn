/******************************************************************************
 * $Id$
 *
 * Project:  High Performance Image Reprojector
 * Purpose:  Implementation of the GDALWarpOperation class.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2003, Frank Warmerdam <warmerdam@pobox.com>
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
 * Revision 1.20  2004/08/09 14:38:27  warmerda
 * added serialize/deserialize support for warpoptions and transformers
 *
 * Revision 1.19  2004/04/29 13:44:55  warmerda
 * fixed some memory leaks
 *
 * Revision 1.18  2004/03/28 21:22:46  warmerda
 * Substantial improvement in default selection of eWorkingDataType.  In some
 * cases we need to use a more refined data type to preserve nodata values.
 *
 * Revision 1.17  2003/11/22 19:13:31  dron
 * Added C bindings for GDALWarpOperation functions.
 *
 * Revision 1.16  2003/07/26 17:43:15  warmerda
 * Added various warp options in ComputeSourceWindow().
 *
 * Revision 1.15  2003/07/24 16:48:40  warmerda
 * fixed bug with window size, and origin in ComputeSourceWindow
 *
 * Revision 1.14  2003/07/04 11:52:22  dron
 * GRA_CubicSpline added to the list of validated algorithms.
 *
 * Revision 1.13  2003/06/05 17:10:13  warmerda
 * changed to report approximately correct completion amount in multi case
 *
 * Revision 1.12  2003/05/27 20:49:26  warmerda
 * added REPORT_TIMINGS support
 *
 * Revision 1.11  2003/05/16 19:43:28  warmerda
 * ensure that chunks dont get negative sizes
 *
 * Revision 1.10  2003/05/07 19:57:32  warmerda
 * INIT_DEST now supports a list of band values
 *
 * Revision 1.9  2003/05/07 19:13:06  warmerda
 * added pre and post warp chunk processor
 *
 * Revision 1.8  2003/05/06 18:32:48  warmerda
 * Added WRITE_FLUSH option support.
 * Fixed multi-threaded support so that mutexes are released after creation.
 *
 * Revision 1.7  2003/04/28 20:47:59  warmerda
 * use dataset level RasterIO call
 *
 * Revision 1.6  2003/04/23 05:18:57  warmerda
 * added multithread support
 *
 * Revision 1.5  2003/03/02 05:25:59  warmerda
 * added some source nodata support
 *
 * Revision 1.4  2003/02/22 02:05:20  warmerda
 * added defaulting of band mapping, added warp options
 *
 * Revision 1.3  2003/02/21 15:41:19  warmerda
 * working minimally
 *
 * Revision 1.2  2003/02/20 21:53:06  warmerda
 * partial implementation
 *
 * Revision 1.1  2003/02/18 17:25:50  warmerda
 * New
 *
 */

#include "gdalwarper.h"
#include "cpl_string.h"
#include "cpl_multiproc.h"

CPL_CVSID("$Id$");

/************************************************************************/
/* ==================================================================== */
/*                          GDALWarpOperation                           */
/* ==================================================================== */
/************************************************************************/

/**
 * \class GDALWarpOperation "gdalwarper.h"
 *
 * High level image warping class. 

<h2>Warper Design</h2>

The overall GDAL high performance image warper is split into a few components.

 - The transformation between input and output file coordinates is handled
via GDALTransformerFunc() implementations such as the one returned by
GDALCreateGenImgProjTransformer().  The transformers are ultimately responsible
for translating pixel/line locations on the destination image to pixel/line
locations on the source image. 

 - In order to handle images too large to hold in RAM, the warper needs to
segment large images.  This is the responsibility of the GDALWarpOperation
class.  The GDALWarpOperation::ChunkAndWarpImage() invokes 
GDALWarpOperation::WarpRegion() on chunks of output and input image that
are small enough to hold in the amount of memory allowed by the application. 
This process is described in greater detail in the <b>Image Chunking</b> 
section. 

 - The GDALWarpOperation::WarpRegion() function creates and loads an output 
image buffer, and then calls WarpRegionToBuffer(). 

 - GDALWarpOperation::WarpRegionToBuffer() is responsible for loading the 
source imagery corresponding to a particular output region, and generating
masks and density masks from the source and destination imagery using
the generator functions found in the GDALWarpOptions structure.  Binds this
all into an instance of GDALWarpKernel on which the 
GDALWarpKernel::PerformWarp() method is called. 

 - GDALWarpKernel does the actual image warping, but is given an input image
and an output image to operate on.  The GDALWarpKernel does no IO, and in
fact knows nothing about GDAL.  It invokes the transformation function to 
get sample locations, builds output values based on the resampling algorithm
in use.  It also takes any validity and density masks into account during
this operation.  

<h3>Chunk Size Selection</h3>

The GDALWarpOptions ChunkAndWarpImage() method is responsible for invoking
the WarpRegion() method on appropriate sized output chunks such that the
memory required for the output image buffer, input image buffer and any
required density and validity buffers is less than or equal to the application
defined maximum memory available for use.  

It checks the memory requrired by walking the edges of the output region, 
transforming the locations back into source pixel/line coordinates and 
establishing a bounding rectangle of source imagery that would be required
for the output area.  This is actually accomplished by the private
GDALWarpOperation::ComputeSourceWindow() method. 

Then memory requirements are used by totaling the memory required for all
output bands, input bands, validity masks and density masks.  If this is
greater than the GDALWarpOptions::dfWarpMemoryLimit then the destination
region is divided in two (splitting the longest dimension), and 
ChunkAndWarpImage() recursively invoked on each destination subregion. 

<h3>Validity and Density Masks Generation</h3>

Fill in ways in which the validity and density masks may be generated here. 
Note that detailed semantics of the masks should be found in
GDALWarpKernel. 

*/

/************************************************************************/
/*                         GDALWarpOperation()                          */
/************************************************************************/

GDALWarpOperation::GDALWarpOperation()

{
    psOptions = NULL;

    dfProgressBase = 0.0;
    dfProgressScale = 1.0;

    hThread1Mutex = NULL;
    hThread2Mutex = NULL;
    hIOMutex = NULL;
    hWarpMutex = NULL;

    nChunkListCount = 0;
    nChunkListMax = 0;
    panChunkList = NULL;

    bReportTimings = FALSE;
    nLastTimeReported = 0;
}

/************************************************************************/
/*                         ~GDALWarpOperation()                         */
/************************************************************************/

GDALWarpOperation::~GDALWarpOperation()

{
    WipeOptions();

    if( hThread1Mutex != NULL )
    {
        CPLDestroyMutex( hThread1Mutex );
        CPLDestroyMutex( hThread2Mutex );
        CPLDestroyMutex( hIOMutex );
        CPLDestroyMutex( hWarpMutex );
    }

    WipeChunkList();
}

/************************************************************************/
/*                             GetOptions()                             */
/************************************************************************/

const GDALWarpOptions *GDALWarpOperation::GetOptions()

{
    return psOptions;
}

/************************************************************************/
/*                            WipeOptions()                             */
/************************************************************************/

void GDALWarpOperation::WipeOptions()

{
    if( psOptions != NULL )
    {
        GDALDestroyWarpOptions( psOptions );
        psOptions = NULL;
    }
}

/************************************************************************/
/*                          ValidateOptions()                           */
/************************************************************************/

int GDALWarpOperation::ValidateOptions()

{
    if( psOptions == NULL )
    {
        CPLError( CE_Failure, CPLE_IllegalArg, 
                  "GDALWarpOptions.Validate()\n"
                  "  no options currently initialized." );
        return FALSE;
    }

    if( psOptions->dfWarpMemoryLimit < 100000.0 )
    {
        CPLError( CE_Failure, CPLE_IllegalArg, 
                  "GDALWarpOptions.Validate()\n"
                  "  dfWarpMemoryLimit=%g is unreasonably small.",
                  psOptions->dfWarpMemoryLimit );
        return FALSE;
    }

    if( psOptions->eResampleAlg != GRA_NearestNeighbour 
        && psOptions->eResampleAlg != GRA_Bilinear
        && psOptions->eResampleAlg != GRA_Cubic
        && psOptions->eResampleAlg != GRA_CubicSpline )
    {
        CPLError( CE_Failure, CPLE_IllegalArg, 
                  "GDALWarpOptions.Validate()\n"
                  "  eResampleArg=%d is not a supported value.",
                  psOptions->eResampleAlg );
        return FALSE;
    }

    if( (int) psOptions->eWorkingDataType < 1
        && (int) psOptions->eWorkingDataType >= GDT_TypeCount )
    {
        CPLError( CE_Failure, CPLE_IllegalArg, 
                  "GDALWarpOptions.Validate()\n"
                  "  eWorkingDataType=%d is not a supported value.",
                  psOptions->eWorkingDataType );
        return FALSE;
    }

    if( psOptions->hSrcDS == NULL )
    {
        CPLError( CE_Failure, CPLE_IllegalArg, 
                  "GDALWarpOptions.Validate()\n"
                  "  hSrcDS is not set." );
        return FALSE;
    }

    if( psOptions->hDstDS == NULL )
    {
        CPLError( CE_Failure, CPLE_IllegalArg, 
                  "GDALWarpOptions.Validate()\n"
                  "  hDstDS is not set." );
        return FALSE;
    }

    if( psOptions->nBandCount == 0 )
    {
        CPLError( CE_Failure, CPLE_IllegalArg, 
                  "GDALWarpOptions.Validate()\n"
                  "  nBandCount=0, no bands configured!" );
        return FALSE;
    }

    if( psOptions->panSrcBands == NULL || psOptions->panDstBands == NULL )
    {
        CPLError( CE_Failure, CPLE_IllegalArg, 
                  "GDALWarpOptions.Validate()\n"
                  "  Either panSrcBands or panDstBands is NULL." );
        return FALSE;
    }

    for( int iBand = 0; iBand < psOptions->nBandCount; iBand++ )
    {
        if( psOptions->panSrcBands[iBand] < 1 
            || psOptions->panSrcBands[iBand] 
            > GDALGetRasterCount( psOptions->hSrcDS ) )
        {
            CPLError( CE_Failure, CPLE_IllegalArg,
                      "panSrcBands[%d] = %d ... out of range for dataset.",
                      iBand, psOptions->panSrcBands[iBand] );
            return FALSE;
        }
        if( psOptions->panDstBands[iBand] < 1 
            || psOptions->panDstBands[iBand]
            > GDALGetRasterCount( psOptions->hDstDS ) )
        {
            CPLError( CE_Failure, CPLE_IllegalArg,
                      "panDstBands[%d] = %d ... out of range for dataset.",
                      iBand, psOptions->panDstBands[iBand] );
            return FALSE;
        }

        if( GDALGetRasterAccess( 
                GDALGetRasterBand(psOptions->hDstDS,
                                  psOptions->panDstBands[iBand]) )
            == GA_ReadOnly )
        {
            CPLError( CE_Failure, CPLE_IllegalArg,
                      "Destination band %d appears to be read-only.",
                      psOptions->panDstBands[iBand] );
            return FALSE;
        }
    }

    if( psOptions->nBandCount == 0 )
    {
        CPLError( CE_Failure, CPLE_IllegalArg, 
                  "GDALWarpOptions.Validate()\n"
                  "  nBandCount=0, no bands configured!" );
        return FALSE;
    }

    if( psOptions->padfSrcNoDataReal != NULL
        && psOptions->padfSrcNoDataImag == NULL )
    {
        CPLError( CE_Failure, CPLE_IllegalArg, 
                  "GDALWarpOptions.Validate()\n"
                  "  padfSrcNoDataReal set, but padfSrcNoDataImag not set." );
        return FALSE;
    }

    if( psOptions->pfnProgress == NULL )
    {
        CPLError( CE_Failure, CPLE_IllegalArg, 
                  "GDALWarpOptions.Validate()\n"
                  "  pfnProgress is NULL." );
        return FALSE;
    }

    if( psOptions->pfnTransformer == NULL )
    {
        CPLError( CE_Failure, CPLE_IllegalArg, 
                  "GDALWarpOptions.Validate()\n"
                  "  pfnTransformer is NULL." );
        return FALSE;
    }

    if( CSLFetchNameValue( psOptions->papszWarpOptions, 
                         "SAMPLE_STEPS" ) != NULL )
    {
        if( atoi(CSLFetchNameValue( psOptions->papszWarpOptions, 
                                    "SAMPLE_STEPS" )) < 2 )
        {
            CPLError( CE_Failure, CPLE_IllegalArg, 
                      "GDALWarpOptions.Validate()\n"
                      "  SAMPLE_STEPS warp option has illegal value." );
            return FALSE;
        }
    }
    return TRUE;
}

/************************************************************************/
/*                             Initialize()                             */
/************************************************************************/

/**
 * \fn CPLErr GDALWarpOperation::Initialize( const GDALWarpOptions * );
 *
 * This method initializes the GDALWarpOperation's concept of the warp
 * options in effect.  It creates an internal copy of the GDALWarpOptions
 * structure and defaults a variety of additional fields in the internal
 * copy if not set in the provides warp options.
 *
 * Defaulting operations include:
 *  - If the nBandCount is 0, it will be set to the number of bands in the
 *    source image (which must match the output image) and the panSrcBands
 *    and panDstBands will be populated. 
 *
 * @param psNewOptions input set of warp options.  These are copied and may
 * be destroyed after this call by the application. 
 *
 * @return CE_None on success or CE_Failure if an error occurs.
 */

CPLErr GDALWarpOperation::Initialize( const GDALWarpOptions *psNewOptions )

{
    CPLErr eErr = CE_None;

/* -------------------------------------------------------------------- */
/*      Copy the passed in options.                                     */
/* -------------------------------------------------------------------- */
    if( psOptions != NULL )
        WipeOptions();

    psOptions = GDALCloneWarpOptions( psNewOptions );

/* -------------------------------------------------------------------- */
/*      Default band mapping if missing.                                */
/* -------------------------------------------------------------------- */
    if( psOptions->nBandCount == 0 
        && psOptions->hSrcDS != NULL
        && psOptions->hDstDS != NULL 
        && GDALGetRasterCount( psOptions->hSrcDS ) 
        == GDALGetRasterCount( psOptions->hDstDS ) )
    {
        int  i;

        psOptions->nBandCount = GDALGetRasterCount( psOptions->hSrcDS );

        psOptions->panSrcBands = (int *) 
            CPLMalloc(sizeof(int) * psOptions->nBandCount );
        psOptions->panDstBands = (int *) 
            CPLMalloc(sizeof(int) * psOptions->nBandCount );

        for( i = 0; i < psOptions->nBandCount; i++ )
        {
            psOptions->panSrcBands[i] = i+1;
            psOptions->panDstBands[i] = i+1;
        }
    }

/* -------------------------------------------------------------------- */
/*      If no working data type was provided, set one now.              */
/*                                                                      */
/*      Default to the highest resolution output band.  But if the      */
/*      input band is higher resolution and has a nodata value "out     */
/*      of band" with the output type we may need to use the higher     */
/*      resolution input type to ensure we can identify nodata values.  */
/* -------------------------------------------------------------------- */
    if( psOptions->eWorkingDataType == GDT_Unknown 
        && psOptions->hDstDS != NULL 
        && psOptions->nBandCount >= 1 )
    {
        int iBand;
        psOptions->eWorkingDataType = GDT_Byte;

        for( iBand = 0; iBand < psOptions->nBandCount; iBand++ )
        {
            GDALRasterBandH hDstBand = GDALGetRasterBand( 
                psOptions->hDstDS, psOptions->panDstBands[iBand] );
            GDALRasterBandH hSrcBand = GDALGetRasterBand( 
                psOptions->hSrcDS, psOptions->panSrcBands[iBand] );
                                                  
            if( hDstBand != NULL )
                psOptions->eWorkingDataType = 
                    GDALDataTypeUnion( psOptions->eWorkingDataType, 
                                       GDALGetRasterDataType( hDstBand ) );
            
            if( hSrcBand != NULL 
                && psOptions->padfSrcNoDataReal != NULL )
            {
                int bMergeSource = FALSE;

                if( psOptions->padfSrcNoDataImag[iBand] != 0.0 
                    && !GDALDataTypeIsComplex( psOptions->eWorkingDataType ) )
                    bMergeSource = TRUE;
                else if( psOptions->padfSrcNoDataReal[iBand] < 0.0 
                         && (psOptions->eWorkingDataType == GDT_Byte
                             || psOptions->eWorkingDataType == GDT_UInt16
                             || psOptions->eWorkingDataType == GDT_UInt32) )
                    bMergeSource = TRUE;
                else if( psOptions->padfSrcNoDataReal[iBand] < -32768.0 
                         && psOptions->eWorkingDataType == GDT_Int16 )
                    bMergeSource = TRUE;
                else if( psOptions->padfSrcNoDataReal[iBand] < -2147483648.0 
                         && psOptions->eWorkingDataType == GDT_Int32 )
                    bMergeSource = TRUE;
                else if( psOptions->padfSrcNoDataReal[iBand] > 256 
                         && psOptions->eWorkingDataType == GDT_Byte )
                    bMergeSource = TRUE;
                else if( psOptions->padfSrcNoDataReal[iBand] > 32767
                         && psOptions->eWorkingDataType == GDT_Int16 )
                    bMergeSource = TRUE;
                else if( psOptions->padfSrcNoDataReal[iBand] > 65535
                         && psOptions->eWorkingDataType == GDT_UInt16 )
                    bMergeSource = TRUE;
                else if( psOptions->padfSrcNoDataReal[iBand] > 2147483648.0 
                         && psOptions->eWorkingDataType == GDT_Int32 )
                    bMergeSource = TRUE;
                else if( psOptions->padfSrcNoDataReal[iBand] > 4294967295.0
                         && psOptions->eWorkingDataType == GDT_UInt32 )
                    bMergeSource = TRUE;

                if( bMergeSource )
                    psOptions->eWorkingDataType = 
                        GDALDataTypeUnion( psOptions->eWorkingDataType, 
                                           GDALGetRasterDataType( hSrcBand ) );
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      Default memory available.                                       */
/*                                                                      */
/*      For now we default to 64MB of RAM, but eventually we should     */
/*      try various schemes to query physical RAM.  This can            */
/*      certainly be done on Win32 and Linux.                           */
/* -------------------------------------------------------------------- */
    if( psOptions->dfWarpMemoryLimit == 0.0 )
    {
        psOptions->dfWarpMemoryLimit = 64.0 * 1024*1024;
    }

/* -------------------------------------------------------------------- */
/*      Are we doing timings?                                           */
/* -------------------------------------------------------------------- */
    bReportTimings = CSLFetchBoolean( psOptions->papszWarpOptions, 
                                      "REPORT_TIMINGS", FALSE );

/* -------------------------------------------------------------------- */
/*      If the options don't validate, then wipe them.                  */
/* -------------------------------------------------------------------- */
    if( !ValidateOptions() )
        eErr = CE_Failure;

    if( eErr != CE_None )
        WipeOptions();

    return eErr;
}

/************************************************************************/
/*                         GDALCreateWarpOperation()                    */
/************************************************************************/

/**
 * @see GDALWarpOperation::Initialize()
 */

GDALWarpOperationH GDALCreateWarpOperation(
    const GDALWarpOptions *psNewOptions )
{
    GDALWarpOperation *poOperation;

    poOperation = new GDALWarpOperation;
    if ( poOperation->Initialize( psNewOptions ) != CE_None )
    {
        delete poOperation;
        return NULL;
    }

    return (GDALWarpOperationH)poOperation;
}

/************************************************************************/
/*                         GDALDestroyWarpOperation()                   */
/************************************************************************/

/**
 * @see GDALWarpOperation::~GDALWarpOperation()
 */

void GDALDestroyWarpOperation( GDALWarpOperationH hOperation )
{
    if ( hOperation )
        delete (GDALWarpOperation *)hOperation;
}

/************************************************************************/
/*                         ChunkAndWarpImage()                          */
/************************************************************************/

/**
 * \fn CPLErr GDALWarpOperation::ChunkAndWarpImage(
                int nDstXOff, int nDstYOff,  int nDstXSize, int nDstYSize );
 *
 * This method does a complete warp of the source image to the destination
 * image for the indicated region with the current warp options in effect.  
 * Progress is reported to the installed progress monitor, if any.  
 *
 * This function will subdivide the region and recursively call itself 
 * until the total memory required to process a region chunk will all fit
 * in the memory pool defined by GDALWarpOptions::dfWarpMemoryLimit.  
 *
 * Once an appropriate region is selected GDALWarpOperation::WarpRegion()
 * is invoked to do the actual work. 
 *
 * @param nDstXOff X offset to window of destination data to be produced.
 * @param nDstYOff Y offset to window of destination data to be produced.
 * @param nDstXSize Width of output window on destination file to be produced.
 * @param nDstYSize Height of output window on destination file to be produced.
 *
 * @return CE_None on success or CE_Failure if an error occurs.
 */

CPLErr GDALWarpOperation::ChunkAndWarpImage( 
    int nDstXOff, int nDstYOff,  int nDstXSize, int nDstYSize )

{
/* -------------------------------------------------------------------- */
/*      Collect the list of chunks to operate on.                       */
/* -------------------------------------------------------------------- */
    WipeChunkList();
    CollectChunkList( nDstXOff, nDstYOff, nDstXSize, nDstYSize );

/* -------------------------------------------------------------------- */
/*      Process them one at a time, updating the progress               */
/*      information for each region.                                    */
/* -------------------------------------------------------------------- */
    int iChunk;
    double dfPixelsProcessed=0.0, dfTotalPixels = nDstXSize*(double)nDstYSize;

    for( iChunk = 0; iChunk < nChunkListCount; iChunk++ )
    {
        int *panThisChunk = panChunkList + iChunk*8;
        double dfChunkPixels = panThisChunk[2] * (double) panThisChunk[3];
        CPLErr eErr;

        dfProgressBase = dfPixelsProcessed / dfTotalPixels;
        dfProgressScale = dfChunkPixels / dfTotalPixels;

        eErr = WarpRegion( panThisChunk[0], panThisChunk[1], 
                           panThisChunk[2], panThisChunk[3],
                           panThisChunk[4], panThisChunk[5],
                           panThisChunk[6], panThisChunk[7] );

        if( eErr != CE_None )
            return eErr;

        dfPixelsProcessed += dfChunkPixels;
    }

    WipeChunkList();

    return CE_None;
}

/************************************************************************/
/*                         GDALChunkAndWarpImage()                      */
/************************************************************************/

/**
 * @see GDALWarpOperation::ChunkAndWarpImage()
 */

CPLErr GDALChunkAndWarpImage( GDALWarpOperationH hOperation,
    int nDstXOff, int nDstYOff,  int nDstXSize, int nDstYSize )
{
    return ( (GDALWarpOperation *)hOperation )->
        ChunkAndWarpImage( nDstXOff, nDstYOff, nDstXSize, nDstYSize );
}

/************************************************************************/
/*                          ChunkThreadMain()                           */
/************************************************************************/

static void ChunkThreadMain( void *pThreadData )

{
    void *hThreadMutex = ((void **) pThreadData)[0];
    GDALWarpOperation *poOperation = 
        (GDALWarpOperation *) (((void **) pThreadData)[1]);
    int *panChunkInfo = (int *) (((void **) pThreadData)[2]);

    if( !CPLAcquireMutex( hThreadMutex, 2.0 ) )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Failed to acquire thread mutex in ChunkThreadMain()." );
        return;
    }

    CPLErr eErr;

    eErr = 
        poOperation->WarpRegion( panChunkInfo[0], panChunkInfo[1], 
                                 panChunkInfo[2], panChunkInfo[3], 
                                 panChunkInfo[4], panChunkInfo[5], 
                                 panChunkInfo[6], panChunkInfo[7] );

    /* Return error. */
    ((void **) pThreadData)[2] = (void *) (long) eErr;

    /* Marks that we are done. */
    ((void **) pThreadData)[1] = NULL;

    /* Release mutex so parent knows we are done. */
    CPLReleaseMutex( hThreadMutex );
}

/************************************************************************/
/*                         ChunkAndWarpMulti()                          */
/************************************************************************/

/**
 * \fn CPLErr GDALWarpOperation::ChunkAndWarpMulti(
                int nDstXOff, int nDstYOff,  int nDstXSize, int nDstYSize );
 *
 * This method does a complete warp of the source image to the destination
 * image for the indicated region with the current warp options in effect.  
 * Progress is reported to the installed progress monitor, if any.  
 *
 * Externally this method operates the same as ChunkAndWarpImage(), but
 * internally this method uses multiple threads to interleave input/output
 * for one region while the processing is being done for another.
 *
 * @param nDstXOff X offset to window of destination data to be produced.
 * @param nDstYOff Y offset to window of destination data to be produced.
 * @param nDstXSize Width of output window on destination file to be produced.
 * @param nDstYSize Height of output window on destination file to be produced.
 *
 * @return CE_None on success or CE_Failure if an error occurs.
 */

CPLErr GDALWarpOperation::ChunkAndWarpMulti( 
    int nDstXOff, int nDstYOff,  int nDstXSize, int nDstYSize )

{
    hThread1Mutex = CPLCreateMutex();
    hThread2Mutex = CPLCreateMutex();
    hIOMutex = CPLCreateMutex();
    hWarpMutex = CPLCreateMutex();

    CPLReleaseMutex( hThread1Mutex );
    CPLReleaseMutex( hThread2Mutex );
    CPLReleaseMutex( hIOMutex );
    CPLReleaseMutex( hWarpMutex );

/* -------------------------------------------------------------------- */
/*      Collect the list of chunks to operate on.                       */
/* -------------------------------------------------------------------- */
    WipeChunkList();
    CollectChunkList( nDstXOff, nDstYOff, nDstXSize, nDstYSize );

/* -------------------------------------------------------------------- */
/*      Process them one at a time, updating the progress               */
/*      information for each region.                                    */
/* -------------------------------------------------------------------- */
    void * volatile papThreadDataList[6] = { hThread1Mutex, NULL, NULL, 
                                            hThread2Mutex, NULL, NULL };
    int iChunk;
    double dfPixelsProcessed=0.0, dfTotalPixels = nDstXSize*(double)nDstYSize;

    for( iChunk = 0; iChunk < nChunkListCount+1; iChunk++ )
    {
        int    iThread = iChunk % 2;

/* -------------------------------------------------------------------- */
/*      Launch thread for this chunk.                                   */
/* -------------------------------------------------------------------- */
        if( iChunk < nChunkListCount )
        {
            int *panThisChunk = panChunkList + iChunk*8;
            double dfChunkPixels = panThisChunk[2] * (double) panThisChunk[3];

            dfProgressBase = dfPixelsProcessed / dfTotalPixels;
            dfProgressScale = dfChunkPixels / dfTotalPixels;

            dfPixelsProcessed += dfChunkPixels;
            
            papThreadDataList[iThread*3+1] = (void *) this;
            papThreadDataList[iThread*3+2] = (void *) panThisChunk;

            CPLDebug( "GDAL", "Start chunk %d.", iChunk );
            if( CPLCreateThread( ChunkThreadMain, 
                          (void *) (papThreadDataList + iThread*3)) == -1 )
            {
                CPLError( CE_Failure, CPLE_AppDefined, 
                          "CPLCreateThread() failed in ChunkAndWarpMulti()" );
                return CE_Failure;
            }

            /* Eventually we need a mechanism to ensure we wait for this 
               thread to acquire the IO mutex before proceeding. */
            if( iChunk == 0 )
                CPLSleep( 0.25 );
        }

        
/* -------------------------------------------------------------------- */
/*      Wait for previous chunks thread to complete.                    */
/* -------------------------------------------------------------------- */
        if( iChunk > 0 )
        {
            iThread = (iChunk-1) % 2;
            
            /* Wait for thread to finish. */
            while( papThreadDataList[iThread*3+1] != NULL )
            {
                if( CPLAcquireMutex( papThreadDataList[iThread*3+0], 1.0 ) )
                    CPLReleaseMutex( papThreadDataList[iThread*3+0] );
            }

            CPLDebug( "GDAL", "Finished chunk %d.", iChunk-1 );

            CPLErr eErr = (CPLErr) (long) papThreadDataList[iThread*3+2];

            if( eErr != CE_None )
                return eErr;
        }            
    }

    WipeChunkList();

    return CE_None;
}

/************************************************************************/
/*                         GDALChunkAndWarpMulti()                      */
/************************************************************************/

/**
 * @see GDALWarpOperation::ChunkAndWarpMulti()
 */

CPLErr GDALChunkAndWarpMulti( GDALWarpOperationH hOperation,
    int nDstXOff, int nDstYOff,  int nDstXSize, int nDstYSize )
{
    return ( (GDALWarpOperation *)hOperation )->
        ChunkAndWarpMulti( nDstXOff, nDstYOff, nDstXSize, nDstYSize );
}

/************************************************************************/
/*                           WipeChunkList()                            */
/************************************************************************/

void GDALWarpOperation::WipeChunkList()

{
    CPLFree( panChunkList );
    panChunkList = NULL;
    nChunkListCount = 0;
    nChunkListMax = 0;
}

/************************************************************************/
/*                          CollectChunkList()                          */
/************************************************************************/

CPLErr GDALWarpOperation::CollectChunkList( 
    int nDstXOff, int nDstYOff,  int nDstXSize, int nDstYSize )

{
/* -------------------------------------------------------------------- */
/*      Compute the bounds of the input area corresponding to the       */
/*      output area.                                                    */
/* -------------------------------------------------------------------- */
    int nSrcXOff, nSrcYOff, nSrcXSize, nSrcYSize;
    CPLErr eErr;

    eErr = ComputeSourceWindow( nDstXOff, nDstYOff, nDstXSize, nDstYSize,
                                &nSrcXOff, &nSrcYOff, &nSrcXSize, &nSrcYSize );
    
    if( eErr != CE_None )
        return eErr;

/* -------------------------------------------------------------------- */
/*      Based on the types of masks in use, how many bits will each     */
/*      source pixel cost us?                                           */
/* -------------------------------------------------------------------- */
    int nSrcPixelCostInBits;

    nSrcPixelCostInBits = 
        GDALGetDataTypeSize( psOptions->eWorkingDataType ) 
        * psOptions->nBandCount;

    if( psOptions->pfnSrcDensityMaskFunc != NULL )
        nSrcPixelCostInBits += 32; /* float mask */

    if( psOptions->papfnSrcPerBandValidityMaskFunc != NULL 
        || psOptions->padfSrcNoDataReal != NULL )
        nSrcPixelCostInBits += psOptions->nBandCount; /* bit/band mask */

    if( psOptions->pfnSrcValidityMaskFunc != NULL )
        nSrcPixelCostInBits += 1; /* bit mask */

/* -------------------------------------------------------------------- */
/*      What about the cost for the destination.                        */
/* -------------------------------------------------------------------- */
    int nDstPixelCostInBits;

    nDstPixelCostInBits = 
        GDALGetDataTypeSize( psOptions->eWorkingDataType ) 
        * psOptions->nBandCount;

    if( psOptions->pfnDstDensityMaskFunc != NULL )
        nDstPixelCostInBits += 32;

    if( psOptions->padfDstNoDataReal != NULL
        || psOptions->pfnDstValidityMaskFunc != NULL )
        nDstPixelCostInBits += psOptions->nBandCount;

/* -------------------------------------------------------------------- */
/*      Does the cost of the current rectangle exceed our memory        */
/*      limit? If so, split the destination along the longest           */
/*      dimension and recurse.                                          */
/* -------------------------------------------------------------------- */
    double dfTotalMemoryUse;

    dfTotalMemoryUse =
        (((double) nSrcPixelCostInBits) * nSrcXSize * nSrcYSize
         + ((double) nDstPixelCostInBits) * nDstXSize * nDstYSize) / 8.0;

    if( dfTotalMemoryUse > psOptions->dfWarpMemoryLimit 
        && (nDstXSize > 2 || nDstYSize > 2) )
    {
        if( nDstXSize > nDstYSize )
        {
            int nChunk1 = nDstXSize / 2;
            int nChunk2 = nDstXSize - nChunk1;

            eErr = CollectChunkList( nDstXOff, nDstYOff, 
                                     nChunk1, nDstYSize );

            if( eErr == CE_None )
            {
                eErr = CollectChunkList( nDstXOff+nChunk1, nDstYOff, 
                                         nChunk2, nDstYSize );
            }
        }
        else
        {
            int nChunk1 = nDstYSize / 2;
            int nChunk2 = nDstYSize - nChunk1;

            eErr = CollectChunkList( nDstXOff, nDstYOff, 
                                     nDstXSize, nChunk1 );

            if( eErr == CE_None )
            {
                eErr = CollectChunkList( nDstXOff, nDstYOff+nChunk1, 
                                         nDstXSize, nChunk2 );
            }
        }

        return eErr;
    }

/* -------------------------------------------------------------------- */
/*      OK, everything fits, so add to the chunk list.                  */
/* -------------------------------------------------------------------- */
    if( nChunkListCount == nChunkListMax )
    {
        nChunkListMax = nChunkListMax * 2 + 1;
        panChunkList = (int *) 
            CPLRealloc(panChunkList,sizeof(int)*nChunkListMax*8 );
    }

    panChunkList[nChunkListCount*8+0] = nDstXOff;
    panChunkList[nChunkListCount*8+1] = nDstYOff;
    panChunkList[nChunkListCount*8+2] = nDstXSize;
    panChunkList[nChunkListCount*8+3] = nDstYSize;
    panChunkList[nChunkListCount*8+4] = nSrcXOff;
    panChunkList[nChunkListCount*8+5] = nSrcYOff;
    panChunkList[nChunkListCount*8+6] = nSrcXSize;
    panChunkList[nChunkListCount*8+7] = nSrcYSize;

    nChunkListCount++;

    return CE_None;
}


/************************************************************************/
/*                             WarpRegion()                             */
/************************************************************************/

/**
 * \fn CPLErr GDALWarpOperation::WarpRegion(int nDstXOff, int nDstYOff, 
                                            int nDstXSize, int nDstYSize,
                                            int nSrcXOff=0, int nSrcYOff=0,
                                            int nSrcXSize=0, int nSrcYSize=0 );
 *
 * This method requests the indicated region of the output file be generated.
 * 
 * Note that WarpRegion() will produce the requested area in one low level warp
 * operation without verifying that this does not exceed the stated memory
 * limits for the warp operation.  Applications should take care not to call
 * WarpRegion() on too large a region!  This function 
 * is normally called by ChunkAndWarpImage(), the normal entry point for 
 * applications.  Use it instead if staying within memory constraints is
 * desired. 
 *
 * Progress is reported from 0.0 to 1.0 for the indicated region. 
 *
 * @param nDstXOff X offset to window of destination data to be produced.
 * @param nDstYOff Y offset to window of destination data to be produced.
 * @param nDstXSize Width of output window on destination file to be produced.
 * @param nDstYSize Height of output window on destination file to be produced.
 *
 * @return CE_None on success or CE_Failure if an error occurs.
 */

CPLErr GDALWarpOperation::WarpRegion( int nDstXOff, int nDstYOff, 
                                      int nDstXSize, int nDstYSize,
                                      int nSrcXOff, int nSrcYOff,
                                      int nSrcXSize, int nSrcYSize )

{
    CPLErr eErr;
    int   iBand;

/* -------------------------------------------------------------------- */
/*      Acquire IO mutex.                                               */
/* -------------------------------------------------------------------- */
    if( hIOMutex != NULL )
    {
        if( !CPLAcquireMutex( hIOMutex, 600.0 ) )
        {
            CPLError( CE_Failure, CPLE_AppDefined, 
                      "Failed to acquire IOMutex in WarpRegion()." );
            return CE_Failure;
        }
    }

    ReportTiming( NULL );

/* -------------------------------------------------------------------- */
/*      Allocate the output buffer.                                     */
/* -------------------------------------------------------------------- */
    void *pDstBuffer;
    int  nWordSize = GDALGetDataTypeSize(psOptions->eWorkingDataType)/8;
    int  nBandSize = nWordSize * nDstXSize * nDstYSize;

    pDstBuffer = VSIMalloc( nBandSize * psOptions->nBandCount );
    if( pDstBuffer == NULL )
    {
        CPLError( CE_Failure, CPLE_OutOfMemory,
                  "Out of memory allocatint %d byte destination buffer.",
                  nBandSize * psOptions->nBandCount );
        return CE_Failure;
    }

/* -------------------------------------------------------------------- */
/*      If the INIT_DEST option is given the initialize the output      */
/*      destination buffer to the indicated value without reading it    */
/*      from the hDstDS.  This is sometimes used to optimize            */
/*      operation to a new output file ... it doesn't have to           */
/*      written out and read back for nothing.                          */
/* -------------------------------------------------------------------- */
    const char *pszInitDest = CSLFetchNameValue( psOptions->papszWarpOptions,
                                                 "INIT_DEST" );

    if( pszInitDest != NULL )
    {
        char **papszInitValues = 
            CSLTokenizeStringComplex( pszInitDest, ",", FALSE, FALSE );
        int nInitCount = CSLCount(papszInitValues);
                                                           
        for( iBand = 0; iBand < psOptions->nBandCount; iBand++ )
        {
            double adfInitRealImag[2];
            GByte *pBandData;
            const char *pszBandInit = papszInitValues[MIN(iBand,nInitCount-1)];

            if( EQUAL(pszBandInit,"NO_DATA")
                && psOptions->padfDstNoDataReal != NULL )
            {
                adfInitRealImag[0] = psOptions->padfDstNoDataReal[iBand];
                adfInitRealImag[1] = psOptions->padfDstNoDataImag[iBand];
            }
            else
            {
                CPLStringToComplex( pszBandInit,
                                    adfInitRealImag + 0, adfInitRealImag + 1);
            }

            pBandData = ((GByte *) pDstBuffer) + iBand * nBandSize;
            
            if( psOptions->eWorkingDataType == GDT_Byte )
                memset( pBandData, 
                        MAX(0,MIN(255,(int)adfInitRealImag[0])), 
                        nBandSize);
            else if( adfInitRealImag[0] == 0.0 && adfInitRealImag[1] == 0 )
            {
                memset( pBandData, 0, nBandSize );
            }
            else if( adfInitRealImag[1] == 0.0 )
            {
                GDALCopyWords( &adfInitRealImag, GDT_Float64, 0, 
                               pBandData,psOptions->eWorkingDataType,nWordSize,
                               nDstXSize * nDstYSize );
            }
            else
            {
                GDALCopyWords( &adfInitRealImag, GDT_CFloat64, 0, 
                               pBandData,psOptions->eWorkingDataType,nWordSize,
                               nDstXSize * nDstYSize );
            }
        }

        CSLDestroy( papszInitValues );
    }

/* -------------------------------------------------------------------- */
/*      If we aren't doing fixed initialization of the output buffer    */
/*      then read it from disk so we can overlay on existing imagery.   */
/* -------------------------------------------------------------------- */
    if( pszInitDest == NULL )
    {
        eErr = GDALDatasetRasterIO( psOptions->hDstDS, GF_Read, 
                                    nDstXOff, nDstYOff, nDstXSize, nDstYSize,
                                    pDstBuffer, nDstXSize, nDstYSize, 
                                    psOptions->eWorkingDataType, 
                                    psOptions->nBandCount, 
                                    psOptions->panDstBands,
                                    0, 0, 0 );

        if( eErr != CE_None )
        {
            CPLFree( pDstBuffer );
            return eErr;
        }

        ReportTiming( "Output buffer read" );
    }

/* -------------------------------------------------------------------- */
/*      Perform the warp.                                               */
/* -------------------------------------------------------------------- */
    eErr = WarpRegionToBuffer( nDstXOff, nDstYOff, nDstXSize, nDstYSize, 
                               pDstBuffer, psOptions->eWorkingDataType, 
                               nSrcXOff, nSrcYOff, nSrcXSize, nSrcYSize );

/* -------------------------------------------------------------------- */
/*      Write the output data back to disk if all went well.            */
/* -------------------------------------------------------------------- */
    if( eErr == CE_None )
    {
        eErr = GDALDatasetRasterIO( psOptions->hDstDS, GF_Write, 
                                    nDstXOff, nDstYOff, nDstXSize, nDstYSize,
                                    pDstBuffer, nDstXSize, nDstYSize, 
                                    psOptions->eWorkingDataType, 
                                    psOptions->nBandCount, 
                                    psOptions->panDstBands,
                                    0, 0, 0 );
        if( CSLFetchBoolean( psOptions->papszWarpOptions, "WRITE_FLUSH", 
                             FALSE ) )					
        {
            GDALFlushCache( psOptions->hDstDS );
        }
        ReportTiming( "Output buffer write" );
    }

/* -------------------------------------------------------------------- */
/*      Cleanup and return.                                             */
/* -------------------------------------------------------------------- */
    VSIFree( pDstBuffer );
    
    if( hIOMutex != NULL )
        CPLReleaseMutex( hIOMutex );

    return eErr;
}

/************************************************************************/
/*                             GDALWarpRegion()                         */
/************************************************************************/

/**
 * @see GDALWarpOperation::WarpRegion()
 */

CPLErr GDALWarpRegion( GDALWarpOperationH hOperation,
                       int nDstXOff, int nDstYOff,
                       int nDstXSize, int nDstYSize,
                       int nSrcXOff, int nSrcYOff,
                       int nSrcXSize, int nSrcYSize )

{
    return ( (GDALWarpOperation *)hOperation )->
        WarpRegion( nDstXOff, nDstYOff, nDstXSize, nDstYSize,
                    nSrcXOff, nSrcYOff, nSrcXSize, nSrcYSize);
}

/************************************************************************/
/*                            WarpRegionToBuffer()                      */
/************************************************************************/

/**
 * \fn CPLErr GDALWarpOperation::WarpRegionToBuffer( 
                                  int nDstXOff, int nDstYOff, 
                                  int nDstXSize, int nDstYSize, 
                                  void *pDataBuf, 
                                  GDALDataType eBufDataType,
                                  int nSrcXOff=0, int nSrcYOff=0,
                                  int nSrcXSize=0, int nSrcYSize=0 );
 * 
 * This method requests that a particular window of the output dataset
 * be warped and the result put into the provided data buffer.  The output
 * dataset doesn't even really have to exist to use this method as long as
 * the transformation function in the GDALWarpOptions is setup to map to
 * a virtual pixel/line space. 
 *
 * This method will do the whole region in one chunk, so be wary of the
 * amount of memory that might be used. 
 *
 * @param nDstXOff X offset to window of destination data to be produced.
 * @param nDstYOff Y offset to window of destination data to be produced.
 * @param nDstXSize Width of output window on destination file to be produced.
 * @param nDstYSize Height of output window on destination file to be produced.
 * @param pDataBuf the data buffer to place result in, of type eBufDataType.
 * @param eBufDataType the type of the output data buffer.  For now this
 * must match GDALWarpOptions::eWorkingDataType. 
 * @param nSrcXOff source window X offset (computed if window all zero)
 * @param nSrcYOff source window Y offset (computed if window all zero)
 * @param nSrcXSize source window X size (computed if window all zero)
 * @param nSrcYSize source window Y size (computed if window all zero)
 *
 * @return CE_None on success or CE_Failure if an error occurs.
 */
                                 
CPLErr GDALWarpOperation::WarpRegionToBuffer( 
    int nDstXOff, int nDstYOff, int nDstXSize, int nDstYSize, 
    void *pDataBuf, GDALDataType eBufDataType,
    int nSrcXOff, int nSrcYOff, int nSrcXSize, int nSrcYSize )

{
    CPLErr eErr = CE_None;
    int    i;
    int    nWordSize = GDALGetDataTypeSize(psOptions->eWorkingDataType)/8;

    (void) eBufDataType;
    CPLAssert( eBufDataType == psOptions->eWorkingDataType );

/* -------------------------------------------------------------------- */
/*      If not given a corresponding source window compute one now.     */
/* -------------------------------------------------------------------- */
    if( nSrcXSize == 0 && nSrcYSize == 0 )
    {
        eErr = ComputeSourceWindow( nDstXOff, nDstYOff, nDstXSize, nDstYSize,
                                    &nSrcXOff, &nSrcYOff, 
                                    &nSrcXSize, &nSrcYSize );
    
        if( eErr != CE_None )
            return eErr;
    }

/* -------------------------------------------------------------------- */
/*      Prepare a WarpKernel object to match this operation.            */
/* -------------------------------------------------------------------- */
    GDALWarpKernel   oWK;

    oWK.eResample = psOptions->eResampleAlg;
    oWK.nBands = psOptions->nBandCount;
    oWK.eWorkingDataType = psOptions->eWorkingDataType;

    oWK.pfnTransformer = psOptions->pfnTransformer;
    oWK.pTransformerArg = psOptions->pTransformerArg;
    
    oWK.pfnProgress = psOptions->pfnProgress;
    oWK.pProgress = psOptions->pProgressArg;
    oWK.dfProgressBase = dfProgressBase;
    oWK.dfProgressScale = dfProgressScale;

    oWK.papszWarpOptions = psOptions->papszWarpOptions;

/* -------------------------------------------------------------------- */
/*      Setup the source buffer.                                        */
/*                                                                      */
/*      Eventually we may need to take advantage of pixel               */
/*      interleaved reading here.                                       */
/* -------------------------------------------------------------------- */
    oWK.nSrcXOff = nSrcXOff;
    oWK.nSrcYOff = nSrcYOff;
    oWK.nSrcXSize = nSrcXSize;
    oWK.nSrcYSize = nSrcYSize;

    oWK.papabySrcImage = (GByte **) 
        CPLCalloc(sizeof(GByte*),psOptions->nBandCount);
    oWK.papabySrcImage[0] = (GByte *)
        VSIMalloc( nWordSize * nSrcXSize * nSrcYSize * psOptions->nBandCount );

    if( oWK.papabySrcImage[0] == NULL )
    {
        CPLError( CE_Failure, CPLE_OutOfMemory, 
                  "Failed to allocate %d byte source buffer.",
                  nWordSize * nSrcXSize * nSrcYSize * psOptions->nBandCount );
        eErr = CE_Failure;
    }
        
    for( i = 0; i < psOptions->nBandCount && eErr == CE_None; i++ )
        oWK.papabySrcImage[i] = ((GByte *) oWK.papabySrcImage[0])
            + nWordSize * nSrcXSize * nSrcYSize * i;

    if( eErr == CE_None && nSrcXSize > 0 && nSrcYSize > 0 )
        eErr = 
            GDALDatasetRasterIO( psOptions->hSrcDS, GF_Read, 
                                 nSrcXOff, nSrcYOff, nSrcXSize, nSrcYSize, 
                                 oWK.papabySrcImage[0], nSrcXSize, nSrcYSize,
                                 psOptions->eWorkingDataType, 
                                 psOptions->nBandCount, psOptions->panSrcBands,
                                 0, 0, 0 );

    ReportTiming( "Input buffer read" );

/* -------------------------------------------------------------------- */
/*      Initialize destination buffer.                                  */
/* -------------------------------------------------------------------- */
    oWK.nDstXOff = nDstXOff;
    oWK.nDstYOff = nDstYOff;
    oWK.nDstXSize = nDstXSize;
    oWK.nDstYSize = nDstYSize;

    oWK.papabyDstImage = (GByte **) 
        CPLCalloc(sizeof(GByte*),psOptions->nBandCount);

    for( i = 0; i < psOptions->nBandCount && eErr == CE_None; i++ )
    {
        oWK.papabyDstImage[i] = ((GByte *) pDataBuf)
            + i * nDstXSize * nDstYSize * nWordSize;
    }

/* -------------------------------------------------------------------- */
/*      Eventually we need handling for a whole bunch of the            */
/*      validity and density masks here.                                */
/* -------------------------------------------------------------------- */
    
    /* TODO */
    
/* -------------------------------------------------------------------- */
/*      If we have source nodata values create, or update the           */
/*      validity mask.                                                  */
/* -------------------------------------------------------------------- */
    if( psOptions->padfSrcNoDataReal != NULL )
    {
        for( i = 0; i < psOptions->nBandCount && eErr == CE_None; i++ )
        {
            eErr = CreateKernelMask( &oWK, i, "BandSrcValid" );
            if( eErr == CE_None )
            {
                double adfNoData[2];

                adfNoData[0] = psOptions->padfSrcNoDataReal[i];
                adfNoData[1] = psOptions->padfSrcNoDataImag[i];

                eErr = 
                    GDALWarpNoDataMasker( adfNoData, 1, 
                                          psOptions->eWorkingDataType,
                                          oWK.nSrcXOff, oWK.nSrcYOff, 
                                          oWK.nSrcXSize, oWK.nSrcYSize,
                                          &(oWK.papabySrcImage[i]),
                                          FALSE, oWK.papanBandSrcValid[i] );
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      Release IO Mutex, and acquire warper mutex.                     */
/* -------------------------------------------------------------------- */
    if( hIOMutex != NULL )
    {
        CPLReleaseMutex( hIOMutex );
        if( !CPLAcquireMutex( hWarpMutex, 600.0 ) )
        {
            CPLError( CE_Failure, CPLE_AppDefined, 
                      "Failed to acquire WarpMutex in WarpRegion()." );
            return CE_Failure;
        }
    }

/* -------------------------------------------------------------------- */
/*      Optional application provided prewarp chunk processor.          */
/* -------------------------------------------------------------------- */
    if( eErr == CE_None && psOptions->pfnPreWarpChunkProcessor != NULL )
        eErr = psOptions->pfnPreWarpChunkProcessor( 
            (void *) &oWK, psOptions->pPreWarpProcessorArg );

/* -------------------------------------------------------------------- */
/*      Perform the warp.                                               */
/* -------------------------------------------------------------------- */
    if( eErr == CE_None )
    {
        eErr = oWK.PerformWarp();
        ReportTiming( "In memory warp operation" );
    }

/* -------------------------------------------------------------------- */
/*      Optional application provided postwarp chunk processor.         */
/* -------------------------------------------------------------------- */
    if( eErr == CE_None && psOptions->pfnPostWarpChunkProcessor != NULL )
        eErr = psOptions->pfnPostWarpChunkProcessor( 
            (void *) &oWK, psOptions->pPostWarpProcessorArg );

/* -------------------------------------------------------------------- */
/*      Release Warp Mutex, and acquire io mutex.                       */
/* -------------------------------------------------------------------- */
    if( hIOMutex != NULL )
    {
        CPLReleaseMutex( hWarpMutex );
        if( !CPLAcquireMutex( hIOMutex, 600.0 ) )
        {
            CPLError( CE_Failure, CPLE_AppDefined, 
                      "Failed to acquire IOMutex in WarpRegion()." );
            return CE_Failure;
        }
    }
        
/* -------------------------------------------------------------------- */
/*      Cleanup.                                                        */
/* -------------------------------------------------------------------- */
    CPLFree( oWK.papabySrcImage[0] );
    CPLFree( oWK.papabySrcImage );
    CPLFree( oWK.papabyDstImage );

    if( oWK.papanBandSrcValid != NULL )
    {
        for( i = 0; i < oWK.nBands; i++ )
            CPLFree( oWK.papanBandSrcValid[i] );
        CPLFree( oWK.papanBandSrcValid );
    }
    CPLFree( oWK.panUnifiedSrcValid );
    CPLFree( oWK.pafUnifiedSrcDensity );
    CPLFree( oWK.panDstValid );
    CPLFree( oWK.pafDstDensity );
    
    return eErr;
}

/************************************************************************/
/*                            GDALWarpRegionToBuffer()                  */
/************************************************************************/

/**
 * @see GDALWarpOperation::WarpRegionToBuffer()
 */
                                 
CPLErr GDALWarpRegionToBuffer( GDALWarpOperationH hOperation,
    int nDstXOff, int nDstYOff, int nDstXSize, int nDstYSize, 
    void *pDataBuf, GDALDataType eBufDataType,
    int nSrcXOff, int nSrcYOff, int nSrcXSize, int nSrcYSize )

{
    return ( (GDALWarpOperation *)hOperation )->
        WarpRegionToBuffer( nDstXOff, nDstYOff, nDstXSize, nDstYSize,
                            pDataBuf, eBufDataType,
                            nSrcXOff, nSrcYOff, nSrcXSize, nSrcYSize );
}

/************************************************************************/
/*                          CreateKernelMask()                          */
/*                                                                      */
/*      If mask does not yet exist, create it.  Supported types are     */
/*      the name of the variable in question.  That is                  */
/*      "BandSrcValid", "UnifiedSrcValid", "UnifiedSrcDensity",         */
/*      "DstValid", and "DstDensity".                                   */
/************************************************************************/

CPLErr GDALWarpOperation::CreateKernelMask( GDALWarpKernel *poKernel,
                                            int iBand, const char *pszType )

{
    void **ppMask;
    int  nXSize, nYSize, nBitsPerPixel, nDefault;

/* -------------------------------------------------------------------- */
/*      Get particulars of mask to be updated.                          */
/* -------------------------------------------------------------------- */
    if( EQUAL(pszType,"BandSrcValid") )
    {
        if( poKernel->papanBandSrcValid == NULL )
            poKernel->papanBandSrcValid = (GUInt32 **)
                CPLCalloc( sizeof(void*),poKernel->nBands);
                
        ppMask = (void **) &(poKernel->papanBandSrcValid[iBand]);
        nXSize = poKernel->nSrcXSize;
        nYSize = poKernel->nSrcYSize;
        nBitsPerPixel = 1;
        nDefault = 0xff;
    }
    else if( EQUAL(pszType,"UnifiedSrcValid") )
    {
        ppMask = (void **) &(poKernel->panUnifiedSrcValid);
        nXSize = poKernel->nSrcXSize;
        nYSize = poKernel->nSrcYSize;
        nBitsPerPixel = 1;
        nDefault = 0xff;
    }
    else if( EQUAL(pszType,"UnifiedSrcDensity") )
    {
        ppMask = (void **) &(poKernel->pafUnifiedSrcDensity);
        nXSize = poKernel->nSrcXSize;
        nYSize = poKernel->nSrcYSize;
        nBitsPerPixel = 32;
        nDefault = 0;
    }
    else if( EQUAL(pszType,"DstValid") )
    {
        ppMask = (void **) &(poKernel->panDstValid);
        nXSize = poKernel->nDstXSize;
        nYSize = poKernel->nDstYSize;
        nBitsPerPixel = 1;
        nDefault = 0xff;
    }
    else if( EQUAL(pszType,"DstDensity") )
    {
        ppMask = (void **) &(poKernel->pafDstDensity);
        nXSize = poKernel->nDstXSize;
        nYSize = poKernel->nDstYSize;
        nBitsPerPixel = 32;
        nDefault = 0;
    }
    else
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "Internal error in CreateKernelMask(%s).",
                  pszType );
        return CE_Failure;
    }

/* -------------------------------------------------------------------- */
/*      Allocate if needed.                                             */
/* -------------------------------------------------------------------- */
    if( *ppMask == NULL )
    {
        int nBytes;

        if( nBitsPerPixel == 32 )
            nBytes = nXSize * nYSize * 4;
        else
            nBytes = (nXSize * nYSize + 7) / 8;

        *ppMask = VSIMalloc( nBytes );

        if( *ppMask == NULL )
        {
            CPLError( CE_Failure, CPLE_OutOfMemory, 
                      "Out of memory allocating %d bytes for %s mask.", 
                      nBytes, pszType );
            return CE_Failure;
        }

        memset( *ppMask, nDefault, nBytes );
    }

    return CE_None;
}



/************************************************************************/
/*                        ComputeSourceWindow()                         */
/************************************************************************/

CPLErr GDALWarpOperation::ComputeSourceWindow(int nDstXOff, int nDstYOff, 
                                              int nDstXSize, int nDstYSize,
                                              int *pnSrcXOff, int *pnSrcYOff, 
                                              int *pnSrcXSize, int *pnSrcYSize)

{
/* -------------------------------------------------------------------- */
/*      Figure out whether we just want to do the usual "along the      */
/*      edge" sampling, or using a grid.  The grid usage is             */
/*      important in some weird "inside out" cases like WGS84 to        */
/*      polar stereographic around the pole.   Also figure out the      */
/*      sampling rate.                                                  */
/* -------------------------------------------------------------------- */
    double dfStepSize;
    int nSampleMax, nStepCount = 21, bUseGrid;
    int *pabSuccess = NULL;
    double *padfX, *padfY, *padfZ;
    int    nSamplePoints=0;
    double dfRatio;

    if( CSLFetchNameValue( psOptions->papszWarpOptions, 
                           "SAMPLE_STEPS" ) != NULL )
    {
        nStepCount = 
            atoi(CSLFetchNameValue( psOptions->papszWarpOptions, 
                                    "SAMPLE_STEPS" ));
        nStepCount = MAX(2,nStepCount);
    }

    dfStepSize = 1.0 / (nStepCount-1);

    bUseGrid = CSLFetchBoolean( psOptions->papszWarpOptions, 
                                "SAMPLE_GRID", FALSE );

    if( bUseGrid )
        nSampleMax = nStepCount * nStepCount;
    else
        nSampleMax = nStepCount * 4;

    pabSuccess = (int *) CPLMalloc(sizeof(int) * nSampleMax);
    padfX = (double *) CPLMalloc(sizeof(double) * 3 * nSampleMax);
    padfY = padfX + nSampleMax;
    padfZ = padfX + nSampleMax * 2;

/* -------------------------------------------------------------------- */
/*      Setup sample points on a grid pattern throughout the area.      */
/* -------------------------------------------------------------------- */
    if( bUseGrid )
    {
        double dfRatioY;

        for( dfRatioY = 0.0; 
             dfRatioY <= 1.0 + dfStepSize*0.5; 
             dfRatioY += dfStepSize )
        {
            for( dfRatio = 0.0; 
                 dfRatio <= 1.0 + dfStepSize*0.5; 
                 dfRatio += dfStepSize )
            {
                padfX[nSamplePoints]   = dfRatio * nDstXSize + nDstXOff;
                padfY[nSamplePoints]   = dfRatioY * nDstYSize + nDstYOff;
                padfZ[nSamplePoints++] = 0.0;
            }
        }
    }
 /* -------------------------------------------------------------------- */
 /*      Setup sample points all around the edge of the input raster.    */
 /* -------------------------------------------------------------------- */
    else
    {
        for( dfRatio = 0.0; dfRatio <= 1.0 + dfStepSize*0.5; dfRatio += dfStepSize )
        {
            // Along top 
            padfX[nSamplePoints]   = dfRatio * nDstXSize + nDstXOff;
            padfY[nSamplePoints]   = nDstYOff;
            padfZ[nSamplePoints++] = 0.0;
            
            // Along bottom 
            padfX[nSamplePoints]   = dfRatio * nDstXSize + nDstXOff;
            padfY[nSamplePoints]   = nDstYOff + nDstYSize;
            padfZ[nSamplePoints++] = 0.0;
            
            // Along left
            padfX[nSamplePoints]   = nDstXOff;
            padfY[nSamplePoints]   = dfRatio * nDstYSize + nDstYOff;
            padfZ[nSamplePoints++] = 0.0;
            
            // Along right
            padfX[nSamplePoints]   = nDstXSize + nDstXOff;
            padfY[nSamplePoints]   = dfRatio * nDstYSize + nDstYOff;
            padfZ[nSamplePoints++] = 0.0;
        }
    }
        
    CPLAssert( nSamplePoints == nSampleMax );

/* -------------------------------------------------------------------- */
/*      Transform them to the output coordinate system.                 */
/* -------------------------------------------------------------------- */
    if( !psOptions->pfnTransformer( psOptions->pTransformerArg, 
                                    TRUE, nSamplePoints, 
                                    padfX, padfY, padfZ, pabSuccess ) )
    {
        CPLFree( padfX );
        CPLFree( pabSuccess );

        CPLError( CE_Failure, CPLE_AppDefined, 
                  "GDALWarperOperation::ComputeSourceWindow() failed because\n"
                  "the pfnTransformer failed." );
        return CE_Failure;
    }
        
/* -------------------------------------------------------------------- */
/*      Collect the bounds, ignoring any failed points.                 */
/* -------------------------------------------------------------------- */
    double dfMinXOut=0.0, dfMinYOut=0.0, dfMaxXOut=0.0, dfMaxYOut=0.0;
    int    bGotInitialPoint = FALSE;
    int    nFailedCount = 0, i;

    for( i = 0; i < nSamplePoints; i++ )
    {
        if( !pabSuccess[i] )
        {
            nFailedCount++;
            continue;
        }

        if( !bGotInitialPoint )
        {
            bGotInitialPoint = TRUE;
            dfMinXOut = dfMaxXOut = padfX[i];
            dfMinYOut = dfMaxYOut = padfY[i];
        }
        else
        {
            dfMinXOut = MIN(dfMinXOut,padfX[i]);
            dfMinYOut = MIN(dfMinYOut,padfY[i]);
            dfMaxXOut = MAX(dfMaxXOut,padfX[i]);
            dfMaxYOut = MAX(dfMaxYOut,padfY[i]);
        }
    }

    CPLFree( padfX );
    CPLFree( pabSuccess );

    if( nFailedCount > nSamplePoints - 10 )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Too many points (%d out of %d) failed to transform,\n"
                  "unable to compute output bounds.",
                  nFailedCount, nSamplePoints );
        return CE_Failure;
    }

    if( nFailedCount > 0 )
        CPLDebug( "GDAL", 
                  "GDALWarpOperation::ComputeSourceWindow() %d out of %d points failed to transform.", 
                  nFailedCount, nSamplePoints );

/* -------------------------------------------------------------------- */
/*      How much of a window around our source pixel might we need      */
/*      to collect data from based on the resampling kernel?  Even      */
/*      if the requested central pixel falls off the source image,      */
/*      we may need to collect data if some portion of the              */
/*      resampling kernel could be on-image.                            */
/* -------------------------------------------------------------------- */
    int nResWinSize = 0;

    if( psOptions->eResampleAlg == GRA_Bilinear )
        nResWinSize = 1;
    
    if( psOptions->eResampleAlg == GRA_Cubic )
        nResWinSize = 2;

/* -------------------------------------------------------------------- */
/*      Allow addition of extra sample pixels to source window to       */
/*      avoid missing pixels due to sampling error.                     */
/* -------------------------------------------------------------------- */
    if( CSLFetchNameValue( psOptions->papszWarpOptions, 
                           "SOURCE_EXTRA" ) != NULL )
    {
        nResWinSize += atoi(
            CSLFetchNameValue( psOptions->papszWarpOptions, "SOURCE_EXTRA" ));
    }

/* -------------------------------------------------------------------- */
/*      return bounds.                                                  */
/* -------------------------------------------------------------------- */
    *pnSrcXOff = MAX(0,(int) floor( dfMinXOut ) - nResWinSize );
    *pnSrcYOff = MAX(0,(int) floor( dfMinYOut ) - nResWinSize );
    *pnSrcXSize = MIN( GDALGetRasterXSize(psOptions->hSrcDS) - *pnSrcXOff,
                       ((int) ceil( dfMaxXOut )) - *pnSrcXOff + nResWinSize );
    *pnSrcYSize = MIN( GDALGetRasterYSize(psOptions->hSrcDS) - *pnSrcYOff,
                       ((int) ceil( dfMaxYOut )) - *pnSrcYOff + nResWinSize );
    *pnSrcXSize = MAX(0,*pnSrcXSize);
    *pnSrcYSize = MAX(0,*pnSrcYSize);

    return CE_None;
}

/************************************************************************/
/*                            ReportTiming()                            */
/************************************************************************/

void GDALWarpOperation::ReportTiming( const char * pszMessage )

{
    if( !bReportTimings )
        return;

    unsigned long nNewTime = VSITime(NULL);

    if( pszMessage != NULL )
    {
        CPLDebug( "WARP_TIMING", "%s: %ds", 
                  pszMessage, nNewTime - nLastTimeReported );
    }

    nLastTimeReported = nNewTime;
}

