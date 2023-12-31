/******************************************************************************
 * $Id$
 *
 * Project:  Erdas Imagine (.img) Translator
 * Purpose:  Testing mainline for HFA services - transitory.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 1999, Intergraph Corporation
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
 * Revision 1.8  2003/05/13 19:32:10  warmerda
 * support for reading and writing opacity provided by Diana Esch-Mosher
 *
 * Revision 1.7  2003/04/22 19:40:36  warmerda
 * fixed email address
 *
 * Revision 1.6  2001/07/18 04:51:57  warmerda
 * added CPL_CVSID
 *
 * Revision 1.5  2000/10/20 04:18:15  warmerda
 * added overviews, stateplane, and u4
 *
 * Revision 1.4  2000/10/12 19:31:01  warmerda
 * fixed usage
 *
 * Revision 1.3  1999/01/22 17:40:05  warmerda
 * Added projections, moved debugging stuff out
 *
 * Revision 1.2  1999/01/04 22:52:47  warmerda
 * field access working
 *
 * Revision 1.1  1999/01/04 05:28:13  warmerda
 * New
 *
 */

#include "hfa_p.h"

CPL_CVSID("$Id$");

/************************************************************************/
/*                               Usage()                                */
/************************************************************************/

static void Usage()

{
    printf( "hfatest [-dd] [-dt] [-dr] filename\n" );
}

/************************************************************************/
/*                                main()                                */
/************************************************************************/

int main( int argc, char ** argv )

{
    const char	*pszFilename = NULL;
    int		nDumpTree = FALSE;
    int		nDumpDict = FALSE;
    int		nRastReport = FALSE;
    int		i, nXSize, nYSize, nBands;
    HFAHandle	hHFA;
    const Eprj_MapInfo *psMapInfo;
    const Eprj_ProParameters *psProParameters;
    const Eprj_Datum *psDatum;

/* -------------------------------------------------------------------- */
/*      Handle arguments.                                               */
/* -------------------------------------------------------------------- */
    for( i = 1; i < argc; i++ )
    {
        if( EQUAL(argv[i],"-dd") )
            nDumpDict = TRUE;
        else if( EQUAL(argv[i],"-dt") )
            nDumpTree = TRUE;
        else if( EQUAL(argv[i],"-dr") )
            nRastReport = TRUE;
        else if( pszFilename == NULL )
            pszFilename = argv[i];
        else
        {
            Usage();
            exit( 1 );
        }
    }

    if( pszFilename == NULL )
    {
        Usage();
        exit( 1 );
    }

/* -------------------------------------------------------------------- */
/*      Open the file.                                                  */
/* -------------------------------------------------------------------- */
    hHFA = HFAOpen( pszFilename, "r" );

    if( hHFA == NULL )
    {
        printf( "HFAOpen() failed.\n" );
        exit( 100 );
    }

/* -------------------------------------------------------------------- */
/*      Do we want to walk the tree dumping out general information?    */
/* -------------------------------------------------------------------- */
    if( nDumpDict )
    {
        HFADumpDictionary( hHFA, stdout );
    }

/* -------------------------------------------------------------------- */
/*      Do we want to walk the tree dumping out general information?    */
/* -------------------------------------------------------------------- */
    if( nDumpTree )
    {
        HFADumpTree( hHFA, stdout );
    }

/* -------------------------------------------------------------------- */
/*      Dump indirectly collected data about bands.                     */
/* -------------------------------------------------------------------- */
    HFAGetRasterInfo( hHFA, &nXSize, &nYSize, &nBands );

    if( nRastReport )
    {
        printf( "Raster Size = %d x %d\n", nXSize, nYSize );

        for( i = 1; i <= nBands; i++ )
        {
            int	nDataType, nColors, nOverviews, iOverview;
            double	*padfRed, *padfGreen, *padfBlue, *padfAlpha;
            int nBlockXSize, nBlockYSize;
        
            HFAGetBandInfo( hHFA, i, &nDataType, &nBlockXSize, &nBlockYSize, 
                            &nOverviews );
            printf( "Band %d: %dx%d tiles, type = %d\n",
                    i, nBlockXSize, nBlockYSize, nDataType );

            for( iOverview=0; iOverview < nOverviews; iOverview++ )
            {
                HFAGetOverviewInfo( hHFA, i, iOverview, 
                                    &nXSize, &nYSize, 
                                    &nBlockXSize, &nBlockYSize );
                printf( "  Overview: %dx%d (blocksize %dx%d)\n", 
                        nXSize, nYSize, nBlockXSize, nBlockYSize );
            }

            if( HFAGetPCT( hHFA, i, &nColors, &padfRed, &padfGreen, 
			   &padfBlue, &padfAlpha )
                == CE_None )
            {
                int	j;

                for( j = 0; j < nColors; j++ )
                {
                    printf( "PCT[%d] = %f,%f,%f %f\n",
                            j, padfRed[j], padfGreen[j], 
			    padfBlue[j], padfAlpha[j]);
                }
            }
        }

/* -------------------------------------------------------------------- */
/*      Dump the map info structure.                                    */
/* -------------------------------------------------------------------- */
        psMapInfo = HFAGetMapInfo( hHFA );

        if( psMapInfo != NULL )
        {
            printf( "MapInfo.proName = %s\n", psMapInfo->proName );
            printf( "MapInfo.upperLeftCenter.x = %.2f\n",
                    psMapInfo->upperLeftCenter.x );
            printf( "MapInfo.upperLeftCenter.y = %.2f\n",
                    psMapInfo->upperLeftCenter.y );
        }
        else
        {
            printf( "No Map Info found\n" );
        }
    }
    
    psProParameters = HFAGetProParameters( hHFA );

    psDatum = HFAGetDatum( hHFA );
    
    HFAClose( hHFA );

#ifdef DBMALLOC
    malloc_dump(1);
#endif

    exit( 0 );
}
