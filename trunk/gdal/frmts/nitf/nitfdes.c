/******************************************************************************
 * $Id$
 *
 * Project:  NITF Read/Write Library
 * Purpose:  Module responsible for implementation of DE segments.
 * Author:   Even Rouault, <even dot rouault at mines dash paris dot org>
 *
 **********************************************************************
 * Copyright (c) 2010, Even Rouault <even dot rouault at mines dash paris dot org>
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
#include "nitflib.h"
#include "cpl_vsi.h"
#include "cpl_conv.h"
#include "cpl_string.h"

CPL_CVSID("$Id$");

/************************************************************************/
/*                          NITFDESAccess()                             */
/************************************************************************/

NITFDES *NITFDESAccess( NITFFile *psFile, int iSegment )

{
    NITFDES   *psDES;
    char      *pachHeader;
    NITFSegmentInfo *psSegInfo;
    char       szTemp[128];
    int        nOffset;
    char       chDECLAS;
    int        bHasDESOFLW;
    int        nDESSHL;
    
/* -------------------------------------------------------------------- */
/*      Verify segment, and return existing DES accessor if there       */
/*      is one.                                                         */
/* -------------------------------------------------------------------- */
    if( iSegment < 0 || iSegment >= psFile->nSegmentCount )
        return NULL;

    psSegInfo = psFile->pasSegmentInfo + iSegment;

    if( !EQUAL(psSegInfo->szSegmentType,"DE") )
        return NULL;

    if( psSegInfo->hAccess != NULL )
        return (NITFDES *) psSegInfo->hAccess;

/* -------------------------------------------------------------------- */
/*      Read the DES subheader.                                         */
/* -------------------------------------------------------------------- */
    if (psSegInfo->nSegmentHeaderSize < 30)
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                    "DES header too small");
        return NULL;
    }

    pachHeader = (char*) VSIMalloc(psSegInfo->nSegmentHeaderSize);
    if (pachHeader == NULL)
    {
        CPLError(CE_Failure, CPLE_OutOfMemory,
                 "Cannot allocate memory for segment header");
        return NULL;
    }

    if( VSIFSeekL( psFile->fp, psSegInfo->nSegmentHeaderStart, 
                  SEEK_SET ) != 0 
        || VSIFReadL( pachHeader, 1, psSegInfo->nSegmentHeaderSize, 
                     psFile->fp ) != psSegInfo->nSegmentHeaderSize )
    {
        CPLError( CE_Failure, CPLE_FileIO, 
                  "Failed to read %u byte DES subheader from " CPL_FRMT_GUIB ".",
                  psSegInfo->nSegmentHeaderSize,
                  psSegInfo->nSegmentHeaderStart );
        CPLFree(pachHeader);
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Initialize DES object.                                          */
/* -------------------------------------------------------------------- */
    psDES = (NITFDES *) CPLCalloc(sizeof(NITFDES),1);

    psDES->psFile = psFile;
    psDES->iSegment = iSegment;
    psDES->pachHeader = pachHeader;

    psSegInfo->hAccess = psDES;

/* -------------------------------------------------------------------- */
/*      Collect a variety of information as metadata.                   */
/* -------------------------------------------------------------------- */
#define GetMD( length, name )              \
    do { NITFExtractMetadata( &(psDES->papszMetadata), pachHeader,    \
                         nOffset, length,                        \
                         "NITF_" #name ); \
    nOffset += length; } while(0)

    nOffset = 2;
    GetMD( 25, DESID  );
    GetMD(  2, DESVER );
    GetMD(  1, DECLAS );

/* -------------------------------------------------------------------- */
/*      Should we extract classification informations ?                 */
/*      Strangely some unclassified files have those fields blank,      */
/*      whereas they should not be present according to the standard... */
/* -------------------------------------------------------------------- */
    chDECLAS = pachHeader[27];
    if (chDECLAS == 'T' || chDECLAS == 'S' ||
        chDECLAS == 'C' || chDECLAS == 'R' ||
        (int)psSegInfo->nSegmentHeaderSize >= nOffset + 166 )
    {
        if ((int)psSegInfo->nSegmentHeaderSize < nOffset + 166 )
        {
            CPLError(CE_Failure, CPLE_AppDefined,
                        "DES header too small");
            NITFDESDeaccess(psDES);
            return NULL;
        }
        GetMD(  2, DESCLSY );
        GetMD( 11, DESCODE );
        GetMD(  2, DESCTLH );
        GetMD( 20, DESREL  );
        GetMD(  2, DESDCTP );
        GetMD(  8, DESDCDT );
        GetMD(  4, DESDCXM );
        GetMD(  1, DESDG   );
        GetMD(  8, DESDGDT );
        GetMD( 43, DESCLTX );
        GetMD(  1, DESCATP );
        GetMD( 40, DESCAUT );
        GetMD(  1, DESCRSN );
        GetMD(  8, DESSRDT );
        GetMD( 15, DESCTLN );
    }

    if ((int)psSegInfo->nSegmentHeaderSize < nOffset + 4 )
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                    "DES header too small");
        NITFDESDeaccess(psDES);
        return NULL;
    }

    /* Load DESID */
    NITFGetField( szTemp, pachHeader, 2, 25);

    /* For NITF < 02.10, we cannot rely on DESID=TRE_OVERFLOW to detect */
    /* if DESOFLW and DESITEM are present. So if the next 4 bytes are non */
    /* numeric, we'll assume that DESOFLW is there */
    bHasDESOFLW = EQUALN(szTemp, "TRE_OVERFLOW", strlen("TRE_OVERFLOW")) ||
       (!((pachHeader[nOffset+0] >= '0' && pachHeader[nOffset+0] <= '9') &&
          (pachHeader[nOffset+1] >= '0' && pachHeader[nOffset+1] <= '9') &&
          (pachHeader[nOffset+2] >= '0' && pachHeader[nOffset+2] <= '9') &&
          (pachHeader[nOffset+3] >= '0' && pachHeader[nOffset+3] <= '9')));

    if (bHasDESOFLW)
    {
        if ((int)psSegInfo->nSegmentHeaderSize < nOffset + 6 + 3 )
        {
            CPLError(CE_Failure, CPLE_AppDefined,
                        "DES header too small");
            NITFDESDeaccess(psDES);
            return NULL;
        }
        GetMD(  6, DESOFLW );
        GetMD(  3, DESITEM );
    }

    if ((int)psSegInfo->nSegmentHeaderSize < nOffset + 4 )
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                    "DES header too small");
        NITFDESDeaccess(psDES);
        return NULL;
    }

    nDESSHL = atoi(NITFGetField( szTemp, pachHeader, nOffset, 4));
    nOffset += 4;

    if (nDESSHL < 0)
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                    "Invalid value for DESSHL");
        NITFDESDeaccess(psDES);
        return NULL;
    }
    if ( (int)psSegInfo->nSegmentHeaderSize < nOffset + nDESSHL)
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                    "DES header too small");
        NITFDESDeaccess(psDES);
        return NULL;
    }

    if (nDESSHL > 0)
        GetMD(  nDESSHL, DESSHF );

    if ((int)psSegInfo->nSegmentHeaderSize > nOffset)
    {
        char* pszEscapedDESDATA =
                CPLEscapeString( pachHeader + nOffset, 
                                 (int)psSegInfo->nSegmentHeaderSize - nOffset, 
                                 CPLES_BackslashQuotable );
        psDES->papszMetadata = CSLSetNameValue( psDES->papszMetadata,
                                                "NITF_DESDATA",
                                                pszEscapedDESDATA );
        CPLFree(pszEscapedDESDATA);
    }

    return psDES;
}

