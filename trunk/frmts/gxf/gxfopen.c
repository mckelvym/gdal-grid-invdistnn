/******************************************************************************
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
 * gxfopen.c: 
 *
 * Supporting routines for reading Geosoft GXF files.
 *
 * $Log$
 * Revision 1.3  1998/12/14 04:52:06  warmerda
 * Added projection support, fixed bugs in compressed image support.
 *
 * Revision 1.2  1998/12/06 02:54:10  warmerda
 * Raw read access now working.
 *
 * Revision 1.1  1998/12/02 19:37:04  warmerda
 * New
 */

#include <ctype.h>
#include "gxfopen.h"


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

/************************************************************************/
/*                            CPLReadLine()                             */
/*                                                                      */
/*      Read a line of text from the given file handle, taking care     */
/*      to capture CR and/or LF and strip off ... equivelent of         */
/*      DKReadLine().  Pointer to an internal buffer is returned.       */
/*      The application shouldn't free it, or depend on it's value      */
/*      past the next call to CPLReadLine()                             */
/************************************************************************/

const char *CPLReadLine( FILE * fp )

{
    static char	*pszRLBuffer = NULL;
    static int	nRLBufferSize = 0;
    int		nLength;

/* -------------------------------------------------------------------- */
/*      Allocate our working buffer.  Eventually this should grow as    */
/*      needed ... we will implement that aspect later.                 */
/* -------------------------------------------------------------------- */
    if( nRLBufferSize < 512 )
    {
        nRLBufferSize = 512;
        pszRLBuffer = (char *) CPLRealloc(pszRLBuffer, nRLBufferSize);
    }

/* -------------------------------------------------------------------- */
/*      Do the actual read.                                             */
/* -------------------------------------------------------------------- */
    if( VSIFGets( pszRLBuffer, nRLBufferSize, fp ) == NULL )
        return NULL;

/* -------------------------------------------------------------------- */
/*      Clear CR and LF off the end.                                    */
/* -------------------------------------------------------------------- */
    nLength = strlen(pszRLBuffer);
    if( nLength > 0
        && (pszRLBuffer[nLength-1] == 10 || pszRLBuffer[nLength-1] == 13) )
    {
        pszRLBuffer[--nLength] = '\0';
    }
    
    if( nLength > 0
        && (pszRLBuffer[nLength-1] == 10 || pszRLBuffer[nLength-1] == 13) )
    {
        pszRLBuffer[--nLength] = '\0';
    }

    return( pszRLBuffer );
}

/************************************************************************/
/*                         GXFReadHeaderValue()                         */
/*                                                                      */
/*      Read one entry from the file header, and return it and it's     */
/*      value in clean form.                                            */
/************************************************************************/

static char **GXFReadHeaderValue( FILE * fp, char * pszHTitle )

{
    const char	*pszLine;
    char	**papszReturn = NULL;
    int		i;
    
/* -------------------------------------------------------------------- */
/*      Try to read a line.  If we fail or if this isn't a proper       */
/*      header value then return the failure.                           */
/* -------------------------------------------------------------------- */
    pszLine = CPLReadLine( fp );
    if( pszLine == NULL )
    {
        strcpy( pszHTitle, "#EOF" );
        return( NULL );
    }

/* -------------------------------------------------------------------- */
/*      Extract the title.  It should be terminated by some sort of     */
/*      white space.                                                    */
/* -------------------------------------------------------------------- */
    for( i = 0; !isspace(pszLine[i]) && pszLine[i] != '\0' && i < 70; i++ ) {}

    strncpy( pszHTitle, pszLine, i );
    pszHTitle[i] = '\0';

/* -------------------------------------------------------------------- */
/*      If this is #GRID, then return ... we are at the end of the      */
/*      header.                                                         */
/* -------------------------------------------------------------------- */
    if( EQUAL(pszHTitle,"#GRID") )
        return NULL;

/* -------------------------------------------------------------------- */
/*      Skip white space.                                               */
/* -------------------------------------------------------------------- */
    while( isspace(pszLine[i]) )
        i++;

/* -------------------------------------------------------------------- */
/*    If we have reached the end of the line, try to read another line. */
/* -------------------------------------------------------------------- */
    if( pszLine[i] == '\0' )
    {
        pszLine = CPLReadLine( fp );
        if( pszLine == NULL )
        {
            strcpy( pszHTitle, "#EOF" );
            return( NULL );
        }
        i = 0;
    }

/* -------------------------------------------------------------------- */
/*      Keeping adding the value stuff as new lines till we reach a     */
/*      `#' mark at the beginning of a new line.                        */
/* -------------------------------------------------------------------- */
    do {
        int		nNextChar;
        char		*pszTrimmedLine;

        pszTrimmedLine = CPLStrdup( pszLine );

        for( i = strlen(pszLine)-1; i >= 0 && pszLine[i] == ' '; i-- ) 
            pszTrimmedLine[i] = '\0';
        
        papszReturn = CSLAddString( papszReturn, pszTrimmedLine );
        CPLFree( pszTrimmedLine );
        
        nNextChar = VSIFGetc( fp );
        VSIUngetc( nNextChar, fp );
        
        if( nNextChar == '#' )
            pszLine = NULL;
        else
            pszLine = CPLReadLine( fp );
    } while( pszLine != NULL );

    return( papszReturn );
}

