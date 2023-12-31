/******************************************************************************
 * $Id: gdalwarp.cpp 12380 2007-10-12 17:35:00Z rouault $
 *
 * Project:  GDAL
 * Purpose:  Commandline point transformer.
 * Author:   Frank Warmerdam <warmerdam@pobox.com>
 *
 ******************************************************************************
 * Copyright (c) 2007, Frank Warmerdam <warmerdam@pobox.com>
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

#include "gdalwarper.h"
#include "cpl_string.h"
#include "ogr_spatialref.h"

CPL_CVSID("$Id: gdalwarp.cpp 12380 2007-10-12 17:35:00Z rouault $");

/************************************************************************/
/*                               Usage()                                */
/************************************************************************/

static void Usage()

{
    printf( 
        "Usage: gdaltransform [--help-general]\n"
        "    [-i] [-s_srs srs_def] [-t_srs srs_def] [-order n] ] [-tps]\n"
        "    [-gcp pixel line easting northing [elevation]]*\n" 
        "    [srcfile [dstfile]]\n" 
        "\n" );
    exit( 1 );
}

/************************************************************************/
/*                             SanitizeSRS                              */
/************************************************************************/

char *SanitizeSRS( const char *pszUserInput )

{
    OGRSpatialReferenceH hSRS;
    char *pszResult = NULL;

    CPLErrorReset();
    
    hSRS = OSRNewSpatialReference( NULL );
    if( OSRSetFromUserInput( hSRS, pszUserInput ) == OGRERR_NONE )
        OSRExportToWkt( hSRS, &pszResult );
    else
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "Translating source or target SRS failed:\n%s",
                  pszUserInput );
        exit( 1 );
    }
    
    OSRDestroySpatialReference( hSRS );

    return pszResult;
}

/************************************************************************/
/*                                main()                                */
/************************************************************************/

int main( int argc, char ** argv )

{
    const char         *pszSrcFilename = NULL;
    const char         *pszDstFilename = NULL;
    char               *pszTargetSRS = NULL;
    char               *pszSourceSRS = NULL;
    int                 nOrder = 0;
    void               *hTransformArg;
    GDALTransformerFunc pfnTransformer = NULL;
    int                 nGCPCount = 0;
    GDAL_GCP            *pasGCPs = NULL;
    int                 bInverse = FALSE;

    GDALAllRegister();
    argc = GDALGeneralCmdLineProcessor( argc, &argv, 0 );
    if( argc < 1 )
        exit( -argc );

/* -------------------------------------------------------------------- */
/*      Parse arguments.                                                */
/* -------------------------------------------------------------------- */
    int i;

    for( i = 1; i < argc; i++ )
    {
        if( EQUAL(argv[i],"-t_srs") && i < argc-1 )
        {
            pszTargetSRS = SanitizeSRS(argv[++i]);
        }
        else if( EQUAL(argv[i],"-s_srs") && i < argc-1 )
        {
            pszSourceSRS = SanitizeSRS(argv[++i]);
        }
        else if( EQUAL(argv[i],"-order") && i < argc-1 )
        {
            nOrder = atoi(argv[++i]);
        }
        else if( EQUAL(argv[i],"-tps") )
        {
            nOrder = -1;
        }
        else if( EQUAL(argv[i],"-i") )
        {
            bInverse = TRUE;
        }
        else if( EQUAL(argv[i],"-gcp") && i < argc - 4 )
        {
            char* endptr = NULL;
            /* -gcp pixel line easting northing [elev] */

            nGCPCount++;
            pasGCPs = (GDAL_GCP *) 
                CPLRealloc( pasGCPs, sizeof(GDAL_GCP) * nGCPCount );
            GDALInitGCPs( 1, pasGCPs + nGCPCount - 1 );

            pasGCPs[nGCPCount-1].dfGCPPixel = atof(argv[++i]);
            pasGCPs[nGCPCount-1].dfGCPLine = atof(argv[++i]);
            pasGCPs[nGCPCount-1].dfGCPX = atof(argv[++i]);
            pasGCPs[nGCPCount-1].dfGCPY = atof(argv[++i]);
            if( argv[i+1] != NULL 
                && (CPLStrtod(argv[i+1], &endptr) != 0.0 || argv[i+1][0] == '0') )
            {
                /* Check that last argument is really a number and not a filename */
                /* looking like a number (see ticket #863) */
                if (endptr && *endptr == 0)
                    pasGCPs[nGCPCount-1].dfGCPZ = atof(argv[++i]);
            }

            /* should set id and info? */
        }   

        else if( argv[i][0] == '-' )
            Usage();

        else if( pszSrcFilename == NULL )
            pszSrcFilename = argv[i];

        else if( pszDstFilename == NULL )
            pszDstFilename = argv[i];

        else
            Usage();
    }

/* -------------------------------------------------------------------- */
/*      Open src and destination file, if appropriate.                  */
/* -------------------------------------------------------------------- */
    GDALDatasetH hSrcDS = NULL, hDstDS = NULL;

    if( pszSrcFilename != NULL )
    {
        hSrcDS = GDALOpen( pszSrcFilename, GA_ReadOnly );
        if( hSrcDS == NULL )
            exit( 1 );
    }

    if( pszDstFilename != NULL )
    {
        hDstDS = GDALOpen( pszDstFilename, GA_ReadOnly );
        if( hDstDS == NULL )
            exit( 1 );
    }

    if( hSrcDS != NULL && nGCPCount > 0 )
    {
        fprintf( stderr, "Commandline GCPs and input file specified, specify one or the other.\n" );
        exit( 1 );
    }
    
/* -------------------------------------------------------------------- */
/*      Create a transformation object from the source to               */
/*      destination coordinate system.                                  */
/* -------------------------------------------------------------------- */
    if( nGCPCount != 0 && nOrder == -1 )
    {
        pfnTransformer = GDALTPSTransform;
        hTransformArg = 
            GDALCreateTPSTransformer( nGCPCount, pasGCPs, FALSE );
    }
    else if( nGCPCount != 0 )
    {
        pfnTransformer = GDALGCPTransform;
        hTransformArg = 
            GDALCreateGCPTransformer( nGCPCount, pasGCPs, nOrder, FALSE );
    }
    else
    {
        pfnTransformer = GDALGenImgProjTransform;
        hTransformArg = 
            GDALCreateGenImgProjTransformer( hSrcDS, pszSourceSRS, 
                                             hDstDS, pszTargetSRS, 
                                             TRUE, 1000.0, nOrder );
    }

    if( hTransformArg == NULL )
    {
        CPLFree( pszTargetSRS );
        exit( 1 );
    }

/* -------------------------------------------------------------------- */
/*      Read points from stdin, transform and write to stdout.          */
/* -------------------------------------------------------------------- */
    while( !feof(stdin) )
    {
        char szLine[1024];

        if( fgets( szLine, sizeof(szLine)-1, stdin ) == NULL )
            break;

        char **papszTokens = CSLTokenizeString(szLine);
        double dfX, dfY, dfZ = 0.0;
        int bSuccess = TRUE;

        if( CSLCount(papszTokens) < 2 )
            continue;

        dfX = atof(papszTokens[0]);
        dfY = atof(papszTokens[1]);
        if( CSLCount(papszTokens) >= 3 )
            dfZ = atof(papszTokens[2]);

        if( pfnTransformer( hTransformArg, bInverse, 1, 
                            &dfX, &dfY, &dfZ, &bSuccess ) )
        {
            printf( "%.15g %.15g %.15g\n", dfX, dfY, dfZ );
        }
        else
        {
            printf( "transformation failed.\n" );
        }
    }
}
