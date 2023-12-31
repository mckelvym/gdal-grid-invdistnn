/******************************************************************************
 * $Id$
 *
 * Project:  DTED Translator
 * Purpose:  Implementation of DTED/CDED access functions.
 * Author:   Frank Warmerdam, warmerda@home.com
 *
 ******************************************************************************
 * Copyright (c) 1999, Frank Warmerdam
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
 * Revision 1.12  2003/05/30 16:17:21  warmerda
 * fix warnings with casting and unused parameters
 *
 * Revision 1.11  2003/02/26 15:30:41  warmerda
 * Modified reading code to treat any large negative values as if negative
 * values are being stored in twos complement form, as per
 * http://bugzilla.remotesensing.org/show_bug.cgi?id=286.
 *
 * Revision 1.10  2002/03/05 14:26:01  warmerda
 * expanded tabs
 *
 * Revision 1.9  2002/02/15 18:26:29  warmerda
 * fixed error code
 *
 * Revision 1.8  2002/01/28 18:17:33  warmerda
 * fixed setting of nUHLOffset
 *
 * Revision 1.7  2002/01/26 05:51:40  warmerda
 * added metadata read/write support
 *
 * Revision 1.6  2001/11/13 15:43:41  warmerda
 * preliminary dted creation working
 *
 * Revision 1.5  2001/07/18 04:51:56  warmerda
 * added CPL_CVSID
 *
 * Revision 1.4  2000/07/07 14:20:57  warmerda
 * fixed AVOID_CPL support
 *
 * Revision 1.3  2000/01/24 13:58:25  warmerda
 * support rb access
 *
 * Revision 1.2  2000/01/12 19:25:57  warmerda
 * Dale Lutz provided fixes to avoid calling DTEDGetField() more than
 * onces within an expression, and to ensure values evaluated with
 * atoi() don't have leading zeros to avoid treatment as octal.
 *
 * Revision 1.1  1999/12/07 18:01:28  warmerda
 * New
 */

#include "dted_api.h"

CPL_CVSID("$Id$");

/************************************************************************/
/*                            DTEDGetField()                            */
/*                                                                      */
/*      Extract a field as a zero terminated string.  Address is        */
/*      deliberately 1 based so the getfield arguments will be the      */
/*      same as the numbers in the file format specification.           */
/************************************************************************/

static
const char *DTEDGetField( const char *pachRecord, int nStart, int nSize )

{
    static char szResult[81];

    CPLAssert( nSize < sizeof(szResult) );
    memcpy( szResult, pachRecord + nStart - 1, nSize );
    szResult[nSize] = '\0';

    return szResult;
}

/************************************************************************/
/*                         StripLeadingZeros()                          */
/*                                                                      */
/*      Return a pointer to the first non-zero character in BUF.        */
/*      BUF must be null terminated.                                    */
/*      If buff is all zeros, then it will point to the last non-zero   */
/************************************************************************/

static const char* stripLeadingZeros(const char* buf)
{
    const char* ptr = buf;

    /* Go until we run out of characters  or hit something non-zero */

    while( *ptr == '0' && *(ptr+1) != '\0' )
    {
        ptr++;
    }

    return ptr;
}

/************************************************************************/
/*                              DTEDOpen()                              */
/************************************************************************/

DTEDInfo * DTEDOpen( const char * pszFilename,
                     const char * pszAccess,
                     int bTestOpen )