/************************************************************************/
/*                              GXFOpen()                               */
/*                                                                      */
/*      Open a GXF file, and collect contents of the header.            */
/************************************************************************/

GXFHandle GXFOpen( const char * pszFilename )

{
    FILE	*fp;
    GXFInfo_t	*psGXF;
    char	szTitle[71];
    char	**papszList;

/* -------------------------------------------------------------------- */
/*      We open in binary to ensure that we can efficiently seek()      */
/*      to any location when reading scanlines randomly.  If we         */
/*      opened as text we might still be able to seek(), but I          */
/*      believe that on Windows, the C library has to read through      */
/*      all the data to find the right spot taking into account DOS     */
/*      CRs.                                                            */
/* -------------------------------------------------------------------- */
    fp = VSIFOpen( pszFilename, "rb" );

    if( fp == NULL )
    {
        /* how to effectively communicate this error out? */
        CPLError( CE_Failure, CPLE_OpenFailed,
                  "Unable to open file: %s\n", pszFilename );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Create the GXF Information object.                              */
/* -------------------------------------------------------------------- */
    psGXF = (GXFInfo_t *) VSICalloc( sizeof(GXFInfo_t), 1 );
    psGXF->fp = fp;
    psGXF->dfTransformScale = 1.0;
    psGXF->dfUnitToMeter = 1.0;
    
/* -------------------------------------------------------------------- */
/*      Read the header, one line at a time.                            */
/* -------------------------------------------------------------------- */
    while( (papszList = GXFReadHeaderValue( fp, szTitle)) != NULL )
    {
        if( EQUALN(szTitle,"#TITL",5) )
        {
            psGXF->pszTitle = CPLStrdup( papszList[0] );
        }
        else if( EQUALN(szTitle,"#POIN",5) )
        {
            psGXF->nRawXSize = atoi(papszList[0]);
        }
        else if( EQUALN(szTitle,"#ROWS",5) )
        {
            psGXF->nRawYSize = atoi(papszList[0]);
        }
        else if( EQUALN(szTitle,"#PTSE",5) )
        {
            psGXF->dfXPixelSize = atof(papszList[0]);
        }
        else if( EQUALN(szTitle,"#RWSE",5) )
        {
            psGXF->dfYPixelSize = atof(papszList[0]);
        }
        else if( EQUALN(szTitle,"#DUMM",5) )
        {
            strcpy( psGXF->szDummy, papszList[0] );
            psGXF->dfSetDummyTo = atof(papszList[0]);
        }
        else if( EQUALN(szTitle,"#XORI",5) )
        {
            psGXF->dfXOrigin = atof(papszList[0]);
        }
        else if( EQUALN(szTitle,"#YORI",5) )
        {
            psGXF->dfYOrigin = atof(papszList[0]);
        }
        else if( EQUALN(szTitle,"#ZMIN",5) )
        {
            psGXF->dfZMinimum = atof(papszList[0]);
        }
        else if( EQUALN(szTitle,"#ZMAX",5) )
        {
            psGXF->dfZMaximum = atof(papszList[0]);
        }
        else if( EQUALN(szTitle,"#SENS",5) )
        {
            psGXF->nSense = atoi(papszList[0]);
        }
        else if( EQUALN(szTitle,"#MAP_PROJECTION",5) )
        {
            psGXF->papszMapProjection = papszList;
            papszList = NULL;
        }
        else if( EQUALN(szTitle,"#TRAN",5) )
        {
            char	**papszFields;

            papszFields = CSLTokenizeStringComplex( papszList[0], ", ",
                                                    TRUE, TRUE );

            if( CSLCount(papszFields) > 1 )
            {
                psGXF->dfTransformScale = atof(papszFields[0]);
                psGXF->dfTransformOffset = atof(papszFields[1]);
            }

            if( CSLCount(papszFields) > 2 )
                psGXF->pszTransformName = CPLStrdup( papszFields[2] );

            CSLDestroy( papszFields );
        }
        else if( EQUALN(szTitle,"#GTYPE",5) )
        {
            psGXF->nGType = atoi(papszList[0]);
        }

        CSLDestroy( papszList );
    }

/* -------------------------------------------------------------------- */
/*      Did we find the #GRID?                                          */
/* -------------------------------------------------------------------- */
    if( !EQUALN(szTitle,"#GRID",5) )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "Didn't parse through to #GRID successfully in.\n"
                  "file `%s'.\n",
                  pszFilename );
        
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Allocate, and initialize the raw scanline offset array.         */
/* -------------------------------------------------------------------- */
    psGXF->panRawLineOffset = (long *)
        CPLCalloc( sizeof(long), psGXF->nRawXSize );

    psGXF->panRawLineOffset[0] = VSIFTell( psGXF->fp );

/* -------------------------------------------------------------------- */
/*      Update the zmin/zmax values to take into account #TRANSFORM     */
/*      information.                                                    */
/* -------------------------------------------------------------------- */
    if( psGXF->dfZMinimum != 0.0 || psGXF->dfZMaximum != 0.0 )
    {
        psGXF->dfZMinimum = (psGXF->dfZMinimum * psGXF->dfTransformScale)
            			+ psGXF->dfTransformOffset;
        psGXF->dfZMaximum = (psGXF->dfZMaximum * psGXF->dfTransformScale)
            			+ psGXF->dfTransformOffset;
    }
    
    return( (GXFHandle) psGXF );
}

