/******************************************************************************
 * $Id$
 *
 * Project:  BSB Reader
 * Purpose:  Low level BSB Access API Implementation (non-GDAL).
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 * NOTE: This code is implemented on the basis of work by Mike Higgins.  The 
 * BSB format is subject to US patent 5,727,090; however, that patent 
 * apparently only covers *writing* BSB files, not reading them, so this code 
 * should not be affected.
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
 * Revision 1.4  2002/03/19 20:58:24  warmerda
 * Changes from Olle Soderholm that should allow for NOS files as well.
 *
 * Revision 1.3  2001/12/08 21:58:32  warmerda
 * save header
 *
 * Revision 1.2  2001/12/08 15:58:45  warmerda
 * fixed VSIFSeek() arguments as per Chris Schaefer
 *
 * Revision 1.1  2001/12/08 04:35:16  warmerda
 * New
 *
 */

#include "bsb_read.h"
#include "cpl_conv.h"
#include "cpl_string.h"

CPL_CVSID("$Id$");

static const char *BSBReadHeaderLine( FILE *fp );

/************************************************************************

Background:

To: Frank Warmerdam <warmerda@home.com>
From: Mike Higgins <higgins@monitor.net>
Subject: Re: GISTrans: Maptech / NDI BSB Chart Format
Mime-Version: 1.0
Content-Type: text/plain; charset="us-ascii"; format=flowed

         I did it! I just wrote a program that reads NOAA BSB chart files 
and converts them to BMP files! BMP files are not the final goal of my 
project, but it served as a proof-of-concept.  Next I will want to write 
routines to extract pieces of the file at full resolution for printing, and 
routines to filter pieces of the chart for display at lower resolution on 
the screen.  (One of the terrible things about most chart display programs 
is that they all sub-sample the charts instead of filtering it down). How 
did I figure out how to read the BSB files?

         If you recall, I have been trying to reverse engineer the file 
formats of those nautical charts. When I am between projects I often do a 
WEB search for the BSB file format to see if someone else has published a 
hack for them. Monday I hit a NOAA project status report that mentioned 
some guy named Marty Yellin who had recently completed writing a program to 
convert BSB files to other file formats! I searched for him and found him 
mentioned as a contact person for some NOAA program. I was composing a 
letter to him in my head, or considering calling the NOAA phone number and 
asking for his extension number, when I saw another NOAA status report 
indicating that he had retired in 1998. His name showed up in a few more 
reports, one of which said that he was the inventor of the BSB file format, 
that it was patented (#5,727,090), and that the patent had been licensed to 
Maptech (the evil company that will not allow anyone using their file 
format to convert them to non-proprietary formats). Patents are readily 
available on the WEB at the IBM patent server and this one is in the 
dtabase!  I printed up a copy of the patent and of course it describes very 
nicely (despite the usual typos and omissions of referenced items in the 
figures) how to write one of these BSB files!

         I was considering talking to a patent lawyer about the legality of 
using information in the patent to read files without getting a license,
when I noticed that the patent is only claiming programs that WRITE the 
file format. I have noticed this before in RF patents where they describe 
how to make a receiver and never bother to claim a transmitter. The logic 
is that the transmitter is no good to anybody unless they license receivers 
from the patent holder. But I think they did it backwards here! They should 
have claimed a program that can READ the described file format. Now I can 
read the files, build programs that read the files, and even sell them 
without violating the claims in the patent! As long as I never try to write 
one of the evil BSB files, I'm OK!!!

         If you ever need to read these BSB chart programs, drop me a 
note.  I would be happy to send you a copy of this conversion program.

... later email ...

         Well, here is my little proof of concept program. I hereby give 
you permission to distribute it freely, modify for you own use, etc.
I built it as a "WIN32 Console application" which means it runs in an MS 
DOS box under Microsoft Windows. But the only Windows specific stuff in it 
are the include files for the BMP file headers.  If you ripped out the BMP 
code it should compile under UNIX or anyplace else.
         I'd be overjoyed to have you announce it to GISTrans or anywhere 
else.  I'm philosophically opposed to the proprietary treatment of the  BSB 
file format and I want to break it open! Chart data for the People!

 ************************************************************************/