/************************************************************************/
/*                           NITFDESDeaccess()                          */
/************************************************************************/

void NITFDESDeaccess( NITFDES *psDES )

{
    CPLAssert( psDES->psFile->pasSegmentInfo[psDES->iSegment].hAccess
               == psDES );

    psDES->psFile->pasSegmentInfo[psDES->iSegment].hAccess = NULL;

    CPLFree( psDES->pachHeader );
    CSLDestroy( psDES->papszMetadata );

    CPLFree( psDES );
}

/************************************************************************/
/*                              NITFDESGetTRE()                         */
/************************************************************************/

/**
 * Return the TRE located at nOffset.
 *
 * @param psDES          descriptor of the DE segment
 * @param nOffset        offset of the TRE relative to the beginning of the segment data
 * @param szTREName      will be filled with the TRE name
 * @param ppabyTREData   will be allocated by the function and filled with the TRE content (in raw form)
 * @param pnFoundTRESize will be filled with the TRE size (excluding the first 11 bytes)
 * @return TRUE if a TRE was found
 */

int   NITFDESGetTRE( NITFDES* psDES,
                     int nOffset,
                     char szTREName[7],
                     char** ppabyTREData,
                     int* pnFoundTRESize)
{
    char szTREHeader[12];
    char szTRETempName[7];
    NITFSegmentInfo* psSegInfo;
    FILE* fp;
    int nTRESize;

    memset(szTREName, '\0', 7);
    if (ppabyTREData)
        *ppabyTREData = NULL;
    if (pnFoundTRESize)
        *pnFoundTRESize = 0;

    if (nOffset < 0)
        return FALSE;

    if (psDES == NULL)
        return FALSE;

    if (CSLFetchNameValue(psDES->papszMetadata, "NITF_DESOFLW") == NULL)
        return FALSE;

    psSegInfo = psDES->psFile->pasSegmentInfo + psDES->iSegment;
    fp = psDES->psFile->fp;

    if (nOffset >= psSegInfo->nSegmentSize)
        return FALSE;

    VSIFSeekL(fp, psSegInfo->nSegmentStart + nOffset, SEEK_SET);

    if (VSIFReadL(szTREHeader, 1, 11, fp) != 11)
    {
        CPLError(CE_Failure, CPLE_FileIO,
                 "Cannot get 11 bytes at offset " CPL_FRMT_GUIB ".",
                 psSegInfo->nSegmentStart + nOffset );
        return FALSE;
    }
    szTREHeader[11] = '\0';

    memcpy(szTRETempName, szTREHeader, 6);
    szTRETempName[6] = '\0';

    nTRESize = atoi(szTREHeader + 6);
    if (nTRESize < 0)
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                 "Invalid size (%d) for TRE %s",
                 nTRESize, szTRETempName);
        return FALSE;
    }
    if (nOffset + 11 + nTRESize > psSegInfo->nSegmentSize)
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                 "Cannot read %s TRE. Not enough bytes : remaining %d, expected %d",
                 szTRETempName,
                 (int)(psSegInfo->nSegmentSize - (nOffset + 11)), nTRESize);
        return FALSE;
    }

    if (ppabyTREData)
    {
        /* Allocate one extra byte for the NULL terminating character */
        *ppabyTREData = (char*) VSIMalloc(nTRESize + 1);
        if (*ppabyTREData  == NULL)
        {
            CPLError(CE_Failure, CPLE_OutOfMemory,
                    "Cannot allocate %d bytes for TRE %s",
                    nTRESize, szTRETempName);
            return FALSE;
        }
        (*ppabyTREData)[nTRESize] = '\0';

        if ((int)VSIFReadL(*ppabyTREData, 1, nTRESize, fp) != nTRESize)
        {
            CPLError(CE_Failure, CPLE_FileIO,
                     "Cannot get %d bytes at offset " CPL_FRMT_GUIB ".",
                     nTRESize, VSIFTellL(fp) );
            VSIFree(*ppabyTREData);
            *ppabyTREData = NULL;
            return FALSE;
        }
    }

    strcpy(szTREName, szTRETempName);
    if (pnFoundTRESize)
        *pnFoundTRESize = nTRESize;

    return TRUE;
}

/************************************************************************/
/*                           NITFDESFreeTREData()                       */
/************************************************************************/

void NITFDESFreeTREData( char* pabyTREData )
{
    VSIFree(pabyTREData);
}