/************************************************************************/
/*                           GXFParseBase90()                           */
/*                                                                      */
/*      Parse a base 90 number ... exceptions (repeat, and dummy)       */
/*      values have to be recognised outside this function.             */
/************************************************************************/

double GXFParseBase90( GXFInfo_t * psGXF, const char * pszText,
                       int bScale )

{
    int		i = 0, nValue = 0;

    while( i < psGXF->nGType )
    {
        nValue = nValue*90 + (pszText[i] - 37);
        i++;
    }

    if( bScale ) 
        return( (nValue * psGXF->dfTransformScale) + psGXF->dfTransformOffset);
    else
        return( nValue );
}


/************************************************************************/
/*                       GXFReadRawScanlineFrom()                       */
/************************************************************************/

static int GXFReadRawScanlineFrom( GXFInfo_t * psGXF, long iOffset,
                                   long * pnNewOffset, double * padfLineBuf )

{
    const char	*pszLine;
    int		nValuesRead = 0, nValuesSought = psGXF->nRawXSize;
    
    VSIFSeek( psGXF->fp, iOffset, SEEK_SET );

    while( nValuesRead < nValuesSought )
    {
        pszLine = CPLReadLine( psGXF->fp );
        if( pszLine == NULL )
            break;

/* -------------------------------------------------------------------- */
/*      Uncompressed case.                                              */
/* -------------------------------------------------------------------- */
        if( psGXF->nGType == 0 )
        {
            /* we could just tokenize the line, but that's pretty expensive.
               Instead I will parse on white space ``by hand''. */
            while( *pszLine != '\0' && nValuesRead < nValuesSought )
            {
                int		i;
                
                /* skip leading white space */
                for( ; isspace(*pszLine); pszLine++ ) {}

                /* Skip the data value (non white space) */
                for( i = 0; pszLine[i] != '\0' && !isspace(pszLine[i]); i++) {}

                if( strncmp(pszLine,psGXF->szDummy,i) == 0 )
                {
                    padfLineBuf[nValuesRead++] = psGXF->dfSetDummyTo;
                }
                else
                {
                    padfLineBuf[nValuesRead++] = atof(pszLine);
                }

                /* skip further whitespace */
                for( pszLine += i; isspace(*pszLine); pszLine++ ) {}
            }
        }

/* -------------------------------------------------------------------- */
/*      Compressed case.                                                */
/* -------------------------------------------------------------------- */
        else
        {
            while( *pszLine != '\0' && nValuesRead < nValuesSought )
            {
                if( pszLine[0] == '!' )
                {
                    padfLineBuf[nValuesRead++] = psGXF->dfSetDummyTo;
                }
                else if( pszLine[0] == '"' )
                {
                    int		nCount, i;
                    double	dfValue;

                    pszLine += psGXF->nGType;
                    if( strlen(pszLine) < psGXF->nGType )
                        pszLine = CPLReadLine( psGXF->fp );
                    
                    nCount = (int) GXFParseBase90( psGXF, pszLine, FALSE);
                    pszLine += psGXF->nGType;
                    
                    if( strlen(pszLine) < psGXF->nGType )
                        pszLine = CPLReadLine( psGXF->fp );
                    
                    if( *pszLine == '!' )
                        dfValue = psGXF->dfSetDummyTo;
                    else
                        dfValue = GXFParseBase90( psGXF, pszLine, TRUE );

                    CPLAssert( nValuesRead + nCount <= nValuesSought );
                    
                    for( i=0; i < nCount && nValuesRead < nValuesSought; i++ )
                        padfLineBuf[nValuesRead++] = dfValue;
                }
                else
                {
                    padfLineBuf[nValuesRead++] =
                        GXFParseBase90( psGXF, pszLine, TRUE );
                }

                pszLine += psGXF->nGType;
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      Return the new offset, if requested.                            */
/* -------------------------------------------------------------------- */
    if( pnNewOffset != NULL )
    {
        *pnNewOffset = VSIFTell( psGXF->fp );
    }

    return CE_None;
}

/************************************************************************/
/*                         GXFGetRawScanline()                          */
/*                                                                      */
/*      Read a raw scanline based on offset from beginning of file.     */
/*      This isn't attempting to account for the #SENSE flag.           */
/************************************************************************/

CPLErr GXFGetRawScanline( GXFHandle hGXF, int iScanline, double * padfLineBuf )

{
    GXFInfo_t	*psGXF = (GXFInfo_t *) hGXF;
    CPLErr	nErr;
    
/* -------------------------------------------------------------------- */
/*      Validate scanline.                                              */
/* -------------------------------------------------------------------- */
    if( iScanline < 0 || iScanline >= psGXF->nRawXSize )
    {
        CPLError( CE_Failure, CPLE_IllegalArg,
                  "GXFGetRawScanline(): Scanline `%d' does not exist.\n",
                  iScanline );
        return CE_Failure;
    }

/* -------------------------------------------------------------------- */
/*      If we don't have the requested scanline, fetch preceeding       */
/*      scanlines to find the pointer to this scanline.                 */
/* -------------------------------------------------------------------- */
    if( psGXF->panRawLineOffset[iScanline] == 0 )
    {
        int		i;
        
        CPLAssert( iScanline > 0 );

        for( i = 0; i < iScanline; i++ )
        {
            if( psGXF->panRawLineOffset[i+1] == 0 )
            {
                nErr = GXFGetRawScanline( hGXF, i, padfLineBuf );
                if( nErr != CE_None )
                    return( nErr );
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      Get this scanline, and update the offset for the next line.     */
/* -------------------------------------------------------------------- */
    nErr = GXFReadRawScanlineFrom( psGXF, psGXF->panRawLineOffset[iScanline],
                                   psGXF->panRawLineOffset+iScanline+1,
                                   padfLineBuf );

    return nErr;
}

/************************************************************************/
/*                             GXFGetRawInfo()                          */
/************************************************************************/

CPLErr GXFGetRawInfo( GXFHandle hGXF, int *pnXSize, int *pnYSize,
                      int * pnSense )

{
    GXFInfo_t	*psGXF = (GXFInfo_t *) hGXF;

    if( pnXSize != NULL )
        *pnXSize = psGXF->nRawXSize;

    if( pnYSize != NULL )
        *pnYSize = psGXF->nRawYSize;

    if( pnSense != NULL )
        *pnSense = psGXF->nSense;

    return( CE_None );
}

/************************************************************************/
/*                        GXFGetMapProjection()                         */
/*                                                                      */
/*      Return the lines related to the map projection.  It is up to    */
/*      the caller to parse them and interprete.  The return result     */
/*      will be NULL if not projection item was found.                  */
/************************************************************************/

char **GXFGetMapProjection( GXFHandle hGXF )

{
    return( ((GXFInfo_t *) hGXF)->papszMapProjection );
}

/************************************************************************/
/*                      GXFGetMapDatumTransform()                       */
/*                                                                      */
/*      Return the lines related to the datum transformation.           */
/************************************************************************/

char **GXFGetMapDatumTransform( GXFHandle hGXF )

{
    return( ((GXFInfo_t *) hGXF)->papszMapDatumTransform );
}

/************************************************************************/
/*                     GXFGetMapProjectionAsPROJ4()                     */
/*                                                                      */
/*      Return the map projection definition in PROJ.4 format.          */
/************************************************************************/

char *GXFGetMapProjectionAsPROJ4( GXFHandle hGXF )

{
    GXFInfo_t	*psGXF = (GXFInfo_t *) hGXF;
    char	**papszMethods;
    char	szPROJ4[512];

/* -------------------------------------------------------------------- */
/*      If there was nothing in the file return "unknown".              */
/* -------------------------------------------------------------------- */
    if( CSLCount(psGXF->papszMapProjection) < 3 )
        return( CPLStrdup( "unknown" ) );

/* -------------------------------------------------------------------- */
/*      Parse the third line, looking for known projection methods.     */
/* -------------------------------------------------------------------- */
    szPROJ4[0] = '\0';
    
    papszMethods = CSLTokenizeStringComplex( psGXF->papszMapProjection[2], ",",
                                             TRUE, TRUE );

#ifdef DBMALLOC
    malloc_chain_check(1);
#endif    
    
    if( CSLCount(papszMethods) < 1 )
    {
        CSLDestroy(papszMethods);
        return( CPLStrdup( "unknown" ) );
    }
    
    if( EQUAL(papszMethods[0],"Geographic") )
    {
        strcat( szPROJ4, "+proj=longlat" );
    }

    else if( EQUAL(papszMethods[0],"Lambert Conic Conformal (1SP)")
             && CSLCount(papszMethods) > 5 )
    {
        /* notdef: It isn't clear that this 1SP + scale method is even
           supported by PROJ.4 */
        
        strcat( szPROJ4, "+proj=lcc" );

        strcat( szPROJ4, " +lat_0=" );
        strcat( szPROJ4, papszMethods[1] );

        strcat( szPROJ4, " +lon_0=" );
        strcat( szPROJ4, papszMethods[2] );

        strcat( szPROJ4, " +k=" );
        strcat( szPROJ4, papszMethods[3] );
        
        strcat( szPROJ4, " +x_0=" );
        strcat( szPROJ4, papszMethods[4] );

        strcat( szPROJ4, " +y_0=" );
        strcat( szPROJ4, papszMethods[5] );
    }
    
    else if( EQUAL(papszMethods[0],"Lambert Conic Conformal (2SP)")
             || EQUAL(papszMethods[0],"Lambert Conformal (2SP Belgium)") )
    {
        /* notdef: Note we are apparently losing whatever makes the
           Belgium variant different than normal LCC, but hopefully
           they are close! */
        
        strcat( szPROJ4, "+proj=lcc" );

        if( CSLCount(papszMethods) > 1 )
        {
            strcat( szPROJ4, " +lat_1=" );
            strcat( szPROJ4, papszMethods[1] );
        }

        if( CSLCount(papszMethods) > 2 )
        {
            strcat( szPROJ4, " +lat_2=" );
            strcat( szPROJ4, papszMethods[2] );
        }

        if( CSLCount(papszMethods) > 3 )
        {
            strcat( szPROJ4, " +lat_0=" );
            strcat( szPROJ4, papszMethods[3] );
        }

        if( CSLCount(papszMethods) > 4 )
        {
            strcat( szPROJ4, " +lon_0=" );
            strcat( szPROJ4, papszMethods[4] );
        }

        if( CSLCount(papszMethods) > 5 )
        {
            strcat( szPROJ4, " +x_0=" );
            strcat( szPROJ4, papszMethods[5] );
        }

        if( CSLCount(papszMethods) > 6 )
        {
            strcat( szPROJ4, " +y_0=" );
            strcat( szPROJ4, papszMethods[6] );
        }
    }
    
    else if( EQUAL(papszMethods[0],"Mercator (1SP)")
             && CSLCount(papszMethods) > 5 )
    {
        /* notdef: it isn't clear that +proj=merc support a scale of other 
           than 1.0 in PROJ.4 */
        
        strcat( szPROJ4, "+proj=merc" );

        strcat( szPROJ4, " +lat_ts=" );
        strcat( szPROJ4, papszMethods[1] );

        strcat( szPROJ4, " +lon_0=" );
        strcat( szPROJ4, papszMethods[2] );

        strcat( szPROJ4, " +k=" );
        strcat( szPROJ4, papszMethods[3] );
        
        strcat( szPROJ4, " +x_0=" );
        strcat( szPROJ4, papszMethods[4] );

        strcat( szPROJ4, " +y_0=" );
        strcat( szPROJ4, papszMethods[5] );
    }
    
    else if( EQUAL(papszMethods[0],"Mercator (2SP)")
             && CSLCount(papszMethods) > 4 )
    {
        /* notdef: it isn't clear that +proj=merc support a scale of other 
           than 1.0 in PROJ.4 */
        
        strcat( szPROJ4, "+proj=merc" );

        strcat( szPROJ4, " +lat_ts=" );
        strcat( szPROJ4, papszMethods[1] );

        strcat( szPROJ4, " +lon_0=" );
        strcat( szPROJ4, papszMethods[2] );

        strcat( szPROJ4, " +x_0=" );
        strcat( szPROJ4, papszMethods[3] );

        strcat( szPROJ4, " +y_0=" );
        strcat( szPROJ4, papszMethods[4] );
    }
    
    else if( EQUAL(papszMethods[0],"Hotine Oblique Mercator") 
             && CSLCount(papszMethods) > 7 )
    {
        /* Note that only the second means of specifying omerc is supported
           by this code in GXF. */
        strcat( szPROJ4, "+proj=omerc" );

        strcat( szPROJ4, " +lat_0=" );
        strcat( szPROJ4, papszMethods[1] );
        
        strcat( szPROJ4, " +lonc=" );
        strcat( szPROJ4, papszMethods[2] );
        
        strcat( szPROJ4, " +alpha=" );
        strcat( szPROJ4, papszMethods[3] );

        if( atof(papszMethods[4]) < 0.00001 )
        {
            strcat( szPROJ4, " +not_rot" );
        }
        else
        {
            if( atof(papszMethods[4]) + atof(papszMethods[3]) < 0.00001 )
                /* ok */;
            else
                /* notdef: no way to specify arbitrary angles! */;
        }

        strcat( szPROJ4, " +k=" );
        strcat( szPROJ4, papszMethods[5] );

        strcat( szPROJ4, " +x_0=" );
        strcat( szPROJ4, papszMethods[6] );

        strcat( szPROJ4, " +y_0=" );
        strcat( szPROJ4, papszMethods[7] );
    }

    else if( EQUAL(papszMethods[0],"Laborde Oblique Mercator")
             && CSLCount(papszMethods) > 6 )
    {
        strcat( szPROJ4, "+proj=labrd" );

        strcat( szPROJ4, " +lat_0=" );
        strcat( szPROJ4, papszMethods[1] );
        
        strcat( szPROJ4, " +lon_0=" );
        strcat( szPROJ4, papszMethods[2] );
        
        strcat( szPROJ4, " +azi=" );
        strcat( szPROJ4, papszMethods[3] );

        strcat( szPROJ4, " +k=" );
        strcat( szPROJ4, papszMethods[4] );

        strcat( szPROJ4, " +x_0=" );
        strcat( szPROJ4, papszMethods[5] );

        strcat( szPROJ4, " +y_0=" );
        strcat( szPROJ4, papszMethods[6] );
    }
    
    else if( EQUAL(papszMethods[0],"New Zealand Map Grid")
             && CSLCount(papszMethods) > 4 )
    {
        strcat( szPROJ4, "+proj=nzmg" );

        strcat( szPROJ4, " +lat_0=" );
        strcat( szPROJ4, papszMethods[1] );
        
        strcat( szPROJ4, " +lon_0=" );
        strcat( szPROJ4, papszMethods[2] );
        
        strcat( szPROJ4, " +x_0=" );
        strcat( szPROJ4, papszMethods[3] );

        strcat( szPROJ4, " +y_0=" );
        strcat( szPROJ4, papszMethods[4] );
    }
    
    else if( EQUAL(papszMethods[0],"New Zealand Map Grid")
             && CSLCount(papszMethods) > 4 )
    {
        strcat( szPROJ4, "+proj=nzmg" );

        strcat( szPROJ4, " +lat_0=" );
        strcat( szPROJ4, papszMethods[1] );
        
        strcat( szPROJ4, " +lon_0=" );
        strcat( szPROJ4, papszMethods[2] );
        
        strcat( szPROJ4, " +x_0=" );
        strcat( szPROJ4, papszMethods[3] );

        strcat( szPROJ4, " +y_0=" );
        strcat( szPROJ4, papszMethods[4] );
    }
    
    else if( (EQUAL(papszMethods[0],"Oblique Stereographic")
              || EQUAL(papszMethods[0],"Polar Stereographic"))
             && CSLCount(papszMethods) > 5 )
    {
        /* there is an option to produce +lat_ts, which we ignore */
        
        strcat( szPROJ4, "+proj=stere" );

        strcat( szPROJ4, " +lat_0=" );
        strcat( szPROJ4, papszMethods[1] );
        
        strcat( szPROJ4, " +lon_0=" );
        strcat( szPROJ4, papszMethods[2] );
        
        strcat( szPROJ4, " +k=" );
        strcat( szPROJ4, papszMethods[3] );

        strcat( szPROJ4, " +x_0=" );
        strcat( szPROJ4, papszMethods[4] );

        strcat( szPROJ4, " +y_0=" );
        strcat( szPROJ4, papszMethods[5] );
    }
    
    else if( EQUAL(papszMethods[0],"Swiss Oblique Cylindrical")
             && CSLCount(papszMethods) > 4 )
    {
        /* notdef: geotiff's geo_ctrans.inc says this is the same as
           ObliqueMercator_Rosenmund, which GG's geotiff support just
           maps directly to +proj=omerc, though I find that questionable. */

        strcat( szPROJ4, "+proj=omerc" );

        strcat( szPROJ4, " +lat_0=" );
        strcat( szPROJ4, papszMethods[1] );
        
        strcat( szPROJ4, " +lonc=" );
        strcat( szPROJ4, papszMethods[2] );
        
        strcat( szPROJ4, " +x_0=" );
        strcat( szPROJ4, papszMethods[3] );

        strcat( szPROJ4, " +y_0=" );
        strcat( szPROJ4, papszMethods[4] );
    }

    else if( EQUAL(papszMethods[0],"Transverse Mercator")
             && CSLCount(papszMethods) > 5 )
    {
        /* notdef: geotiff's geo_ctrans.inc says this is the same as
           ObliqueMercator_Rosenmund, which GG's geotiff support just
           maps directly to +proj=omerc, though I find that questionable. */

        strcat( szPROJ4, "+proj=tmerc" );

        strcat( szPROJ4, " +lat_0=" );
        strcat( szPROJ4, papszMethods[1] );
        
        strcat( szPROJ4, " +lon_0=" );
        strcat( szPROJ4, papszMethods[2] );
        
        strcat( szPROJ4, " +k=" );
        strcat( szPROJ4, papszMethods[3] );
        
        strcat( szPROJ4, " +x_0=" );
        strcat( szPROJ4, papszMethods[4] );

        strcat( szPROJ4, " +y_0=" );
        strcat( szPROJ4, papszMethods[5] );
    }

    else if( EQUAL(papszMethods[0],"Transverse Mercator (South Oriented)")
             && CSLCount(papszMethods) > 5 )
    {
        /* notdef: I don't know how south oriented is different from
           normal, and I don't find any mention of it in Geotiff;s geo_ctrans.
           Translating as tmerc, but that is presumably wrong. */

        strcat( szPROJ4, "+proj=tmerc" );

        strcat( szPROJ4, " +lat_0=" );
        strcat( szPROJ4, papszMethods[1] );
        
        strcat( szPROJ4, " +lon_0=" );
        strcat( szPROJ4, papszMethods[2] );
        
        strcat( szPROJ4, " +k=" );
        strcat( szPROJ4, papszMethods[3] );
        
        strcat( szPROJ4, " +x_0=" );
        strcat( szPROJ4, papszMethods[4] );

        strcat( szPROJ4, " +y_0=" );
        strcat( szPROJ4, papszMethods[5] );
    }

    else if( EQUAL(papszMethods[0],"Equidistant Conic")
             && CSLCount(papszMethods) > 6 )
    {
        strcat( szPROJ4, "+proj=eqdc" );

        strcat( szPROJ4, " +lat_1=" );
        strcat( szPROJ4, papszMethods[1] );
        
        strcat( szPROJ4, " +lat_2=" );
        strcat( szPROJ4, papszMethods[2] );
        
        strcat( szPROJ4, " +lat_0=" );
        strcat( szPROJ4, papszMethods[3] );
        
        strcat( szPROJ4, " +lon_0=" );
        strcat( szPROJ4, papszMethods[4] );
        
        strcat( szPROJ4, " +x_0=" );
        strcat( szPROJ4, papszMethods[5] );

        strcat( szPROJ4, " +y_0=" );
        strcat( szPROJ4, papszMethods[6] );
    }

    else if( EQUAL(papszMethods[0],"Polyconic") 
             && CSLCount(papszMethods) > 5 )
    {
        strcat( szPROJ4, "+proj=poly" );

        strcat( szPROJ4, " +lat_0=" );
        strcat( szPROJ4, papszMethods[1] );
        
        strcat( szPROJ4, " +lon_0=" );
        strcat( szPROJ4, papszMethods[2] );

#ifdef notdef
        /*not supported by PROJ.4 */
        strcat( szPROJ4, " +k=" );
        strcat( szPROJ4, papszMethods[3] );
#endif
        strcat( szPROJ4, " +x_0=" ); 
        strcat( szPROJ4, papszMethods[4] );

        strcat( szPROJ4, " +y_0=" );
        strcat( szPROJ4, papszMethods[5] );
    }

    else
    {
        strcat( szPROJ4, "unknown" );
    }

/* -------------------------------------------------------------------- */
/*          Now we should try to get the ellipsoid parameters.          */
/* -------------------------------------------------------------------- */
    strcat( szPROJ4, " +ellps=clrk66" );

    CSLDestroy( papszMethods );
    
    return( CPLStrdup( szPROJ4 ) );
}


/************************************************************************/
/*                         GXFGetRawPosition()                          */
/*                                                                      */
/*      Get the raw grid positioning information.  Return CE_Failure    */
/*      if no positioning info was available.                           */
/************************************************************************/

CPLErr GXFGetRawPosition( GXFHandle hGXF,
                          double * pdfXOrigin, double * pdfYOrigin,
                          double * pdfXPixelSize, double * pdfYPixelSize,
                          double * pdfRotation )

{
    GXFInfo_t	*psGXF = (GXFInfo_t *) hGXF;
    
    if( pdfXOrigin != NULL )
        *pdfXOrigin = psGXF->dfXOrigin;
    if( pdfYOrigin != NULL )
        *pdfYOrigin = psGXF->dfYOrigin;
    if( pdfXPixelSize != NULL )
        *pdfXPixelSize = psGXF->dfXPixelSize;
    if( pdfYPixelSize != NULL )
        *pdfYPixelSize = psGXF->dfYPixelSize;
    if( pdfRotation != NULL )
        *pdfRotation = psGXF->dfRotation;

    if( psGXF->dfXOrigin == 0.0 && psGXF->dfYOrigin == 0.0
        && psGXF->dfXPixelSize == 0.0 && psGXF->dfYPixelSize == 0.0 )
        return( CE_Failure );
    else
        return( CE_None );
}