{
    FILE        *fp;
    char        achRecord[DTED_UHL_SIZE];
    DTEDInfo    *psDInfo = NULL;
    double      dfLLOriginX, dfLLOriginY;
    int deg = 0;
    int min = 0;
    int sec = 0;

/* -------------------------------------------------------------------- */
/*      Open the physical file.                                         */
/* -------------------------------------------------------------------- */
    if( EQUAL(pszAccess,"r") || EQUAL(pszAccess,"rb") )
        pszAccess = "rb";
    else
        pszAccess = "r+b";
    
    fp = VSIFOpen( pszFilename, pszAccess );

    if( fp == NULL )
    {
#ifndef AVOID_CPL
        if( !bTestOpen )
        {
            CPLError( CE_Failure, CPLE_AppDefined,
                      "Failed to open file %s.",
                      pszFilename );
        }
#endif

        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Read, trying to find the UHL record.  Skip VOL or HDR           */
/*      records if they are encountered.                                */
/* -------------------------------------------------------------------- */
    do
    {
        if( VSIFRead( achRecord, 1, DTED_UHL_SIZE, fp ) != DTED_UHL_SIZE )
        {
#ifndef AVOID_CPL
            if( !bTestOpen )
                CPLError( CE_Failure, CPLE_OpenFailed,
                          "Unable to read header, %s is not DTED.",
                          pszFilename );
#endif
            VSIFClose( fp );
            return NULL;
        }

    } while( EQUALN(achRecord,"VOL",3) || EQUALN(achRecord,"HDR",3) );

    if( !EQUALN(achRecord,"UHL",3) )
    {
#ifndef AVOID_CPL
        if( !bTestOpen )
            CPLError( CE_Failure, CPLE_OpenFailed,
                      "No UHL record.  %s is not a DTED file.",
                      pszFilename );
#endif
        
        VSIFClose( fp );
        return NULL;
    }
    
/* -------------------------------------------------------------------- */
/*      Create and initialize the DTEDInfo structure.                   */
/* -------------------------------------------------------------------- */
    psDInfo = (DTEDInfo *) CPLCalloc(1,sizeof(DTEDInfo));

    psDInfo->fp = fp;

    psDInfo->bUpdate = EQUAL(pszAccess,"r+b");
    
    psDInfo->nXSize = atoi(DTEDGetField(achRecord,48,4));
    psDInfo->nYSize = atoi(DTEDGetField(achRecord,52,4));

    psDInfo->nUHLOffset = VSIFTell( fp ) - DTED_UHL_SIZE;
    psDInfo->pachUHLRecord = (char *) CPLMalloc(DTED_UHL_SIZE);
    memcpy( psDInfo->pachUHLRecord, achRecord, DTED_UHL_SIZE );

    psDInfo->nDSIOffset = VSIFTell( fp );
    psDInfo->pachDSIRecord = (char *) CPLMalloc(DTED_DSI_SIZE);
    VSIFRead( psDInfo->pachDSIRecord, 1, DTED_DSI_SIZE, fp );
    
    psDInfo->nACCOffset = VSIFTell( fp );
    psDInfo->pachACCRecord = (char *) CPLMalloc(DTED_ACC_SIZE);
    VSIFRead( psDInfo->pachACCRecord, 1, DTED_ACC_SIZE, fp );

    if( !EQUALN(psDInfo->pachDSIRecord,"DSI",3)
        || !EQUALN(psDInfo->pachACCRecord,"ACC",3) )
    {
#ifndef AVOID_CPL
        CPLError( CE_Failure, CPLE_OpenFailed,
#else
        fprintf( stderr, 
#endif
                 "DSI or ACC record missing.  DTED access to\n%s failed.",
                 pszFilename );
        
        VSIFClose( fp );
        return NULL;
    }

    psDInfo->nDataOffset = VSIFTell( fp );

/* -------------------------------------------------------------------- */
/*      Parse out position information.  Note that we are extracting    */
/*      the top left corner of the top left pixel area, not the         */
/*      center of the area.                                             */
/* -------------------------------------------------------------------- */
    psDInfo->dfPixelSizeX =
        atoi(DTEDGetField(achRecord,21,4)) / 36000.0;

    psDInfo->dfPixelSizeY =
        atoi(DTEDGetField(achRecord,25,4)) / 36000.0;

    /* create a scope so I don't need to declare these up top */
    deg = atoi(stripLeadingZeros(DTEDGetField(achRecord,5,3)));
    min = atoi(stripLeadingZeros(DTEDGetField(achRecord,8,2)));
    sec = atoi(stripLeadingZeros(DTEDGetField(achRecord,10,2)));

    dfLLOriginX = deg + min / 60.0 + sec / 3600.0;
    if( achRecord[11] == 'W' )
        dfLLOriginX *= -1;

    deg = atoi(stripLeadingZeros(DTEDGetField(achRecord,13,3)));
    min = atoi(stripLeadingZeros(DTEDGetField(achRecord,16,2)));
    sec = atoi(stripLeadingZeros(DTEDGetField(achRecord,18,2)));

    dfLLOriginY = deg + min / 60.0 + sec / 3600.0;
    if( achRecord[19] == 'S' )
        dfLLOriginY *= -1;

    psDInfo->dfULCornerX = dfLLOriginX - 0.5 * psDInfo->dfPixelSizeX;
    psDInfo->dfULCornerY = dfLLOriginY - 0.5 * psDInfo->dfPixelSizeY
        + psDInfo->nYSize * psDInfo->dfPixelSizeY;

    return psDInfo;
}

/************************************************************************/
/*                          DTEDReadProfile()                           */
/*                                                                      */
/*      Read one profile line.  These are organized in bottom to top    */
/*      order starting from the leftmost column (0).                    */
/************************************************************************/

int DTEDReadProfile( DTEDInfo * psDInfo, int nColumnOffset,
                     GInt16 * panData )

{
    int         nOffset;
    int         i;
    GByte       *pabyRecord;

/* -------------------------------------------------------------------- */
/*      Read data record from disk.                                     */
/* -------------------------------------------------------------------- */
    pabyRecord = (GByte *) CPLMalloc(12 + psDInfo->nYSize*2);
    
    nOffset = psDInfo->nDataOffset + nColumnOffset * (12+psDInfo->nYSize*2);

    if( VSIFSeek( psDInfo->fp, nOffset, SEEK_SET ) != 0
        || VSIFRead( pabyRecord, (12+psDInfo->nYSize*2), 1, psDInfo->fp ) != 1)
    {
#ifndef AVOID_CPL
        CPLError( CE_Failure, CPLE_FileIO,
#else
        fprintf( stderr, 
#endif
                  "Failed to seek to, or read profile %d at offset %d\n"
                  "in DTED file.\n",
                  nColumnOffset, nOffset );
        return FALSE;
    }

/* -------------------------------------------------------------------- */
/*      Translate data values from "signed magnitude" to standard       */
/*      binary.                                                         */
/* -------------------------------------------------------------------- */
    for( i = 0; i < psDInfo->nYSize; i++ )
    {
        panData[i] = (pabyRecord[8+i*2] & 0x7f) * 256 + pabyRecord[8+i*2+1];

        if( pabyRecord[8+i*2] & 0x80 )
        {
            panData[i] *= -1;

            // It seems that some files are improperly generated in twos
            // complement form for negatives.  For these, redo the job
            // in twos complement.  eg. w_069_s50.dt0
            if( panData[i] < -16000 )
            {
                memcpy( panData + i, pabyRecord + 8+i*2, 2 );
                panData[i] = CPL_MSBWORD16( panData[i] );
            }
        }
    }

    CPLFree( pabyRecord );

    return TRUE;
}

/************************************************************************/
/*                          DTEDWriteProfile()                          */
/************************************************************************/

int DTEDWriteProfile( DTEDInfo * psDInfo, int nColumnOffset,
                     GInt16 * panData )

{
    int         nOffset;
    int         i, nCheckSum = 0;
    GByte       *pabyRecord;

/* -------------------------------------------------------------------- */
/*      Format the data record.                                         */
/* -------------------------------------------------------------------- */
    pabyRecord = (GByte *) CPLMalloc(12 + psDInfo->nYSize*2);
    
    for( i = 0; i < psDInfo->nYSize; i++ )
    {
        int     nABSVal = ABS(panData[psDInfo->nYSize-i-1]);
        pabyRecord[8+i*2] = (GByte) ((nABSVal >> 8) & 0x7f);
        pabyRecord[8+i*2+1] = (GByte) (nABSVal & 0xff);

        if( panData[psDInfo->nYSize-i-1] < 0 )
            pabyRecord[8+i*2] |= 0x80;
    }

    pabyRecord[0] = 0xaa;
    pabyRecord[1] = 0;
    pabyRecord[2] = (GByte) (nColumnOffset / 256);
    pabyRecord[3] = (GByte) (nColumnOffset % 256);
    pabyRecord[4] = (GByte) (nColumnOffset / 256);
    pabyRecord[5] = (GByte) (nColumnOffset % 256);
    pabyRecord[6] = 0;
    pabyRecord[7] = 0;

/* -------------------------------------------------------------------- */
/*      Compute the checksum.                                           */
/* -------------------------------------------------------------------- */
    for( i = 0; i < psDInfo->nYSize*2 + 8; i++ )
        nCheckSum += pabyRecord[i];

    pabyRecord[8+psDInfo->nYSize*2+0] = (GByte) ((nCheckSum >> 24) & 0xff);
    pabyRecord[8+psDInfo->nYSize*2+1] = (GByte) ((nCheckSum >> 16) & 0xff);
    pabyRecord[8+psDInfo->nYSize*2+2] = (GByte) ((nCheckSum >> 8) & 0xff);
    pabyRecord[8+psDInfo->nYSize*2+3] = (GByte) (nCheckSum & 0xff);

/* -------------------------------------------------------------------- */
/*      Write the record.                                               */
/* -------------------------------------------------------------------- */
    nOffset = psDInfo->nDataOffset + nColumnOffset * (12+psDInfo->nYSize*2);

    if( VSIFSeek( psDInfo->fp, nOffset, SEEK_SET ) != 0
        || VSIFWrite( pabyRecord,(12+psDInfo->nYSize*2),1,psDInfo->fp ) != 1)
    {
        CPLFree( pabyRecord );
#ifndef AVOID_CPL
        CPLError( CE_Failure, CPLE_FileIO,
#else
        fprintf( stderr, 
#endif
                  "Failed to seek to, or write profile %d at offset %d\n"
                  "in DTED file.\n",
                  nColumnOffset, nOffset );
        return FALSE;
    }

    CPLFree( pabyRecord );

    return TRUE;
    
}

/************************************************************************/
/*                      DTEDGetMetadataLocation()                       */
/************************************************************************/

static void DTEDGetMetadataLocation( DTEDInfo *psDInfo, 
                                     DTEDMetaDataCode eCode, 
                                     char **ppszLocation, int *pnLength )
{
    switch( eCode )
    {
      case DTEDMD_VERTACCURACY:
        *ppszLocation = psDInfo->pachUHLRecord + 28;
        *pnLength = 4;
        break;

      case DTEDMD_SECURITYCODE:
        *ppszLocation = psDInfo->pachUHLRecord + 32;
        *pnLength = 3;
        break;

      case DTEDMD_PRODUCER:
        *ppszLocation = psDInfo->pachDSIRecord + 102;
        *pnLength = 8;
        break;

      case DTEDMD_COMPILATION_DATE:
        *ppszLocation = psDInfo->pachDSIRecord + 159;
        *pnLength = 4;
        break;

      default:
        *ppszLocation = NULL;
        *pnLength = 0;
    }
}
                                         


/************************************************************************/
/*                          DTEDGetMetadata()                           */
/************************************************************************/

char *DTEDGetMetadata( DTEDInfo *psDInfo, DTEDMetaDataCode eCode )

{
    int nFieldLen;
    char *pszFieldSrc;
    char *pszResult;

    DTEDGetMetadataLocation( psDInfo, eCode, &pszFieldSrc, &nFieldLen );
    if( pszFieldSrc == NULL )
        return VSIStrdup( "" );

    pszResult = (char *) malloc(nFieldLen+1);
    strncpy( pszResult, pszFieldSrc, nFieldLen );
    pszResult[nFieldLen] = '\0';

    return pszResult;
}

/************************************************************************/
/*                          DTEDSetMetadata()                           */
/************************************************************************/

int DTEDSetMetadata( DTEDInfo *psDInfo, DTEDMetaDataCode eCode, 
                     const char *pszNewValue )

{
    int nFieldLen;
    char *pszFieldSrc;

    if( !psDInfo->bUpdate )
        return FALSE;

/* -------------------------------------------------------------------- */
/*      Get the location in the headers to update.                      */
/* -------------------------------------------------------------------- */
    DTEDGetMetadataLocation( psDInfo, eCode, &pszFieldSrc, &nFieldLen );
    if( pszFieldSrc == NULL )
        return FALSE;

/* -------------------------------------------------------------------- */
/*      Update it, padding with spaces.                                 */
/* -------------------------------------------------------------------- */
    memset( pszFieldSrc, ' ', nFieldLen );
    strncpy( pszFieldSrc, pszNewValue, 
             MIN(strlen(pszFieldSrc),strlen(pszNewValue)) );

/* -------------------------------------------------------------------- */
/*      Write all headers back to disk.                                 */
/* -------------------------------------------------------------------- */
    VSIFSeek( psDInfo->fp, psDInfo->nUHLOffset, SEEK_SET );
    VSIFWrite( psDInfo->pachUHLRecord, 1, DTED_UHL_SIZE, psDInfo->fp );

    VSIFSeek( psDInfo->fp, psDInfo->nDSIOffset, SEEK_SET );
    VSIFWrite( psDInfo->pachDSIRecord, 1, DTED_DSI_SIZE, psDInfo->fp );

    VSIFSeek( psDInfo->fp, psDInfo->nACCOffset, SEEK_SET );
    VSIFWrite( psDInfo->pachACCRecord, 1, DTED_ACC_SIZE, psDInfo->fp );

    return TRUE;
}

/************************************************************************/
/*                             DTEDClose()                              */
/************************************************************************/

void DTEDClose( DTEDInfo * psDInfo )

{
    VSIFClose( psDInfo->fp );

    CPLFree( psDInfo->pachUHLRecord );
    CPLFree( psDInfo->pachDSIRecord );
    CPLFree( psDInfo->pachACCRecord );
    
    CPLFree( psDInfo );
}
