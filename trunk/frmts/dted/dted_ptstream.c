/******************************************************************************
 * $Id$
 *
 * Project:  DTED Translator
 * Purpose:  DTED Point Stream Writer.
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
 * $Log$
 * Revision 1.1  2001/11/21 19:51:34  warmerda
 * New
 *
 */

#include "dted_api.h"

typedef struct {
    DTEDInfo *psInfo;

    GInt16 **papanProfiles;
} DTEDCachedFile;

typedef struct {
    int nLevel;
    char *pszPath;

    int nOpenFiles;
    DTEDCachedFile *pasCF;

    int nLastFile;
} DTEDPtStream;

/************************************************************************/
/*                         DTEDCreatePtStream()                         */
/************************************************************************/

void *DTEDCreatePtStream( const char *pszPath, int nLevel )

{
    DTEDPtStream *psStream;

    psStream = (DTEDPtStream *) CPLCalloc( sizeof(DTEDPtStream), 1 );
    psStream->nLevel = nLevel;
    psStream->pszPath = CPLStrdup( pszPath );
    psStream->nOpenFiles = 0;
    psStream->pasCF = NULL;
    psStream->nLastFile = -1;

    return (void *) psStream;
}

/************************************************************************/
/*                            DTEDWritePt()                             */
/*                                                                      */
/*      Write a single point out, creating a new file if necessary      */
/*      to hold it.                                                     */
/************************************************************************/

int DTEDWritePt( void *hStream, double dfLong, double dfLat, double dfElev )

{
    DTEDPtStream *psStream = (DTEDPtStream *) hStream;
    int          i, iProfile, iRow;
    DTEDInfo     *psInfo;

/* -------------------------------------------------------------------- */
/*      Is the last file used still applicable?                         */
/* -------------------------------------------------------------------- */
    if( psStream->nLastFile != -1 )
    {
        psInfo = psStream->pasCF[psStream->nLastFile].psInfo;

        if( dfLat > psInfo->dfULCornerY
            || dfLat < psInfo->dfULCornerY - 1.0
            || dfLong < psInfo->dfULCornerX
            || dfLong > psInfo->dfULCornerX + 1.0 )
            psStream->nLastFile = -1;
    }

/* -------------------------------------------------------------------- */
/*      Search for the file to write to.                                */
/* -------------------------------------------------------------------- */
    for( i = 0; i < psStream->nOpenFiles && psStream->nLastFile == -1; i++ )
    {
        psInfo = psStream->pasCF[i].psInfo;

        if( !(dfLat > psInfo->dfULCornerY
              || dfLat < psInfo->dfULCornerY - 1.0
              || dfLong < psInfo->dfULCornerX
              || dfLong > psInfo->dfULCornerX + 1.0) )
        {
            psStream->nLastFile = i;
        }
    }

/* -------------------------------------------------------------------- */
/*      If none found, create a new file.                               */
/* -------------------------------------------------------------------- */
    if( psStream->nLastFile == -1 )
    {
        DTEDInfo	*psInfo;
        char	        szFile[128];
        char		chNSHemi, chEWHemi;
        int		nCrLong, nCrLat;
        char		*pszFullFilename;
        const char      *pszError;

        nCrLong = (int) floor(dfLong);
        nCrLat = (int) floor(dfLat);

        /* work out filename */
        if( nCrLat < 0 )
            chNSHemi = 's';
        else
            chNSHemi = 'n';

        if( nCrLong < 0 )
            chEWHemi = 'w';
        else
            chEWHemi = 'e';

        sprintf( szFile, "%c%03d%c%03d.dt%d", 
                 chEWHemi, ABS(nCrLong), chNSHemi, ABS(nCrLat),
                 psStream->nLevel );

        pszFullFilename = 
            CPLStrdup(CPLFormFilename( psStream->pszPath, szFile, NULL ));

        /* create the dted file */
        pszError = DTEDCreate( pszFullFilename, psStream->nLevel, 
                               nCrLat, nCrLong );
        if( pszError != NULL )
            return FALSE;

        psInfo = DTEDOpen( pszFullFilename, "rb", FALSE );

        CPLFree( pszFullFilename );

        if( psInfo == NULL )
            return FALSE;

        /* add cached file to stream */
        psStream->nOpenFiles++;
        psStream->pasCF = 
            CPLRealloc(psStream->pasCF, 
                       sizeof(DTEDCachedFile)*psStream->nOpenFiles);

        psStream->pasCF[psStream->nOpenFiles-1].psInfo = psInfo;
        psStream->pasCF[psStream->nOpenFiles-1].papanProfiles =
            CPLCalloc(sizeof(GInt16*),psInfo->nXSize);

        psStream->nLastFile = psStream->nOpenFiles-1;
    }

/* -------------------------------------------------------------------- */
/*	Determine what profile this belongs in, and initialize the	*/
/*	profile if it doesn't already exist.				*/
/* -------------------------------------------------------------------- */
    psInfo = psStream->pasCF[psStream->nLastFile].psInfo;

    iProfile = (int) ((dfLong - psInfo->dfULCornerX) / psInfo->dfPixelSizeX);
    iProfile = MAX(0,MIN(psInfo->nXSize-1,iProfile));

    if( psStream->pasCF[psStream->nLastFile].papanProfiles[iProfile] == NULL )
    {
        psStream->pasCF[psStream->nLastFile].papanProfiles[iProfile] = 
            CPLMalloc(sizeof(GInt16) * psInfo->nYSize);

        memset( psStream->pasCF[psStream->nLastFile].papanProfiles[iProfile], 
                0xff, sizeof(GInt16) * psInfo->nYSize );
    }

/* -------------------------------------------------------------------- */
/*      Establish where we fit in the profile.                          */
/* -------------------------------------------------------------------- */
    iRow = (int) ((psInfo->dfULCornerY-dfLat) / psInfo->dfPixelSizeY);
    iRow = MAX(0,MIN(psInfo->nYSize-1,iRow));

    psStream->pasCF[psStream->nLastFile].papanProfiles[iProfile][iRow] = 
        (GInt16) dfElev;
    
    return TRUE;
}

/************************************************************************/
/*                         DTEDClosePtStream()                          */
/************************************************************************/

void DTEDClosePtStream( void *hStream )

{
    DTEDPtStream *psStream = (DTEDPtStream *) hStream;
    int	          iFile;

/* -------------------------------------------------------------------- */
/*      Flush all DTED files.                                           */
/* -------------------------------------------------------------------- */
    for( iFile = 0; iFile < psStream->nOpenFiles; iFile++ )
    {
        int             iProfile;
        DTEDCachedFile  *psCF = psStream->pasCF + iFile;

        for( iProfile = 0; iProfile < psCF->psInfo->nXSize; iProfile++ )
        {
            if( psCF->papanProfiles[iProfile] != NULL )
            {
                DTEDWriteProfile( psCF->psInfo, iProfile, 
                                  psCF->papanProfiles[iProfile] );
                CPLFree( psCF->papanProfiles[iProfile] );
            }
        }

        CPLFree( psCF->papanProfiles );

        DTEDClose( psCF->psInfo );
    }

/* -------------------------------------------------------------------- */
/*      Final cleanup.                                                  */
/* -------------------------------------------------------------------- */
    CPLFree( psStream->pasCF );
    CPLFree( psStream->pszPath );
    CPLFree( psStream );
}