/************************************************************************/
/*                              BSBOpen()                               */
/*                                                                      */
/*      Read BSB header, and return information.                        */
/************************************************************************/

BSBInfo *BSBOpen( const char *pszFilename )

{
    FILE	*fp;
    char	achTestBlock[1000];
    const char  *pszLine;
    int         i;
    BSBInfo     *psInfo;

/* -------------------------------------------------------------------- */
/*      Open the file.                                                  */
/* -------------------------------------------------------------------- */
    fp = VSIFOpen( pszFilename, "rb" );
    if( fp == NULL )
    {
        CPLError( CE_Failure, CPLE_OpenFailed, 
                  "File %s not found.", pszFilename );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*	Read the first 1000 bytes, and verify that it contains the	*/
/*	"BSB/" keyword"							*/
/* -------------------------------------------------------------------- */
    if( VSIFRead( achTestBlock, 1, sizeof(achTestBlock), fp ) 
        != sizeof(achTestBlock) )
    {
        VSIFClose( fp );
        CPLError( CE_Failure, CPLE_FileIO,
                  "Could not read first %d bytes for header!", 
                  sizeof(achTestBlock) );
        return NULL;
    }

    for( i = 0; i < sizeof(achTestBlock) - 4; i++ )
    {
        if( achTestBlock[i+0] == 'B' && achTestBlock[i+1] == 'S' 
            && achTestBlock[i+2] == 'B' && achTestBlock[i+3] == '/' )
            break;

        if( achTestBlock[i+0] == 'N' && achTestBlock[i+1] == 'O'
            && achTestBlock[i+2] == 'S' && achTestBlock[i+3] == '/' )
            break;

    }

    if( i == sizeof(achTestBlock) - 4 )
    {
        VSIFClose( fp );
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "This does not appear to be a BSB file, no BSB/ header." );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Create info structure.                                          */
/* -------------------------------------------------------------------- */
    psInfo = (BSBInfo *) CPLCalloc(1,sizeof(BSBInfo));
    psInfo->fp = fp;

/* -------------------------------------------------------------------- */
/*      Rewind, and read line by line.                                  */
/* -------------------------------------------------------------------- */
    VSIFSeek( fp, 0, SEEK_SET );

    while( (pszLine = BSBReadHeaderLine(fp)) != NULL )
    {
        char	**papszTokens = NULL;
        int      nCount = 0;

        if( pszLine[3] == '/' )
        {
            psInfo->papszHeader = CSLAddString( psInfo->papszHeader, pszLine );
            papszTokens = CSLTokenizeStringComplex( pszLine+4, ",=", 
                                                    FALSE,FALSE);
            nCount = CSLCount(papszTokens);
        }

        if( EQUALN(pszLine,"BSB/",4) )
        {
            int		nRAIndex;

            nRAIndex = CSLFindString(papszTokens, "RA" );
            if( nRAIndex < 0 || nRAIndex > nCount - 2 )
            {
                CSLDestroy( papszTokens );
                CPLError( CE_Failure, CPLE_AppDefined, 
                          "Failed to extract RA from BSB/ line." );
                BSBClose( psInfo );
                return NULL;
            }
            psInfo->nXSize = atoi(papszTokens[nRAIndex+1]);
            psInfo->nYSize = atoi(papszTokens[nRAIndex+2]);
        }
        else if( EQUALN(pszLine,"NOS/",4) )
        {
            int  nRAIndex;
            
            nRAIndex = CSLFindString(papszTokens, "RA" );
            if( nRAIndex < 0 || nRAIndex > nCount - 2 )
            {
                CSLDestroy( papszTokens );
                CPLError( CE_Failure, CPLE_AppDefined,
                          "Failed to extract RA from NOS/ line." );
                BSBClose( psInfo );
                return NULL;
            }
            psInfo->nXSize = atoi(papszTokens[nRAIndex+3]);
            psInfo->nYSize = atoi(papszTokens[nRAIndex+4]);
        }
        else if( EQUALN(pszLine,"RGB/",4) && nCount >= 4 )
        {
            int	iPCT = atoi(papszTokens[0]);
            if( iPCT > psInfo->nPCTSize-1 )
            {
                psInfo->pabyPCT = (unsigned char *) 
                    CPLRealloc(psInfo->pabyPCT,(iPCT+1) * 3);
                memset( psInfo->pabyPCT + psInfo->nPCTSize*3, 0, 
                        (iPCT+1-psInfo->nPCTSize) * 3);
                psInfo->nPCTSize = iPCT+1;
            }

            psInfo->pabyPCT[iPCT*3+0] = atoi(papszTokens[1]);
            psInfo->pabyPCT[iPCT*3+1] = atoi(papszTokens[2]);
            psInfo->pabyPCT[iPCT*3+2] = atoi(papszTokens[3]);
        }

        CSLDestroy( papszTokens );
    }
    
/* -------------------------------------------------------------------- */
/*      Verify we found required keywords.                              */
/* -------------------------------------------------------------------- */
    if( psInfo->nXSize == 0 || psInfo->nPCTSize == 0 )
    {
        BSBClose( psInfo );
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Failed to find required RGB/ or BSB/ keyword in header." );
        
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      If all has gone well this far, we should be pointing at the     */
/*      sequence "0x1A 0x00".  Read past to get to start of data.       */
/* -------------------------------------------------------------------- */
    if( VSIFGetc( fp ) !=  0x1A
        || VSIFGetc( fp ) != 0x00 )
    {
        BSBClose( psInfo );
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Failed to find compressed data segment of BSB file." );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Read the number of bit size of color numbers.                   */
/* -------------------------------------------------------------------- */
    psInfo->nColorSize = VSIFGetc( fp );
    CPLAssert( psInfo->nColorSize > 0 && psInfo->nColorSize < 9 );

/* -------------------------------------------------------------------- */
/*      Initialize line offset list.                                    */
/* -------------------------------------------------------------------- */
    psInfo->panLineOffset = (int *) 
        CPLMalloc(sizeof(int) * psInfo->nYSize);
    for( i = 0; i < psInfo->nYSize; i++ )
        psInfo->panLineOffset[i] = -1;

    psInfo->panLineOffset[0] = VSIFTell( fp );

    return psInfo;
}

/************************************************************************/
/*                         BSBReadHeaderLine()                          */
/*                                                                      */
/*      Read one virtual line of text from the BSB header.  This        */
/*      will end if a 0x1A (EOF) is encountered, indicating the data    */
/*      is about to start.  It will also merge multiple physical        */
/*      lines where appropriate.  The buffer is internal, and only      */
/*      lasts till the next call.                                       */
/************************************************************************/

const char *BSBReadHeaderLine( FILE * fp )

{
    static char	szLine[241];
    char        chNext;
    int	        nLineLen = 0;

    while( !feof(fp) && nLineLen < sizeof(szLine)-1 )
    {
        chNext = VSIFGetc( fp );
        if( chNext == 0x1A )
        {
            VSIUngetc( chNext, fp );
            return NULL;
        }

        /* each CR/LF (or LF/CR) as if just "CR" */
        if( chNext == 10 || chNext == 13 )
        {
            char	chLF;

            chLF = VSIFGetc( fp );
            if( chLF != 10 && chLF != 13 )
                VSIUngetc( chLF, fp );
            chNext = '\n';
        }

        /* If we are at the end-of-line, check for blank at start
        ** of next line, to indicate need of continuation.
        */
        if( chNext == '\n' )
        {
            char chTest;

            chTest = VSIFGetc(fp);
            /* Are we done? */
            if( chTest != ' ' )
            {
                VSIUngetc( chTest, fp );
                szLine[nLineLen] = '\0';
                return szLine;
            }

            /* eat pending spaces */
            while( chTest == ' ' )
                chTest = VSIFGetc(fp);
            VSIUngetc( chTest, fp );

            /* insert comma in data stream */
            szLine[nLineLen++] = ',';
        }
        else
        {
            szLine[nLineLen++] = chNext;
        }
    }

    return NULL;
}

/************************************************************************/
/*                          BSBReadScanline()                           */
/************************************************************************/

int BSBReadScanline( BSBInfo *psInfo, int nScanline, 
                     unsigned char *pabyScanlineBuf )

{
    int		nLineMarker = 0, nValueShift, iPixel = 0;
    unsigned char byValueMask, byCountMask;
    FILE	*fp = psInfo->fp;
    int         byNext, i;

/* -------------------------------------------------------------------- */
/*      Do we know where the requested line is?  If not, read all       */
/*      the preceeding ones to "find" our line.                         */
/* -------------------------------------------------------------------- */
    if( nScanline < 0 || nScanline >= psInfo->nYSize )
    {
        CPLError( CE_Failure, CPLE_FileIO, 
                  "Scanline %d out of range.", 
                   nScanline );
        return FALSE;
    }

    if( psInfo->panLineOffset[nScanline] == -1 )
    {
        for( i = 0; i < nScanline; i++ )
        {
            if( psInfo->panLineOffset[i+1] == -1 )
            {
                if( !BSBReadScanline( psInfo, i, pabyScanlineBuf ) )
                    return FALSE;
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      Seek to requested scanline.                                     */
/* -------------------------------------------------------------------- */
    if( VSIFSeek( fp, psInfo->panLineOffset[nScanline], SEEK_SET ) != 0 )
    {
        CPLError( CE_Failure, CPLE_FileIO, 
                  "Seek to offset %d for scanline %d failed.", 
                  psInfo->panLineOffset[nScanline], nScanline );
        return FALSE;
    }

/* -------------------------------------------------------------------- */
/*      read the line number.                                           */
/* -------------------------------------------------------------------- */
    do {
        byNext = VSIFGetc( fp );
        nLineMarker = nLineMarker * 128 + (byNext & 0x7f);
    } while( (byNext & 0x80) != 0 );

    if( nLineMarker == 0 )
        return FALSE;

    CPLAssert( nLineMarker == nScanline + 1 );

/* -------------------------------------------------------------------- */
/*      Setup masking values.                                           */
/* -------------------------------------------------------------------- */
    nValueShift = 7 - psInfo->nColorSize;
    byValueMask = (((1 << psInfo->nColorSize)) - 1) << nValueShift;
    byCountMask = (1 << (7 - psInfo->nColorSize)) - 1;
    
/* -------------------------------------------------------------------- */
/*      Read and expand runs.                                           */
/* -------------------------------------------------------------------- */
    while( (byNext = VSIFGetc(fp)) != 0 )
    {
        int	nPixValue;
        int     nRunCount, i;

        nPixValue = (byNext & byValueMask) >> nValueShift;

        nRunCount = byNext & byCountMask;

        while( (byNext & 0x80) != 0 )
        {
            byNext = VSIFGetc( fp );
            nRunCount = nRunCount * 128 + (byNext & 0x7f);
        }

        if( iPixel + nRunCount + 1 > psInfo->nXSize )
            nRunCount = psInfo->nXSize - iPixel - 1;

        for( i = 0; i < nRunCount+1; i++ )
            pabyScanlineBuf[iPixel++] = nPixValue;
    }

/* -------------------------------------------------------------------- */
/*      Remember the start of the next line.                            */
/* -------------------------------------------------------------------- */
    if( iPixel == psInfo->nXSize && nScanline < psInfo->nYSize-1 )
        psInfo->panLineOffset[nScanline+1] = VSIFTell( fp );

    return iPixel == psInfo->nXSize;
}

/************************************************************************/
/*                              BSBClose()                              */
/************************************************************************/

void BSBClose( BSBInfo *psInfo )

{
    if( psInfo->fp != NULL )
        VSIFClose( psInfo->fp );

    CSLDestroy( psInfo->papszHeader );
    CPLFree( psInfo->panLineOffset );
    CPLFree( psInfo->pabyPCT );
    CPLFree( psInfo );
}
