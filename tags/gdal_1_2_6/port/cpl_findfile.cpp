/******************************************************************************
 * $Id$
 *
 * Project:  CPL - Common Portability Library
 * Purpose:  Generic data file location finder, with application hooking.
 * Author:   Frank Warmerdam, warmerda@home.com
 *
 ******************************************************************************
 * Copyright (c) 2000, Frank Warmerdam
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
 * Revision 1.8  2005/01/15 07:46:20  fwarmerdam
 * make cplpopfinderlocation safer for final cleanup
 *
 * Revision 1.7  2004/11/22 16:01:05  fwarmerdam
 * added GDAL_PREFIX
 *
 * Revision 1.6  2003/10/24 16:41:16  warmerda
 * Added /usr/local/share/gdal (not /usr/local/share) to default locations.
 *
 * Revision 1.5  2003/10/24 16:30:10  warmerda
 * fixed serious bug in default finder ... only last location used
 *
 * Revision 1.4  2002/12/03 04:42:02  warmerda
 * improved finder cleanup support
 *
 * Revision 1.3  2001/07/18 04:00:49  warmerda
 * added CPL_CVSID
 *
 * Revision 1.2  2001/01/19 21:16:41  warmerda
 * expanded tabs
 *
 * Revision 1.1  2000/08/29 21:06:25  warmerda
 * New
 *
 */

#include "cpl_conv.h"
#include "cpl_string.h"

CPL_CVSID("$Id$");

static int bFinderInitialized = FALSE;
static int nFileFinders = 0;
static CPLFileFinder *papfnFinders = NULL;
static char **papszFinderLocations = NULL;

/************************************************************************/
/*                           CPLFinderInit()                            */
/************************************************************************/

static void CPLFinderInit()

{
    if( !bFinderInitialized )
    {
        bFinderInitialized = TRUE;
        CPLPushFileFinder( CPLDefaultFindFile );
#ifdef GDAL_PREFIX
        CPLPushFinderLocation( GDAL_PREFIX "/share/gdal" );
#else
        CPLPushFinderLocation( "/usr/local/share/gdal" );
#endif
        CPLPushFinderLocation( "." );
    }
}

/************************************************************************/
/*                           CPLFinderClean()                           */
/************************************************************************/

void CPLFinderClean()

{
    while( papszFinderLocations != NULL )
        CPLPopFinderLocation();
    while( CPLPopFileFinder() != NULL ) {}

    bFinderInitialized = FALSE;
}

/************************************************************************/
/*                         CPLDefaultFileFind()                         */
/************************************************************************/

const char *CPLDefaultFindFile( const char *pszClass, 
                                const char *pszBasename )

{
    int         i, nLocations = CSLCount( papszFinderLocations );

    (void) pszClass;

    for( i = nLocations-1; i >= 0; i-- )
    {
        const char  *pszResult;
        VSIStatBuf  sStat;

        pszResult = CPLFormFilename( papszFinderLocations[i], pszBasename, 
                                     NULL );

        if( VSIStat( pszResult, &sStat ) == 0 )
            return pszResult;
    }
    
    return NULL;
}

/************************************************************************/
/*                            CPLFindFile()                             */
/************************************************************************/

const char *CPLFindFile( const char *pszClass, const char *pszBasename )

{
    int         i;

    CPLFinderInit();

    for( i = nFileFinders-1; i >= 0; i-- )
    {
        const char * pszResult;

        pszResult = (papfnFinders[i])( pszClass, pszBasename );
        if( pszResult != NULL )
            return pszResult;
    }

    return NULL;
}

/************************************************************************/
/*                         CPLPushFileFinder()                          */
/************************************************************************/

void CPLPushFileFinder( CPLFileFinder pfnFinder )

{
    CPLFinderInit();

    papfnFinders = (CPLFileFinder *) 
        CPLRealloc(papfnFinders,  sizeof(void*) * ++nFileFinders);
    papfnFinders[nFileFinders-1] = pfnFinder;
}

/************************************************************************/
/*                          CPLPopFileFinder()                          */
/************************************************************************/

CPLFileFinder CPLPopFileFinder()

{
    CPLFileFinder pfnReturn;

    CPLFinderInit();

    if( nFileFinders == 0 )
        return NULL;

    pfnReturn = papfnFinders[--nFileFinders];

    if( nFileFinders == 0)
    {
        CPLFree( papfnFinders );
        papfnFinders = NULL;
    }

    return pfnReturn;
}

/************************************************************************/
/*                       CPLPushFinderLocation()                        */
/************************************************************************/

void CPLPushFinderLocation( const char *pszLocation )

{
    CPLFinderInit();

    papszFinderLocations  = CSLAddString( papszFinderLocations, 
                                          pszLocation );
}


/************************************************************************/
/*                       CPLPopFinderLocation()                         */
/************************************************************************/

void CPLPopFinderLocation()

{
    int      nCount;

    if( papszFinderLocations == NULL )
        return;

    CPLFinderInit();

    nCount = CSLCount(papszFinderLocations);
    if( nCount == 0 )
        return;

    CPLFree( papszFinderLocations[nCount-1] );
    papszFinderLocations[nCount-1] = NULL;

    if( nCount == 1 )
    {
        CPLFree( papszFinderLocations );
        papszFinderLocations = NULL;
    }
}
