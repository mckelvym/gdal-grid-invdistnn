/******************************************************************************
 * $Id$
 *
 * Project:  NITF Read/Write Library
 * Purpose:  Module responsible for implementation of most NITFImage 
 *           implementation.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 **********************************************************************
 * Copyright (c) 2002, Frank Warmerdam
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
#include "mgrs.h"
#include "cpl_vsi.h"
#include "cpl_conv.h"
#include "cpl_string.h"

CPL_CVSID("$Id$");

static char *NITFTrimWhite( char * );
#ifdef CPL_LSB
static void NITFSwapWords( void *pData, int nWordSize, int nWordCount,
                           int nWordSkip );
#endif

static void NITFLoadLocationTable( NITFImage *psImage );
static int NITFLoadVQTables( NITFImage *psImage );

void NITFGetGCP ( const char* pachCoord, GDAL_GCP *psIGEOLOGCPs, int iCoord );
int NITFReadBLOCKA_GCPs ( NITFImage *psImage, GDAL_GCP *psIGEOLOGCPs );


/************************************************************************/
/*                          NITFImageAccess()                           */
/************************************************************************/

NITFImage *NITFImageAccess( NITFFile *psFile, int iSegment )

{
    NITFImage *psImage;
    char      *pachHeader;
    NITFSegmentInfo *psSegInfo;
    char       szTemp[128];
    int        nOffset, iBand, i;
    int        nIGEOLOGCPCount = 4;
    double     adfGeoTransform[6] = { 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };	
    GDAL_GCP   *psIGEOLOGCPs;
    
/* -------------------------------------------------------------------- */
/*      Verify segment, and return existing image accessor if there     */
/*      is one.                                                         */
/* -------------------------------------------------------------------- */
    if( iSegment < 0 || iSegment >= psFile->nSegmentCount )
        return NULL;

    psSegInfo = psFile->pasSegmentInfo + iSegment;

    if( !EQUAL(psSegInfo->szSegmentType,"IM") )
        return NULL;

    if( psSegInfo->hAccess != NULL )
        return (NITFImage *) psSegInfo->hAccess;

/* -------------------------------------------------------------------- */
/*      Read the image subheader.                                       */
/* -------------------------------------------------------------------- */
    pachHeader = (char*) CPLMalloc(psSegInfo->nSegmentHeaderSize);
    if( VSIFSeekL( psFile->fp, psSegInfo->nSegmentHeaderStart, 
                  SEEK_SET ) != 0 
        || VSIFReadL( pachHeader, 1, psSegInfo->nSegmentHeaderSize, 
                     psFile->fp ) != psSegInfo->nSegmentHeaderSize )
    {
        CPLError( CE_Failure, CPLE_FileIO, 
                  "Failed to read %d byte image subheader from %d.",
                  psSegInfo->nSegmentHeaderSize,
                  psSegInfo->nSegmentHeaderStart );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Initialize image object.                                        */
/* -------------------------------------------------------------------- */
    psImage = (NITFImage *) CPLCalloc(sizeof(NITFImage),1);

    psImage->psFile = psFile;
    psImage->iSegment = iSegment;
    psImage->pachHeader = pachHeader;

    psSegInfo->hAccess = psImage;

/* -------------------------------------------------------------------- */
/*      Collect a variety of information as metadata.                   */
/* -------------------------------------------------------------------- */
#define GetMD( target, hdr, start, length, name )              \
    NITFExtractMetadata( &(target->papszMetadata), hdr,       \
                         start, length,                        \
                         "NITF_" #name );
       
    if( EQUAL(psFile->szVersion,"NITF02.10") )
    {
        GetMD( psImage, pachHeader,   2,  10, IID1   );
        GetMD( psImage, pachHeader,  12,  14, IDATIM );
        GetMD( psImage, pachHeader,  26,  17, TGTID  );
        GetMD( psImage, pachHeader,  43,  80, IID2   );
        GetMD( psImage, pachHeader, 123,   1, ISCLAS );
        GetMD( psImage, pachHeader, 124,   2, ISCLSY );
        GetMD( psImage, pachHeader, 126,  11, ISCODE );
        GetMD( psImage, pachHeader, 137,   2, ISCTLH );
        GetMD( psImage, pachHeader, 139,  20, ISREL  );
        GetMD( psImage, pachHeader, 159,   2, ISDCTP );
        GetMD( psImage, pachHeader, 161,   8, ISDCDT );
        GetMD( psImage, pachHeader, 169,   4, ISDCXM );
        GetMD( psImage, pachHeader, 173,   1, ISDG   );
        GetMD( psImage, pachHeader, 174,   8, ISDGDT );
        GetMD( psImage, pachHeader, 182,  43, ISCLTX );
        GetMD( psImage, pachHeader, 225,   1, ISCATP );
        GetMD( psImage, pachHeader, 226,  40, ISCAUT );
        GetMD( psImage, pachHeader, 266,   1, ISCRSN );
        GetMD( psImage, pachHeader, 267,   8, ISSRDT );
        GetMD( psImage, pachHeader, 275,  15, ISCTLN );
        /* skip ENCRYPT - 1 character */
        GetMD( psImage, pachHeader, 291,  42, ISORCE );
        /* skip NROWS (8), and NCOLS (8) */
        GetMD( psImage, pachHeader, 349,   3, PVTYPE );
        GetMD( psImage, pachHeader, 352,   8, IREP   );
        GetMD( psImage, pachHeader, 360,   8, ICAT   );
        GetMD( psImage, pachHeader, 368,   2, ABPP   );
        GetMD( psImage, pachHeader, 370,   1, PJUST  );
    }
    else if( EQUAL(psFile->szVersion,"NITF02.00") )
    {
        int nOffset = 0;
        GetMD( psImage, pachHeader,   2,  10, IID1   );
        GetMD( psImage, pachHeader,  12,  14, IDATIM );
        GetMD( psImage, pachHeader,  26,  17, TGTID  );
        GetMD( psImage, pachHeader,  43,  80, ITITLE );
        GetMD( psImage, pachHeader, 123,   1, ISCLAS );
        GetMD( psImage, pachHeader, 124,  40, ISCODE );
        GetMD( psImage, pachHeader, 164,  40, ISCTLH );
        GetMD( psImage, pachHeader, 204,  40, ISREL  );
        GetMD( psImage, pachHeader, 244,  20, ISCAUT );
        GetMD( psImage, pachHeader, 264,  20, ISCTLN );
        GetMD( psImage, pachHeader, 284,   6, ISDWNG );
        
        if( EQUALN(pachHeader+284,"999998",6) )
        {
            GetMD( psImage, pachHeader, 290,  40, ISDEVT );
            nOffset += 40;
        }

        /* skip ENCRYPT - 1 character */
        GetMD( psImage, pachHeader, 291+nOffset,  42, ISORCE );
        /* skip NROWS (8), and NCOLS (8) */
        GetMD( psImage, pachHeader, 349+nOffset,   3, PVTYPE );
        GetMD( psImage, pachHeader, 352+nOffset,   8, IREP   );
        GetMD( psImage, pachHeader, 360+nOffset,   8, ICAT   );
        GetMD( psImage, pachHeader, 368+nOffset,   2, ABPP   );
        GetMD( psImage, pachHeader, 370+nOffset,   1, PJUST  );
    }

/* -------------------------------------------------------------------- */
/*      Does this header have the FSDEVT field?                         */
/* -------------------------------------------------------------------- */
    nOffset = 333;

    if( EQUALN(psFile->szVersion,"NITF01.",7) 
        || EQUALN(pachHeader+284,"999998",6) )
        nOffset += 40;

/* -------------------------------------------------------------------- */
/*      Read lots of header fields.                                     */
/* -------------------------------------------------------------------- */
    if( !EQUALN(psFile->szVersion,"NITF01.",7) )
    {
        psImage->nRows = atoi(NITFGetField(szTemp,pachHeader,nOffset,8));
        psImage->nCols = atoi(NITFGetField(szTemp,pachHeader,nOffset+8,8));
        
        NITFTrimWhite( NITFGetField( psImage->szPVType, pachHeader, 
                                     nOffset+16, 3) );
        NITFTrimWhite( NITFGetField( psImage->szIREP, pachHeader, 
                                     nOffset+19, 8) );
        NITFTrimWhite( NITFGetField( psImage->szICAT, pachHeader, 
                                     nOffset+27, 8) );
        psImage->nABPP = atoi(NITFGetField(szTemp,pachHeader,nOffset+35,2));
    }

    nOffset += 38;

/* -------------------------------------------------------------------- */
/*      Do we have IGEOLO information?  In NITF 2.0 (and 1.x) 'N' means */
/*      no information, while in 2.1 this is indicated as ' ', and 'N'  */
/*      means UTM (north).  So for 2.0 products we change 'N' to ' '    */
/*      to conform to 2.1 conventions.                                  */
/* -------------------------------------------------------------------- */
    psImage->chICORDS = pachHeader[nOffset++];
    psImage->bHaveIGEOLO = FALSE;

    if( (EQUALN(psFile->szVersion,"NITF02.0",8)
         || EQUALN(psFile->szVersion,"NITF01.",7))
        && psImage->chICORDS == 'N' )
        psImage->chICORDS = ' ';

/* -------------------------------------------------------------------- */
/*      Read the image bounds.                                          */
/* -------------------------------------------------------------------- */
    psIGEOLOGCPs = (GDAL_GCP *) CPLMalloc(sizeof(GDAL_GCP) * nIGEOLOGCPCount);
    GDALInitGCPs( nIGEOLOGCPCount, psIGEOLOGCPs );

    if( psImage->chICORDS != ' ' )
    {
        int iCoord;

        psImage->bHaveIGEOLO = TRUE;
        for( iCoord = 0; iCoord < 4; iCoord++ )
        {
            const char *pszCoordPair = pachHeader + nOffset + iCoord*15;

            if( psImage->chICORDS == 'N' || psImage->chICORDS == 'S' )
            {
                psImage->nZone = 
                    atoi(NITFGetField( szTemp, pszCoordPair, 0, 2 ));

                psIGEOLOGCPs[iCoord].dfGCPX = atof(NITFGetField( szTemp, pszCoordPair, 2, 6 ));
                psIGEOLOGCPs[iCoord].dfGCPY = atof(NITFGetField( szTemp, pszCoordPair, 8, 7 ));
            }
            else if( psImage->chICORDS == 'G' || psImage->chICORDS == 'C' )
            {
                psIGEOLOGCPs[iCoord].dfGCPY = 
                    atof(NITFGetField( szTemp, pszCoordPair, 0, 2 )) 
                  + atof(NITFGetField( szTemp, pszCoordPair, 2, 2 )) / 60.0
                  + atof(NITFGetField( szTemp, pszCoordPair, 4, 2 )) / 3600.0;
                if( pszCoordPair[6] == 's' || pszCoordPair[6] == 'S' )
                    psIGEOLOGCPs[iCoord].dfGCPY *= -1;

                psIGEOLOGCPs[iCoord].dfGCPX = 
                    atof(NITFGetField( szTemp, pszCoordPair, 7, 3 )) 
                  + atof(NITFGetField( szTemp, pszCoordPair,10, 2 )) / 60.0
                  + atof(NITFGetField( szTemp, pszCoordPair,12, 2 )) / 3600.0;

                if( pszCoordPair[14] == 'w' || pszCoordPair[14] == 'W' )
                    psIGEOLOGCPs[iCoord].dfGCPX *= -1;
            }
            else if( psImage->chICORDS == 'D' )
            {  /* 'D' is Decimal Degrees */
                psIGEOLOGCPs[iCoord].dfGCPY =
                    atof(NITFGetField( szTemp, pszCoordPair, 0, 7 ));
                psIGEOLOGCPs[iCoord].dfGCPX =
                    atof(NITFGetField( szTemp, pszCoordPair, 7, 8 ));
            }      
            else if( psImage->chICORDS == 'U' )
            {
                int err;
                long nZone;
                char chHemisphere;
                NITFGetField( szTemp, pszCoordPair, 0, 15 );
                
                CPLDebug( "NITF", "IGEOLO = %15.15s", pszCoordPair );
                err = Convert_MGRS_To_UTM( szTemp, &nZone, &chHemisphere,
                                           &(psIGEOLOGCPs[iCoord].dfGCPX), 
										   &(psIGEOLOGCPs[iCoord].dfGCPY) );

                if( chHemisphere == 'S' )
                    nZone = -1 * nZone;

                if( psImage->nZone != 0 && psImage->nZone != -100 )
                {
                    if( nZone != psImage->nZone )
                    {
                        CPLError( CE_Warning, CPLE_AppDefined,
                                  "Some IGEOLO points are in different UTM\n"
                                  "zones, but this configuration isn't currently\n"
                                  "supported by GDAL, ignoring IGEOLO." );
                        psImage->nZone = -100;
                    }
                }
                else if( psImage->nZone == 0 )
                {
                    psImage->nZone = nZone;
                }
            }
        }

        if( psImage->nZone == -100 )
            psImage->nZone = 0;

        nOffset += 60;
    }

/* -------------------------------------------------------------------- */
/*      Read the image comments.                                        */
/* -------------------------------------------------------------------- */
    {
        int nNICOM;

        nNICOM = atoi(NITFGetField( szTemp, pachHeader, nOffset++, 1));

        psImage->pszComments = (char *) CPLMalloc(nNICOM*80+1);
        NITFGetField( psImage->pszComments, pachHeader,
                      nOffset, 80 * nNICOM );
        nOffset += nNICOM * 80;
    }
    
/* -------------------------------------------------------------------- */
/*      Read more stuff.                                                */
/* -------------------------------------------------------------------- */
    NITFGetField( psImage->szIC, pachHeader, nOffset, 2 );
    nOffset += 2;

    if( psImage->szIC[0] != 'N' )
    {
        NITFGetField( psImage->szCOMRAT, pachHeader, nOffset, 4 );
        nOffset += 4;
    }

    /* NBANDS */
    psImage->nBands = atoi(NITFGetField(szTemp,pachHeader,nOffset,1));
    nOffset++;

    /* XBANDS */
    if( psImage->nBands == 0 )
    {
        psImage->nBands = atoi(NITFGetField(szTemp,pachHeader,nOffset,5));
        nOffset += 5;
    }

/* -------------------------------------------------------------------- */
/*      Read per-band information.                                      */
/* -------------------------------------------------------------------- */
    psImage->pasBandInfo = (NITFBandInfo *) 
        CPLCalloc(sizeof(NITFBandInfo),psImage->nBands);
    
    for( iBand = 0; iBand < psImage->nBands; iBand++ )
    {
        NITFBandInfo *psBandInfo = psImage->pasBandInfo + iBand;
        int nLUTS;

        NITFTrimWhite(
            NITFGetField( psBandInfo->szIREPBAND, pachHeader, nOffset, 2 ) );
        nOffset += 2;

        NITFTrimWhite(
            NITFGetField( psBandInfo->szISUBCAT, pachHeader, nOffset, 6 ) );
        nOffset += 6;

        nOffset += 4; /* Skip IFCn and IMFLTn */

        nLUTS = atoi(NITFGetField( szTemp, pachHeader, nOffset, 1 ));
        nOffset += 1;

        if( nLUTS == 0 )
            continue;

        psBandInfo->nSignificantLUTEntries = 
            atoi(NITFGetField( szTemp, pachHeader, nOffset, 5 ));
        nOffset += 5;

        psBandInfo->nLUTLocation = nOffset + psSegInfo->nSegmentHeaderStart;

        psBandInfo->pabyLUT = (unsigned char *) CPLCalloc(768,1);
        memcpy( psBandInfo->pabyLUT, pachHeader + nOffset, 
                psBandInfo->nSignificantLUTEntries );
        nOffset += psBandInfo->nSignificantLUTEntries;

        if( nLUTS == 3 )
        {
            memcpy( psBandInfo->pabyLUT+256, pachHeader + nOffset, 
                    psBandInfo->nSignificantLUTEntries );
            nOffset += psBandInfo->nSignificantLUTEntries;
            
            memcpy( psBandInfo->pabyLUT+512, pachHeader + nOffset, 
                    psBandInfo->nSignificantLUTEntries );
            nOffset += psBandInfo->nSignificantLUTEntries;
        }
        else 
        {
            /* morph greyscale lut into RGB LUT. */
            memcpy( psBandInfo->pabyLUT+256, psBandInfo->pabyLUT, 256 );
            memcpy( psBandInfo->pabyLUT+512, psBandInfo->pabyLUT, 256 );
        }
    }								

/* -------------------------------------------------------------------- */
/*      Some files (ie NSIF datasets) have truncated image              */
/*      headers.  This has been observed with jpeg compressed           */
/*      files.  In this case guess reasonable values for these          */
/*      fields.                                                         */
/* -------------------------------------------------------------------- */
    if( nOffset + 40 > psSegInfo->nSegmentHeaderSize )
    {
        psImage->chIMODE = 'B';
        psImage->nBlocksPerRow = 1;
        psImage->nBlocksPerColumn = 1;
        psImage->nBlockWidth = psImage->nCols;
        psImage->nBlockHeight = psImage->nRows;
        psImage->nBitsPerSample = psImage->nABPP;
        psImage->nIDLVL = 0;
        psImage->nIALVL = 0;
        psImage->nILOCRow = 0;
        psImage->nILOCColumn = 0;
        psImage->szIMAG[0] = '\0';

        nOffset += 40;
    }

/* -------------------------------------------------------------------- */
/*      Read more header fields.                                        */
/* -------------------------------------------------------------------- */
    else
    {
        psImage->chIMODE = pachHeader[nOffset + 1];
        
        psImage->nBlocksPerRow = 
            atoi(NITFGetField(szTemp, pachHeader, nOffset+2, 4));
        psImage->nBlocksPerColumn = 
            atoi(NITFGetField(szTemp, pachHeader, nOffset+6, 4));
        psImage->nBlockWidth = 
            atoi(NITFGetField(szTemp, pachHeader, nOffset+10, 4));
        psImage->nBlockHeight = 
            atoi(NITFGetField(szTemp, pachHeader, nOffset+14, 4));
        psImage->nBitsPerSample = 
            atoi(NITFGetField(szTemp, pachHeader, nOffset+18, 2));
        
        if( psImage->nABPP == 0 )
            psImage->nABPP = psImage->nBitsPerSample;

        nOffset += 20;

        /* capture image inset information */

        psImage->nIDLVL = atoi(NITFGetField(szTemp,pachHeader, nOffset+0, 3));
        psImage->nIALVL = atoi(NITFGetField(szTemp,pachHeader, nOffset+3, 3));
        psImage->nILOCRow = atoi(NITFGetField(szTemp,pachHeader,nOffset+6,5));
        psImage->nILOCColumn = 
            atoi(NITFGetField(szTemp,pachHeader, nOffset+11,5));

        memcpy( psImage->szIMAG, pachHeader+nOffset+16, 4 );
        psImage->szIMAG[4] = '\0';
        
        nOffset += 3;                   /* IDLVL */
        nOffset += 3;                   /* IALVL */
        nOffset += 10;                  /* ILOC */
        nOffset += 4;                   /* IMAG */
    }

/* -------------------------------------------------------------------- */
/*      Override nCols and nRows for NITF 1.1 (not sure why!)           */
/* -------------------------------------------------------------------- */
    if( EQUALN(psFile->szVersion,"NITF01.",7) )
    {
        psImage->nCols = psImage->nBlocksPerRow * psImage->nBlockWidth;
        psImage->nRows = psImage->nBlocksPerColumn * psImage->nBlockHeight;
    }

/* -------------------------------------------------------------------- */
/*      Read TREs if we have them.                                      */
/* -------------------------------------------------------------------- */
    else if( nOffset+10 <= psSegInfo->nSegmentHeaderSize )
    {
        int nUserTREBytes, nExtendedTREBytes;
        
/* -------------------------------------------------------------------- */
/*      Are there user TRE bytes to skip?                               */
/* -------------------------------------------------------------------- */
        nUserTREBytes = atoi(NITFGetField( szTemp, pachHeader, nOffset, 5 ));
        nOffset += 5;

        if( nUserTREBytes > 0 )
        {
            psImage->nTREBytes = nUserTREBytes - 3;
            psImage->pachTRE = (char *) CPLMalloc(psImage->nTREBytes);
            memcpy( psImage->pachTRE, pachHeader + nOffset + 3,
                    psImage->nTREBytes );

            nOffset += nUserTREBytes;
        }
        else
        {
            psImage->nTREBytes = 0;
            psImage->pachTRE = NULL;
        }

/* -------------------------------------------------------------------- */
/*      Are there managed TRE bytes to recognise?                       */
/* -------------------------------------------------------------------- */
        nExtendedTREBytes = atoi(NITFGetField(szTemp,pachHeader,nOffset,5));
        nOffset += 5;

        if( nExtendedTREBytes != 0 )
        {
            psImage->pachTRE = (char *) 
                CPLRealloc( psImage->pachTRE, 
                            psImage->nTREBytes + nExtendedTREBytes - 3 );
            memcpy( psImage->pachTRE + psImage->nTREBytes, 
                    pachHeader + nOffset + 3, 
                    nExtendedTREBytes - 3 );

            psImage->nTREBytes += (nExtendedTREBytes - 3);
            nOffset += nExtendedTREBytes;
        }
    }

/* -------------------------------------------------------------------- */
/*      Is there a location table to load?                              */
/* -------------------------------------------------------------------- */
    NITFLoadLocationTable( psImage );

/* -------------------------------------------------------------------- */
/*      Setup some image access values.  Some of these may not apply    */
/*      for compressed images, or band interleaved by block images.     */
/* -------------------------------------------------------------------- */
    psImage->nWordSize = psImage->nBitsPerSample / 8;
    if( psImage->chIMODE == 'S' )
    {
        psImage->nPixelOffset = psImage->nWordSize;
        psImage->nLineOffset = psImage->nBlockWidth * psImage->nPixelOffset;
        psImage->nBlockOffset = psImage->nLineOffset * psImage->nBlockHeight;
        psImage->nBandOffset = psImage->nBlockOffset * psImage->nBlocksPerRow 
            * psImage->nBlocksPerColumn;
    }
    else if( psImage->chIMODE == 'P' )
    {
        psImage->nPixelOffset = psImage->nWordSize * psImage->nBands;
        psImage->nLineOffset = psImage->nBlockWidth * psImage->nPixelOffset;
        psImage->nBandOffset = psImage->nWordSize;
        psImage->nBlockOffset = psImage->nLineOffset * psImage->nBlockHeight;
    }
    else if( psImage->chIMODE == 'R' )
    {
        psImage->nPixelOffset = psImage->nWordSize;
        psImage->nBandOffset = psImage->nBlockWidth * psImage->nPixelOffset;
        psImage->nLineOffset = psImage->nBandOffset * psImage->nBands;
        psImage->nBlockOffset = psImage->nLineOffset * psImage->nBlockHeight;
    }
    else if( psImage->chIMODE == 'B' )
    {
        psImage->nPixelOffset = psImage->nWordSize;
        psImage->nLineOffset = psImage->nBlockWidth * psImage->nPixelOffset;
        psImage->nBandOffset = psImage->nBlockHeight * psImage->nLineOffset;
        psImage->nBlockOffset = psImage->nBandOffset * psImage->nBands;
    }
    else
    {
        psImage->nPixelOffset = psImage->nWordSize;
        psImage->nLineOffset = psImage->nBlockWidth * psImage->nPixelOffset;
        psImage->nBandOffset = psImage->nBlockHeight * psImage->nLineOffset;
        psImage->nBlockOffset = psImage->nBandOffset * psImage->nBands;
    }

/* -------------------------------------------------------------------- */
/*      Setup block map.                                                */
/* -------------------------------------------------------------------- */
    psImage->panBlockStart = (GUInt32 *) 
        CPLCalloc( psImage->nBlocksPerRow * psImage->nBlocksPerColumn 
                   * psImage->nBands, sizeof(GUInt32) );

/* -------------------------------------------------------------------- */
/*      Offsets to VQ compressed tiles are based on a fixed block       */
/*      size, and are offset from the spatial data location kept in     */
/*      the location table ... which is generally not the beginning     */
/*      of the image data segment.                                      */
/* -------------------------------------------------------------------- */
    if( EQUAL(psImage->szIC,"C4") )
    {
        GUInt32  nLocBase = psSegInfo->nSegmentStart;

        for( i = 0; i < psImage->nLocCount; i++ )
        {
            if( psImage->pasLocations[i].nLocId == LID_SpatialDataSubsection )
                nLocBase = psImage->pasLocations[i].nLocOffset;
        }

        if( nLocBase == psSegInfo->nSegmentStart )
            CPLError( CE_Warning, CPLE_AppDefined, 
                      "Failed to find spatial data location, guessing." );

        for( i=0; i < psImage->nBlocksPerRow * psImage->nBlocksPerColumn; i++ )
            psImage->panBlockStart[i] = nLocBase + 6144 * i;
    }

/* -------------------------------------------------------------------- */
/*      If there is no block map, just compute directly assuming the    */
/*      blocks start at the beginning of the image segment, and are     */
/*      packed tightly with the IMODE organization.                     */
/* -------------------------------------------------------------------- */
    else if( psImage->szIC[0] != 'M' && psImage->szIC[1] != 'M' )
    {
        int iBlockX, iBlockY, iBand;

        for( iBlockY = 0; iBlockY < psImage->nBlocksPerColumn; iBlockY++ )
        {
            for( iBlockX = 0; iBlockX < psImage->nBlocksPerRow; iBlockX++ )
            {
                for( iBand = 0; iBand < psImage->nBands; iBand++ )
                {
                    int iBlock;

                    iBlock = iBlockX + iBlockY * psImage->nBlocksPerRow
                        + iBand * psImage->nBlocksPerRow 
                        * psImage->nBlocksPerColumn;
                    
                    psImage->panBlockStart[iBlock] = 
                        psSegInfo->nSegmentStart
                        + ((iBlockX + iBlockY * psImage->nBlocksPerRow) 
                           * psImage->nBlockOffset)
                        + (iBand * psImage->nBandOffset );
                }
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      Otherwise we need to read the block map from the beginning      */
/*      of the image segment.                                           */
/* -------------------------------------------------------------------- */
    else
    {
        GUInt32  nIMDATOFF;
        GUInt16  nBMRLNTH, nTMRLNTH, nTPXCDLNTH;
        int nBlockCount;

        nBlockCount = psImage->nBlocksPerRow * psImage->nBlocksPerColumn
            * psImage->nBands;

        CPLAssert( psImage->szIC[0] == 'M' || psImage->szIC[1] == 'M' );

        VSIFSeekL( psFile->fp, psSegInfo->nSegmentStart, SEEK_SET );
        VSIFReadL( &nIMDATOFF, 1, 4, psFile->fp );
        VSIFReadL( &nBMRLNTH, 1, 2, psFile->fp );
        VSIFReadL( &nTMRLNTH, 1, 2, psFile->fp );
        VSIFReadL( &nTPXCDLNTH, 1, 2, psFile->fp );

        CPL_MSBPTR32( &nIMDATOFF );
        CPL_MSBPTR16( &nBMRLNTH );
        CPL_MSBPTR16( &nTMRLNTH );
        CPL_MSBPTR16( &nTPXCDLNTH );

        if( nTPXCDLNTH == 8 )
        {
            GByte byNodata;

            psImage->bNoDataSet = TRUE;
            VSIFReadL( &byNodata, 1, 1, psFile->fp );
            psImage->nNoDataValue = byNodata;
        }
        else
            VSIFSeekL( psFile->fp, (nTPXCDLNTH+7)/8, SEEK_CUR );

        if( nBMRLNTH == 4 && psImage->chIMODE == 'P' )
        {
            int nStoredBlocks = psImage->nBlocksPerRow 
                * psImage->nBlocksPerColumn; 
            int iBand;

            VSIFReadL( psImage->panBlockStart, 4, nStoredBlocks, psFile->fp );

            for( i = 0; i < nStoredBlocks; i++ )
            {
                CPL_MSBPTR32( psImage->panBlockStart + i );
                if( psImage->panBlockStart[i] != 0xffffffff )
                {
                    psImage->panBlockStart[i] 
                        += psSegInfo->nSegmentStart + nIMDATOFF;

                    for( iBand = 1; iBand < psImage->nBands; iBand++ )
                    {
                        psImage->panBlockStart[i + iBand * nStoredBlocks] = 
                            psImage->panBlockStart[i] 
                            + iBand * psImage->nBandOffset;
                    }
                }
                else
                {
                    for( iBand = 1; iBand < psImage->nBands; iBand++ )
                        psImage->panBlockStart[i + iBand * nStoredBlocks] = 
                            0xffffffff;
                }
            }
        }
        else if( nBMRLNTH == 4 )
        {
            VSIFReadL( psImage->panBlockStart, 4, nBlockCount, psFile->fp );
            for( i=0; i < nBlockCount; i++ )
            {
                CPL_MSBPTR32( psImage->panBlockStart + i );
                if( psImage->panBlockStart[i] != 0xffffffff )
                    psImage->panBlockStart[i] 
                        += psSegInfo->nSegmentStart + nIMDATOFF;
            }
        }
        else
        {
            for( i=0; i < nBlockCount; i++ )
            {
                if( EQUAL(psImage->szIC,"M4") )
                    psImage->panBlockStart[i] = 6144 * i
                        + psSegInfo->nSegmentStart + nIMDATOFF;
                else if( EQUAL(psImage->szIC,"NM") )
                    psImage->panBlockStart[i] = 
                        psImage->nBlockOffset * i
                        + psSegInfo->nSegmentStart + nIMDATOFF;
            }
        }
    }

/* -------------------------------------------------------------------- */
/*  We override the coordinates found in IGEOLO in case a BLOCKA is     */
/*  present. According to the BLOCKA specification, it repeats earth    */
/*  coordinates image corner locations described by IGEOLO in the NITF  */
/*  image subheader, but provide higher precision.                      */
/* -------------------------------------------------------------------- */

    NITFReadBLOCKA_GCPs( psImage, psIGEOLOGCPs );

/* -------------------------------------------------------------------- */
/*      We can't set dfGCPPixel and dfGCPPixel until we know            */
/*      psImage->nRows and psImage->nCols.                              */
/* -------------------------------------------------------------------- */

    if( psImage->chICORDS != ' ' )
    {
        psIGEOLOGCPs[0].dfGCPPixel = 0.5;
        psIGEOLOGCPs[0].dfGCPLine = 0.5;
        psIGEOLOGCPs[1].dfGCPPixel = psImage->nCols - 0.5;
        psIGEOLOGCPs[1].dfGCPLine = 0.5;
        psIGEOLOGCPs[2].dfGCPPixel = psImage->nCols - 0.5;
        psIGEOLOGCPs[2].dfGCPLine = psImage->nRows - 0.5;
        psIGEOLOGCPs[3].dfGCPPixel = 0.5;
        psIGEOLOGCPs[3].dfGCPLine = psImage->nRows - 0.5;

/* -------------------------------------------------------------------- */
/*      Convert the GCPs into a geotransform definition, if possible.	*/
/* -------------------------------------------------------------------- */
        if( !GDALGCPsToGeoTransform( nIGEOLOGCPCount, psIGEOLOGCPs, 
                                     adfGeoTransform, TRUE ) )
        {
            CPLDebug( "GDAL", "NITFImageAccess() wasn't able to derive a\n"
                      "first order geotransform.");
        }
        
        psImage->dfULX = adfGeoTransform[0];
        psImage->dfULY = adfGeoTransform[3];
        psImage->dfURX = psImage->dfULX + adfGeoTransform[1] * psImage->nCols;
        psImage->dfURY = psImage->dfULY + adfGeoTransform[4] * psImage->nCols;
        psImage->dfLRX = psImage->dfULX + adfGeoTransform[1] * psImage->nCols
            + adfGeoTransform[2] * psImage->nRows;
        psImage->dfLRY = psImage->dfULY + adfGeoTransform[4] * psImage->nCols
            + adfGeoTransform[5] * psImage->nRows;
        psImage->dfLLX = psImage->dfULX + adfGeoTransform[2] * psImage->nRows;
        psImage->dfLLY = psImage->dfULY + adfGeoTransform[5] * psImage->nRows;
    }

    GDALDeinitGCPs( nIGEOLOGCPCount, psIGEOLOGCPs );
    CPLFree( psIGEOLOGCPs );
        
/* -------------------------------------------------------------------- */
/*      If we have an RPF CoverageSectionSubheader, read the more       */
/*      precise bounds from it.                                         */
/* -------------------------------------------------------------------- */
    for( i = 0; i < psImage->nLocCount; i++ )
    {
        if( psImage->pasLocations[i].nLocId == LID_CoverageSectionSubheader )
        {
            double adfTarget[8];

            VSIFSeekL( psFile->fp, psImage->pasLocations[i].nLocOffset,
                      SEEK_SET );
            VSIFReadL( adfTarget, 8, 8, psFile->fp );
            for( i = 0; i < 8; i++ )
                CPL_MSBPTR64( (adfTarget + i) );

            psImage->dfULX = adfTarget[1];
            psImage->dfULY = adfTarget[0];
            psImage->dfLLX = adfTarget[3];
            psImage->dfLLY = adfTarget[2];
            psImage->dfURX = adfTarget[5];
            psImage->dfURY = adfTarget[4];
            psImage->dfLRX = adfTarget[7];
            psImage->dfLRY = adfTarget[6];

            CPLDebug( "NITF", "Got spatial info from CoverageSection" );
            break;
        }
    }

/* -------------------------------------------------------------------- */
/*      Are the VQ tables to load up?                                   */
/* -------------------------------------------------------------------- */
    NITFLoadVQTables( psImage );

    return psImage;
}

/************************************************************************/
/*                         NITFImageDeaccess()                          */
/************************************************************************/

void NITFImageDeaccess( NITFImage *psImage )

{
    int  iBand;

    CPLAssert( psImage->psFile->pasSegmentInfo[psImage->iSegment].hAccess
               == psImage );

    psImage->psFile->pasSegmentInfo[psImage->iSegment].hAccess = NULL;

    for( iBand = 0; iBand < psImage->nBands; iBand++ )
        CPLFree( psImage->pasBandInfo[iBand].pabyLUT );
    CPLFree( psImage->pasBandInfo );
    CPLFree( psImage->panBlockStart );
    CPLFree( psImage->pszComments );
    CPLFree( psImage->pachHeader );
    CPLFree( psImage->pachTRE );
    CSLDestroy( psImage->papszMetadata );

    CPLFree( psImage->pasLocations );
    for( iBand = 0; iBand < 4; iBand++ )
        CPLFree( psImage->apanVQLUT[iBand] );

    CPLFree( psImage );
}

/************************************************************************/
/*                        NITFUncompressVQTile()                        */
/*                                                                      */
/*      This code was derived from OSSIM which in turn derived it       */
/*      from OpenMap ... open source means sharing!                     */
/************************************************************************/

static void NITFUncompressVQTile( NITFImage *psImage, 
                                  GByte *pabyVQBuf,
                                  GByte *pabyResult )

{
    int   i, j, t, iSrcByte = 0;

    for (i = 0; i < 256; i += 4)
    {
        for (j = 0; j < 256; j += 8)
        {
            GUInt16 firstByte  = pabyVQBuf[iSrcByte++];
            GUInt16 secondByte = pabyVQBuf[iSrcByte++];
            GUInt16 thirdByte  = pabyVQBuf[iSrcByte++];

            /*
             * because dealing with half-bytes is hard, we
             * uncompress two 4x4 tiles at the same time. (a
             * 4x4 tile compressed is 12 bits )
             * this little code was grabbed from openmap software.
             */
                  
            /* Get first 12-bit value as index into VQ table */

            GUInt16 val1 = (firstByte << 4) | (secondByte >> 4);
                  
            /* Get second 12-bit value as index into VQ table*/

            GUInt16 val2 = ((secondByte & 0x000F) << 8) | thirdByte;
                  
            for ( t = 0; t < 4; ++t)
            {
                GByte *pabyTarget = pabyResult + (i+t) * 256 + j;
                
                memcpy( pabyTarget, psImage->apanVQLUT[t] + val1, 4 );
                memcpy( pabyTarget+4, psImage->apanVQLUT[t] + val2, 4);
            }
        }  /* for j */
    } /* for i */
}

/************************************************************************/
/*                         NITFReadImageBlock()                         */
/************************************************************************/

int NITFReadImageBlock( NITFImage *psImage, int nBlockX, int nBlockY, 
                        int nBand, void *pData )

{
    int   nWrkBufSize;
    int   iBaseBlock = nBlockX + nBlockY * psImage->nBlocksPerRow;
    int   iFullBlock = iBaseBlock 
        + (nBand-1) * psImage->nBlocksPerRow * psImage->nBlocksPerColumn;

    if( nBand == 0 )
        return BLKREAD_FAIL;

    nWrkBufSize = psImage->nLineOffset * (psImage->nBlockHeight-1)
        + psImage->nPixelOffset * (psImage->nBlockWidth-1)
        + psImage->nWordSize;

    if( psImage->panBlockStart[iFullBlock] == 0xffffffff )
        return BLKREAD_NULL;

/* -------------------------------------------------------------------- */
/*      Can we do a direct read into our buffer?                        */
/* -------------------------------------------------------------------- */
    if( psImage->nWordSize == psImage->nPixelOffset
        && psImage->nWordSize * psImage->nBlockWidth == psImage->nLineOffset 
        && psImage->szIC[0] != 'C' && psImage->szIC[0] != 'M'
        && psImage->chIMODE != 'P' )
    {
        if( VSIFSeekL( psImage->psFile->fp, 
                      psImage->panBlockStart[iFullBlock], 
                      SEEK_SET ) != 0 
            || (int) VSIFReadL( pData, 1, nWrkBufSize,
                               psImage->psFile->fp ) != nWrkBufSize )
        {
            CPLError( CE_Failure, CPLE_FileIO, 
                      "Unable to read %d byte block from %d.", 
                      nWrkBufSize, psImage->panBlockStart[iFullBlock] );
            return BLKREAD_FAIL;
        }
        else
        {
#ifdef CPL_LSB
            NITFSwapWords( pData, psImage->nWordSize, 
                           psImage->nBlockWidth * psImage->nBlockHeight, 
                           psImage->nWordSize );
#endif

            return BLKREAD_OK;
        }
    }

/* -------------------------------------------------------------------- */
/*      Read the requested information into a temporary buffer and      */
/*      pull out what we want.                                          */
/* -------------------------------------------------------------------- */
    if( psImage->szIC[0] == 'N' )
    {
        GByte *pabyWrkBuf = (GByte *) CPLMalloc(nWrkBufSize);
        int   iPixel, iLine;

        /* read all the data needed to get our requested band-block */
        if( VSIFSeekL( psImage->psFile->fp, psImage->panBlockStart[iFullBlock], 
                      SEEK_SET ) != 0 
            || (int) VSIFReadL( pabyWrkBuf, 1, nWrkBufSize,
                               psImage->psFile->fp ) != nWrkBufSize )
        {
            CPLError( CE_Failure, CPLE_FileIO, 
                      "Unable to read %d byte block from %d.", 
                      nWrkBufSize, psImage->panBlockStart[iFullBlock] );
            return BLKREAD_FAIL;
        }

        for( iLine = 0; iLine < psImage->nBlockHeight; iLine++ )
        {
            GByte *pabySrc, *pabyDst;

            pabySrc = pabyWrkBuf + iLine * psImage->nLineOffset;
            pabyDst = ((GByte *) pData) 
                + iLine * (psImage->nWordSize * psImage->nBlockWidth);

            for( iPixel = 0; iPixel < psImage->nBlockWidth; iPixel++ )
            {
                memcpy( pabyDst + iPixel * psImage->nWordSize, 
                        pabySrc + iPixel * psImage->nPixelOffset,
                        psImage->nWordSize );
            }
        }

#ifdef CPL_LSB
        NITFSwapWords( pData, psImage->nWordSize, 
                       psImage->nBlockWidth * psImage->nBlockHeight, 
                       psImage->nWordSize );
#endif

        CPLFree( pabyWrkBuf );

        return BLKREAD_OK;
    }

/* -------------------------------------------------------------------- */
/*      Handle VQ compression.  The VQ compression basically keeps a    */
/*      64x64 array of 12bit code words.  Each code word expands to     */
/*      a predefined 4x4 8 bit per pixel pattern.                       */
/* -------------------------------------------------------------------- */
    else if( EQUAL(psImage->szIC,"C4") || EQUAL(psImage->szIC,"M4") )
    {
        GByte abyVQCoded[6144];

        if( psImage->apanVQLUT[0] == NULL )
        {
            CPLError( CE_Failure, CPLE_NotSupported, 
                      "File lacks VQ LUTs, unable to decode imagery." );
            return BLKREAD_FAIL;
        }

        /* Read the codewords */
        if( VSIFSeekL(psImage->psFile->fp, psImage->panBlockStart[iFullBlock], 
                      SEEK_SET ) != 0 
            || VSIFReadL(abyVQCoded, 1, sizeof(abyVQCoded),
                         psImage->psFile->fp ) != sizeof(abyVQCoded) )
        {
            CPLError( CE_Failure, CPLE_FileIO, 
                      "Unable to read %d byte block from %d.", 
                      sizeof(abyVQCoded), psImage->panBlockStart[iFullBlock] );
            return BLKREAD_FAIL;
        }
        
        NITFUncompressVQTile( psImage, abyVQCoded, pData );

        return BLKREAD_OK;
    }

/* -------------------------------------------------------------------- */
/*      Report unsupported compression scheme(s).                       */
/* -------------------------------------------------------------------- */
    else if( atoi(psImage->szIC + 1) > 0 )
    {
        CPLError( CE_Failure, CPLE_NotSupported, 
                  "Unsupported imagery compression format %s in NITF library.",
                  psImage->szIC );
        return BLKREAD_FAIL;
    }

    return BLKREAD_FAIL;
}

/************************************************************************/
/*                        NITFWriteImageBlock()                         */
/************************************************************************/

int NITFWriteImageBlock( NITFImage *psImage, int nBlockX, int nBlockY, 
                         int nBand, void *pData )

{
    int   nWrkBufSize;
    int   iBaseBlock = nBlockX + nBlockY * psImage->nBlocksPerRow;
    int   iFullBlock = iBaseBlock 
        + (nBand-1) * psImage->nBlocksPerRow * psImage->nBlocksPerColumn;

    if( nBand == 0 )
        return BLKREAD_FAIL;

    nWrkBufSize = psImage->nLineOffset * (psImage->nBlockHeight-1)
        + psImage->nPixelOffset * (psImage->nBlockWidth-1)
        + psImage->nWordSize;

/* -------------------------------------------------------------------- */
/*      Can we do a direct read into our buffer?                        */
/* -------------------------------------------------------------------- */
    if( psImage->nWordSize == psImage->nPixelOffset
        && psImage->nWordSize * psImage->nBlockWidth == psImage->nLineOffset 
        && psImage->szIC[0] != 'C' && psImage->szIC[0] != 'M' )
    {
#ifdef CPL_LSB
        NITFSwapWords( pData, psImage->nWordSize, 
                       psImage->nBlockWidth * psImage->nBlockHeight, 
                       psImage->nWordSize );
#endif

        if( VSIFSeekL( psImage->psFile->fp, psImage->panBlockStart[iFullBlock], 
                      SEEK_SET ) != 0 
            || (int) VSIFWriteL( pData, 1, nWrkBufSize,
                                psImage->psFile->fp ) != nWrkBufSize )
        {
            CPLError( CE_Failure, CPLE_FileIO, 
                      "Unable to write %d byte block from %d.", 
                      nWrkBufSize, psImage->panBlockStart[iFullBlock] );
            return BLKREAD_FAIL;
        }
        else
        {
#ifdef CPL_LSB
            /* restore byte order to original */
            NITFSwapWords( pData, psImage->nWordSize, 
                           psImage->nBlockWidth * psImage->nBlockHeight, 
                           psImage->nWordSize );
#endif

            return BLKREAD_OK;
        }
    }

/* -------------------------------------------------------------------- */
/*      Other forms not supported at this time.                         */
/* -------------------------------------------------------------------- */
    CPLError( CE_Failure, CPLE_NotSupported, 
              "Mapped, interleaved and compressed NITF forms not supported\n"
              "for writing at this time." );

    return BLKREAD_FAIL;
}

/************************************************************************/
/*                         NITFReadImageLine()                          */
/************************************************************************/

int NITFReadImageLine( NITFImage *psImage, int nLine, int nBand, void *pData )

{
    int   nLineOffsetInFile, nLineSize;
    unsigned char *pabyLineBuf;

    if( nBand == 0 )
        return BLKREAD_FAIL;

    if( psImage->nBlocksPerRow != 1 || psImage->nBlocksPerColumn != 1 )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Scanline access not supported on tiled NITF files." );
        return BLKREAD_FAIL;
    }

    if( !EQUAL(psImage->szIC,"NC") )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Scanline access not supported on compressed NITF files." );
        return BLKREAD_FAIL;
    }

/* -------------------------------------------------------------------- */
/*      Workout location and size of data in file.                      */
/* -------------------------------------------------------------------- */
    nLineOffsetInFile = psImage->panBlockStart[0]
        + psImage->nLineOffset * nLine
        + psImage->nBandOffset * (nBand-1);

    nLineSize = psImage->nPixelOffset * (psImage->nCols - 1) 
        + psImage->nWordSize;

    VSIFSeekL( psImage->psFile->fp, nLineOffsetInFile, SEEK_SET );

/* -------------------------------------------------------------------- */
/*      Can we do a direct read into our buffer.                        */
/* -------------------------------------------------------------------- */
    if( psImage->nWordSize == psImage->nPixelOffset
        && psImage->nWordSize * psImage->nBlockWidth == psImage->nLineOffset )
    {
        VSIFReadL( pData, 1, nLineSize, psImage->psFile->fp );

#ifdef CPL_LSB
        NITFSwapWords( pData, psImage->nWordSize, 
                       psImage->nBlockWidth, psImage->nWordSize );
#endif

        return BLKREAD_OK;
    }

/* -------------------------------------------------------------------- */
/*      Allocate a buffer for all the interleaved data, and read        */
/*      it.                                                             */
/* -------------------------------------------------------------------- */
    pabyLineBuf = (unsigned char *) CPLMalloc(nLineSize);
    VSIFReadL( pabyLineBuf, 1, nLineSize, psImage->psFile->fp );

/* -------------------------------------------------------------------- */
/*      Copy the desired data out of the interleaved buffer.            */
/* -------------------------------------------------------------------- */
    {
        GByte *pabySrc, *pabyDst;
        int iPixel;
        
        pabySrc = pabyLineBuf;
        pabyDst = ((GByte *) pData);
        
        for( iPixel = 0; iPixel < psImage->nBlockWidth; iPixel++ )
        {
            memcpy( pabyDst + iPixel * psImage->nWordSize, 
                    pabySrc + iPixel * psImage->nPixelOffset,
                    psImage->nWordSize );
        }

#ifdef CPL_LSB
        NITFSwapWords( (void *) pabyDst, psImage->nWordSize, 
                       psImage->nBlockWidth, psImage->nWordSize );
#endif
    }

    CPLFree( pabyLineBuf );

    return BLKREAD_OK;
}

/************************************************************************/
/*                         NITFWriteImageLine()                         */
/************************************************************************/

int NITFWriteImageLine( NITFImage *psImage, int nLine, int nBand, void *pData )

{
    int   nLineOffsetInFile, nLineSize;
    unsigned char *pabyLineBuf;

    if( nBand == 0 )
        return BLKREAD_FAIL;

    if( psImage->nBlocksPerRow != 1 || psImage->nBlocksPerColumn != 1 )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Scanline access not supported on tiled NITF files." );
        return BLKREAD_FAIL;
    }

    if( !EQUAL(psImage->szIC,"NC") )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Scanline access not supported on compressed NITF files." );
        return BLKREAD_FAIL;
    }

/* -------------------------------------------------------------------- */
/*      Workout location and size of data in file.                      */
/* -------------------------------------------------------------------- */
    nLineOffsetInFile = psImage->panBlockStart[0]
        + psImage->nLineOffset * nLine
        + psImage->nBandOffset * (nBand-1);

    nLineSize = psImage->nPixelOffset * (psImage->nCols - 1) 
        + psImage->nWordSize;

    VSIFSeekL( psImage->psFile->fp, nLineOffsetInFile, SEEK_SET );

/* -------------------------------------------------------------------- */
/*      Can we do a direct write into our buffer.                       */
/* -------------------------------------------------------------------- */
    if( psImage->nWordSize == psImage->nPixelOffset
        && psImage->nWordSize * psImage->nBlockWidth == psImage->nLineOffset )
    {
#ifdef CPL_LSB
        NITFSwapWords( (void *) pData, psImage->nWordSize, 
                       psImage->nCols, psImage->nWordSize );
#endif

        VSIFWriteL( pData, 1, nLineSize, psImage->psFile->fp );

#ifdef CPL_LSB
        NITFSwapWords( (void *) pData, psImage->nWordSize, 
                       psImage->nCols, psImage->nWordSize );
#endif

        return BLKREAD_OK;
    }

/* -------------------------------------------------------------------- */
/*      Allocate a buffer for all the interleaved data, and read        */
/*      it.                                                             */
/* -------------------------------------------------------------------- */
    pabyLineBuf = (unsigned char *) CPLMalloc(nLineSize);
    VSIFReadL( pabyLineBuf, 1, nLineSize, psImage->psFile->fp );

/* -------------------------------------------------------------------- */
/*      Copy the desired data into the interleaved buffer.              */
/* -------------------------------------------------------------------- */
    {
        GByte *pabySrc, *pabyDst;
        int iPixel;
        
        pabySrc = pabyLineBuf;
        pabyDst = ((GByte *) pData);
        
        for( iPixel = 0; iPixel < psImage->nBlockWidth; iPixel++ )
        {
            memcpy( pabySrc + iPixel * psImage->nPixelOffset,
                    pabyDst + iPixel * psImage->nWordSize, 
                    psImage->nWordSize );
#ifdef CPL_LSB
        NITFSwapWords( pabyDst + iPixel * psImage->nWordSize, 
                       psImage->nWordSize, 1, psImage->nWordSize );
#endif
        }
    }

/* -------------------------------------------------------------------- */
/*      Write the results back out.                                     */
/* -------------------------------------------------------------------- */
    VSIFSeekL( psImage->psFile->fp, nLineOffsetInFile, SEEK_SET );
    VSIFWriteL( pabyLineBuf, 1, nLineSize, psImage->psFile->fp );
    CPLFree( pabyLineBuf );

    return BLKREAD_OK;
}

/************************************************************************/
/*                          NITFEncodeDMSLoc()                          */
/************************************************************************/

static void NITFEncodeDMSLoc( char *pszTarget, double dfValue, 
                              const char *pszAxis )

{
    char chHemisphere;
    int  nDegrees, nMinutes, nSeconds;

    if( EQUAL(pszAxis,"Lat") )
    {
        if( dfValue < 0.0 )
            chHemisphere = 'S';
        else
            chHemisphere = 'N';
    }
    else
    {
        if( dfValue < 0.0 )
            chHemisphere = 'W';
        else
            chHemisphere = 'E';
    }

    dfValue = fabs(dfValue);

    nDegrees = (int) dfValue;
    dfValue = (dfValue-nDegrees) * 60.0;

    nMinutes = (int) dfValue;
    dfValue = (dfValue-nMinutes) * 60.0;

/* -------------------------------------------------------------------- */
/*      Do careful rounding on seconds so that 59.9->60 is properly     */
/*      rolled into minutes and degrees.                                */
/* -------------------------------------------------------------------- */
    nSeconds = (int) (dfValue + 0.5);
    if (nSeconds == 60) 
    {
        nSeconds = 0;
        nMinutes += 1;
        if (nMinutes == 60) 
        {
            nMinutes = 0;
            nDegrees += 1;
        }
    }

    if( EQUAL(pszAxis,"Lat") )
        sprintf( pszTarget, "%02d%02d%02d%c", 
                 nDegrees, nMinutes, nSeconds, chHemisphere );
    else
        sprintf( pszTarget, "%03d%02d%02d%c", 
                 nDegrees, nMinutes, nSeconds, chHemisphere );
}

/************************************************************************/
/*                          NITFWriteIGEOLO()                           */
/************************************************************************/

int NITFWriteIGEOLO( NITFImage *psImage, char chICORDS,
                     int nZone, 
                     double dfULX, double dfULY,
                     double dfURX, double dfURY,
                     double dfLRX, double dfLRY,
                     double dfLLX, double dfLLY )

{
    char szIGEOLO[61];

/* -------------------------------------------------------------------- */
/*      Do some checking.                                               */
/* -------------------------------------------------------------------- */
    if( psImage->chICORDS == ' ' )
    {
        CPLError(CE_Failure, CPLE_NotSupported, 
                 "Apparently no space reserved for IGEOLO info in NITF file.\n"
                 "NITFWriteIGEOGLO() fails." );
        return FALSE;
    }

    if( chICORDS != 'G' && chICORDS != 'N' && chICORDS != 'S' )
    {
        CPLError( CE_Failure, CPLE_NotSupported, 
                  "Currently NITFWriteIGEOLO() only supports writing ICORDS=G, N and S corners." );
        return FALSE;
    }

/* -------------------------------------------------------------------- */
/*      Format geographic coordinates.                                  */
/* -------------------------------------------------------------------- */
    if( chICORDS == 'G' )
    {
        if( fabs(dfULX) > 180 || fabs(dfURX) > 180 
            || fabs(dfLRX) > 180 || fabs(dfLLX) > 180 
            || fabs(dfULY) >  90 || fabs(dfURY) >  90
            || fabs(dfLRY) >  90 || fabs(dfLLY) >  90 )
        {
            CPLError( CE_Failure, CPLE_AppDefined, 
                      "Attempt to write geographic bound outside of legal range." );
            return FALSE;
        }

        NITFEncodeDMSLoc( szIGEOLO +  0, dfULY, "Lat" );
        NITFEncodeDMSLoc( szIGEOLO +  7, dfULX, "Long" );
        NITFEncodeDMSLoc( szIGEOLO + 15, dfURY, "Lat" );
        NITFEncodeDMSLoc( szIGEOLO + 22, dfURX, "Long" );
        NITFEncodeDMSLoc( szIGEOLO + 30, dfLRY, "Lat" );
        NITFEncodeDMSLoc( szIGEOLO + 37, dfLRX, "Long" );
        NITFEncodeDMSLoc( szIGEOLO + 45, dfLLY, "Lat" );
        NITFEncodeDMSLoc( szIGEOLO + 52, dfLLX, "Long" );
    }

/* -------------------------------------------------------------------- */
/*      Format UTM coordinates.                                         */
/* -------------------------------------------------------------------- */
    else if( chICORDS == 'N' || chICORDS == 'S' )
    {
        sprintf( szIGEOLO + 0, "%02d%06d%07d",
                 nZone, (int) floor(dfULX+0.5), (int) floor(dfULY+0.5) );
        sprintf( szIGEOLO + 15, "%02d%06d%07d",
                 nZone, (int) floor(dfURX+0.5), (int) floor(dfURY+0.5) );
        sprintf( szIGEOLO + 30, "%02d%06d%07d",
                 nZone, (int) floor(dfLRX+0.5), (int) floor(dfLRY+0.5) );
        sprintf( szIGEOLO + 45, "%02d%06d%07d",
                 nZone, (int) floor(dfLLX+0.5), (int) floor(dfLLY+0.5) );
    }

/* -------------------------------------------------------------------- */
/*      Write IGEOLO data to disk.                                      */
/* -------------------------------------------------------------------- */
    if( VSIFSeekL( psImage->psFile->fp, 
                  psImage->psFile->pasSegmentInfo[psImage->iSegment].nSegmentHeaderStart + 372, SEEK_SET ) == 0
        && VSIFWriteL( szIGEOLO, 1, 60, psImage->psFile->fp ) == 60 )
    {
        return TRUE;
    }
    else
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "I/O Error writing IGEOLO segment.\n%s",
                  VSIStrerror( errno ) );
        return FALSE;
    }
}

/************************************************************************/
/*                            NITFWriteLUT()                            */
/************************************************************************/

int NITFWriteLUT( NITFImage *psImage, int nBand, int nColors, 
                  unsigned char *pabyLUT )

{
    NITFBandInfo *psBandInfo;
    int           bSuccess = TRUE;

    if( nBand < 1 || nBand > psImage->nBands )
        return FALSE;

    psBandInfo = psImage->pasBandInfo + (nBand-1);

    if( nColors > psBandInfo->nSignificantLUTEntries )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "Unable to write all %d LUT entries, only able to write %d.",
                  nColors, psBandInfo->nSignificantLUTEntries );
        nColors = psBandInfo->nSignificantLUTEntries;
        bSuccess = FALSE;
    }

    VSIFSeekL( psImage->psFile->fp, psBandInfo->nLUTLocation, SEEK_SET );
    VSIFWriteL( pabyLUT, 1, nColors, psImage->psFile->fp );
    VSIFSeekL( psImage->psFile->fp, 
              psBandInfo->nLUTLocation + psBandInfo->nSignificantLUTEntries, 
              SEEK_SET );
    VSIFWriteL( pabyLUT+256, 1, nColors, psImage->psFile->fp );
    VSIFSeekL( psImage->psFile->fp, 
              psBandInfo->nLUTLocation + 2*psBandInfo->nSignificantLUTEntries, 
              SEEK_SET );
    VSIFWriteL( pabyLUT+512, 1, nColors, psImage->psFile->fp );

    return bSuccess;
}



/************************************************************************/
/*                           NITFTrimWhite()                            */
/*                                                                      */
/*      Trim any white space off the white of the passed string in      */
/*      place.                                                          */
/************************************************************************/

char *NITFTrimWhite( char *pszTarget )

{
    int i;

    i = strlen(pszTarget)-1;
    while( i >= 0 && pszTarget[i] == ' ' )
        pszTarget[i--] = '\0';

    return pszTarget;
}

/************************************************************************/
/*                           NITFSwapWords()                            */
/************************************************************************/

#ifdef CPL_LSB
static void NITFSwapWords( void *pData, int nWordSize, int nWordCount,
                           int nWordSkip )

{
    int         i;
    GByte       *pabyData = (GByte *) pData;

    switch( nWordSize )
    {
      case 1:
        break;

      case 2:
        for( i = 0; i < nWordCount; i++ )
        {
            GByte       byTemp;

            byTemp = pabyData[0];
            pabyData[0] = pabyData[1];
            pabyData[1] = byTemp;

            pabyData += nWordSkip;
        }
        break;
        
      case 4:
        for( i = 0; i < nWordCount; i++ )
        {
            GByte       byTemp;

            byTemp = pabyData[0];
            pabyData[0] = pabyData[3];
            pabyData[3] = byTemp;

            byTemp = pabyData[1];
            pabyData[1] = pabyData[2];
            pabyData[2] = byTemp;

            pabyData += nWordSkip;
        }
        break;

      case 8:
        for( i = 0; i < nWordCount; i++ )
        {
            GByte       byTemp;

            byTemp = pabyData[0];
            pabyData[0] = pabyData[7];
            pabyData[7] = byTemp;

            byTemp = pabyData[1];
            pabyData[1] = pabyData[6];
            pabyData[6] = byTemp;

            byTemp = pabyData[2];
            pabyData[2] = pabyData[5];
            pabyData[5] = byTemp;

            byTemp = pabyData[3];
            pabyData[3] = pabyData[4];
            pabyData[4] = byTemp;

            pabyData += nWordSkip;
        }
        break;

      default:
        break;
    }
}

#endif /* def CPL_LSB */

/************************************************************************/
/*                           NITFReadRPC00B()                           */
/*                                                                      */
/*      Read an RPC00B structure if the TRE is available.               */
/************************************************************************/

int NITFReadRPC00B( NITFImage *psImage, NITFRPC00BInfo *psRPC )

{
    const char *pachTRE;
    char szTemp[100];
    int  i;

    psRPC->SUCCESS = 0;

/* -------------------------------------------------------------------- */
/*      Do we have the TRE?                                             */
/* -------------------------------------------------------------------- */
    pachTRE = NITFFindTRE( psImage->pachTRE, psImage->nTREBytes, 
                           "RPC00B", NULL );

    if( pachTRE == NULL )
	{
		pachTRE = NITFFindTRE( psImage->pachTRE, psImage->nTREBytes,
						"RPC00A", NULL );
	}

	if( pachTRE == NULL )
	{
        return FALSE;
	}

/* -------------------------------------------------------------------- */
/*      Parse out field values.                                         */
/* -------------------------------------------------------------------- */
    psRPC->SUCCESS = atoi(NITFGetField(szTemp, pachTRE, 0, 1 ));
	
	if ( !psRPC->SUCCESS )
	fprintf( stdout, "RPC Extension not Populated!\n");

    psRPC->ERR_BIAS = atof(NITFGetField(szTemp, pachTRE, 1, 7 ));
    psRPC->ERR_RAND = atof(NITFGetField(szTemp, pachTRE, 8, 7 ));

    psRPC->LINE_OFF = atof(NITFGetField(szTemp, pachTRE, 15, 6 ));
    psRPC->SAMP_OFF = atof(NITFGetField(szTemp, pachTRE, 21, 5 ));
    psRPC->LAT_OFF = atof(NITFGetField(szTemp, pachTRE, 26, 8 ));
    psRPC->LONG_OFF = atof(NITFGetField(szTemp, pachTRE, 34, 9 ));
    psRPC->HEIGHT_OFF = atof(NITFGetField(szTemp, pachTRE, 43, 5 ));

    psRPC->LINE_SCALE = atof(NITFGetField(szTemp, pachTRE, 48, 6 ));
    psRPC->SAMP_SCALE = atof(NITFGetField(szTemp, pachTRE, 54, 5 ));
    psRPC->LAT_SCALE = atof(NITFGetField(szTemp, pachTRE, 59, 8 ));
    psRPC->LONG_SCALE = atof(NITFGetField(szTemp, pachTRE, 67, 9 ));
    psRPC->HEIGHT_SCALE = atof(NITFGetField(szTemp, pachTRE, 76, 5 ));

/* -------------------------------------------------------------------- */
/*      Parse out coefficients.                                         */
/* -------------------------------------------------------------------- */
    for( i = 0; i < 20; i++ )
    {
        psRPC->LINE_NUM_COEFF[i] = 
            atof(NITFGetField(szTemp, pachTRE, 81+i*12, 12));
        psRPC->LINE_DEN_COEFF[i] = 
            atof(NITFGetField(szTemp, pachTRE, 321+i*12, 12));
        psRPC->SAMP_NUM_COEFF[i] = 
            atof(NITFGetField(szTemp, pachTRE, 561+i*12, 12));
        psRPC->SAMP_DEN_COEFF[i] = 
            atof(NITFGetField(szTemp, pachTRE, 801+i*12, 12));
    }

    return TRUE;
}

/************************************************************************/
/*                           NITFReadICHIPB()                           */
/*                                                                      */
/*      Read an ICHIPB structure if the TRE is available.               */
/************************************************************************/

int NITFReadICHIPB( NITFImage *psImage, NITFICHIPBInfo *psICHIP )

{
    const char *pachTRE;
    char szTemp[32];

/* -------------------------------------------------------------------- */
/*      Do we have the TRE?                                             */
/* -------------------------------------------------------------------- */
    pachTRE = NITFFindTRE( psImage->pachTRE, psImage->nTREBytes, 
                           "ICHIPB", NULL );

    if( pachTRE == NULL )
    {
        pachTRE = NITFFindTRE( psImage->pachTRE, psImage->nTREBytes,
                               "ICHIPA", NULL );
    }

    if( pachTRE == NULL )
    {
        return FALSE;
    }

/* -------------------------------------------------------------------- */
/*      Parse out field values.                                         */
/* -------------------------------------------------------------------- */
    psICHIP->XFRM_FLAG = atoi(NITFGetField(szTemp, pachTRE, 0, 2 ));

    if ( psICHIP->XFRM_FLAG == 0 )
    {
        psICHIP->SCALE_FACTOR = atof(NITFGetField(szTemp, pachTRE, 2, 10 ));
        psICHIP->ANAMORPH_CORR = atoi(NITFGetField(szTemp, pachTRE, 12, 2 ));
        psICHIP->SCANBLK_NUM = atoi(NITFGetField(szTemp, pachTRE, 14, 2 ));

        psICHIP->OP_ROW_11 = atof(NITFGetField(szTemp, pachTRE, 16, 12 ));
        psICHIP->OP_COL_11 = atof(NITFGetField(szTemp, pachTRE, 28, 12 ));

        psICHIP->OP_ROW_12 = atof(NITFGetField(szTemp, pachTRE, 40, 12 ));
        psICHIP->OP_COL_12 = atof(NITFGetField(szTemp, pachTRE, 52, 12 ));

        psICHIP->OP_ROW_21 = atof(NITFGetField(szTemp, pachTRE, 64, 12 ));
        psICHIP->OP_COL_21 = atof(NITFGetField(szTemp, pachTRE, 76, 12 ));

        psICHIP->OP_ROW_22 = atof(NITFGetField(szTemp, pachTRE, 88, 12 ));
        psICHIP->OP_COL_22 = atof(NITFGetField(szTemp, pachTRE, 100, 12 ));

        psICHIP->FI_ROW_11 = atof(NITFGetField(szTemp, pachTRE, 112, 12 ));
        psICHIP->FI_COL_11 = atof(NITFGetField(szTemp, pachTRE, 124, 12 ));

        psICHIP->FI_ROW_12 = atof(NITFGetField(szTemp, pachTRE, 136, 12 ));
        psICHIP->FI_COL_12 = atof(NITFGetField(szTemp, pachTRE, 148, 12 ));

        psICHIP->FI_ROW_21 = atof(NITFGetField(szTemp, pachTRE, 160, 12 ));
        psICHIP->FI_COL_21 = atof(NITFGetField(szTemp, pachTRE, 172, 12 ));

        psICHIP->FI_ROW_22 = atof(NITFGetField(szTemp, pachTRE, 184, 12 ));
        psICHIP->FI_COL_22 = atof(NITFGetField(szTemp, pachTRE, 196, 12 ));

        psICHIP->FI_ROW = atoi(NITFGetField(szTemp, pachTRE, 208, 8 ));
        psICHIP->FI_COL = atoi(NITFGetField(szTemp, pachTRE, 216, 8 ));
    }
    else
    {
        fprintf( stdout, "Chip is already de-warpped?\n" );
    }

    return TRUE;
}

/************************************************************************/
/*                           NITFReadUSE00A()                           */
/*                                                                      */
/*      Read a USE00A TRE and return contents as metadata strings.      */
/************************************************************************/

char **NITFReadUSE00A( NITFImage *psImage )

{
    const char *pachTRE;
    int  nTRESize;
    char **papszMD = NULL;


/* -------------------------------------------------------------------- */
/*      Do we have the TRE?                                             */
/* -------------------------------------------------------------------- */
    pachTRE = NITFFindTRE( psImage->pachTRE, psImage->nTREBytes, 
                           "USE00A", &nTRESize );

    if( pachTRE == NULL )
        return NULL;

    if( nTRESize != 107 )
    {
        CPLError( CE_Warning, CPLE_AppDefined, 
                  "USE00A TRE wrong size, ignoring." );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Parse out field values.                                         */
/* -------------------------------------------------------------------- */
    NITFExtractMetadata( &papszMD, pachTRE,   0,   3, 
                         "NITF_USE00A_ANGLE_TO_NORTH" );
    NITFExtractMetadata( &papszMD, pachTRE,   3,   5, 
                         "NITF_USE00A_MEAN_GSD" );
    /* reserved: 1 */
    NITFExtractMetadata( &papszMD, pachTRE,   9,   5, 
                         "NITF_USE00A_DYNAMIC_RANGE" );
    /* reserved: 3+1+3 */
    NITFExtractMetadata( &papszMD, pachTRE,  21,   5, 
                         "NITF_USE00A_OBL_ANG" );
    NITFExtractMetadata( &papszMD, pachTRE,  26,   6, 
                         "NITF_USE00A_ROLL_ANG" );
    /* reserved: 12+15+4+1+3+1+1 = 37 */
    NITFExtractMetadata( &papszMD, pachTRE,  69,   2, 
                         "NITF_USE00A_N_REF" );
    NITFExtractMetadata( &papszMD, pachTRE,  71,   5, 
                         "NITF_USE00A_REV_NUM" );
    NITFExtractMetadata( &papszMD, pachTRE,  76,   3, 
                         "NITF_USE00A_N_SEG" );
    NITFExtractMetadata( &papszMD, pachTRE,  79,   6, 
                         "NITF_USE00A_MAX_LP_SEG" );
    /* reserved: 6+6 */
    NITFExtractMetadata( &papszMD, pachTRE,  97,   5, 
                         "NITF_USE00A_SUN_EL" );
    NITFExtractMetadata( &papszMD, pachTRE, 102,   5, 
                         "NITF_USE00A_SUN_AZ" );

    return papszMD;
}

/************************************************************************/
/*                           NITFReadBLOCKA()                           */
/*                                                                      */
/*      Read a BLOCKA SDE and return contents as metadata strings.      */
/************************************************************************/

char **NITFReadBLOCKA( NITFImage *psImage )

{
    const char *pachTRE;
    int  nTRESize;
    char **papszMD = NULL;
    int nBlockaCount = 0;
    char szTemp[128];

    while ( TRUE )
    {
/* -------------------------------------------------------------------- */
/*      Do we have the TRE?                                             */
/* -------------------------------------------------------------------- */
        pachTRE = NITFFindTREByIndex( psImage->pachTRE, psImage->nTREBytes, 
                                      "BLOCKA", nBlockaCount,
                                      &nTRESize );

        if( pachTRE == NULL )
            break;

        if( nTRESize != 123 )
        {
            CPLError( CE_Warning, CPLE_AppDefined, 
                      "BLOCKA TRE wrong size, ignoring." );
            break;
        }

        nBlockaCount++;

/* -------------------------------------------------------------------- */
/*      Parse out field values.                                         */
/* -------------------------------------------------------------------- */
        sprintf( szTemp, "NITF_BLOCKA_BLOCK_INSTANCE_%02d", nBlockaCount );
        NITFExtractMetadata( &papszMD, pachTRE,   0,   2, szTemp );
        sprintf( szTemp, "NITF_BLOCKA_N_GRAY_%02d", nBlockaCount );
        NITFExtractMetadata( &papszMD, pachTRE,   2,   5, szTemp );
        sprintf( szTemp, "NITF_BLOCKA_L_LINES_%02d",      nBlockaCount );
        NITFExtractMetadata( &papszMD, pachTRE,   7,   5, szTemp );
        sprintf( szTemp, "NITF_BLOCKA_LAYOVER_ANGLE_%02d",nBlockaCount );
        NITFExtractMetadata( &papszMD, pachTRE,  12,   3, szTemp );
        sprintf( szTemp, "NITF_BLOCKA_SHADOW_ANGLE_%02d", nBlockaCount );
        NITFExtractMetadata( &papszMD, pachTRE,  15,   3, szTemp );
        /* reserved: 16 */
        sprintf( szTemp, "NITF_BLOCKA_FRLC_LOC_%02d",     nBlockaCount );
        NITFExtractMetadata( &papszMD, pachTRE,  34,  21, szTemp );
        sprintf( szTemp, "NITF_BLOCKA_LRLC_LOC_%02d",     nBlockaCount );
        NITFExtractMetadata( &papszMD, pachTRE,  55,  21, szTemp );
        sprintf( szTemp, "NITF_BLOCKA_LRFC_LOC_%02d",     nBlockaCount );
        NITFExtractMetadata( &papszMD, pachTRE,  76,  21, szTemp );
        sprintf( szTemp, "NITF_BLOCKA_FRFC_LOC_%02d",     nBlockaCount );
        NITFExtractMetadata( &papszMD, pachTRE,  97,  21, szTemp );
        /* reserved: 5 -> 97 + 21 + 5 = 123 -> OK */
    }

    if ( nBlockaCount > 0 )
    {
        sprintf( szTemp, "%02d", nBlockaCount );
        papszMD = CSLSetNameValue( papszMD, "NITF_BLOCKA_BLOCK_COUNT", szTemp );
    }

    return papszMD;
}


/************************************************************************/
/*                           NITFGetGCP()                               */
/*                                                                      */
/* Reads a geographical coordinate (lat, long) from the provided        */
/* buffer.                                                              */
/************************************************************************/

void NITFGetGCP ( const char* pachCoord, GDAL_GCP *psIGEOLOGCPs, int iCoord )
{
    char szTemp[128];

    if( pachCoord[0] == 'N' || pachCoord[0] == 'n' || 
        pachCoord[0] == 'S' || pachCoord[0] == 's' )
    {	
        /* ------------------------------------------------------------ */
        /*                             0....+....1....+....2            */
        /* Coordinates are in the form Xddmmss.ssYdddmmss.ss:           */
        /* The format Xddmmss.cc represents degrees (00 to 89), minutes */
        /* (00 to 59), seconds (00 to 59), and hundredths of seconds    */
        /* (00 to 99) of latitude, with X = N for north or S for south, */
        /* and Ydddmmss.cc represents degrees (000 to 179), minutes     */
        /* (00 to 59), seconds (00 to 59), and hundredths of seconds    */
        /* (00 to 99) of longitude, with Y = E for east or W for west.  */
        /* ------------------------------------------------------------ */

        psIGEOLOGCPs[iCoord].dfGCPY = 
            atof(NITFGetField( szTemp, pachCoord, 1, 2 )) 
          + atof(NITFGetField( szTemp, pachCoord, 3, 2 )) / 60.0
          + atof(NITFGetField( szTemp, pachCoord, 5, 5 )) / 3600.0;

        if( pachCoord[0] == 's' || pachCoord[0] == 'S' )
            psIGEOLOGCPs[iCoord].dfGCPY *= -1;

        psIGEOLOGCPs[iCoord].dfGCPX = 
            atof(NITFGetField( szTemp, pachCoord,11, 3 )) 
          + atof(NITFGetField( szTemp, pachCoord,14, 2 )) / 60.0
          + atof(NITFGetField( szTemp, pachCoord,16, 5 )) / 3600.0;

        if( pachCoord[10] == 'w' || pachCoord[10] == 'W' )
            psIGEOLOGCPs[iCoord].dfGCPX *= -1;
    }
    else
    {
        /* ------------------------------------------------------------ */
        /*                             0....+....1....+....2            */
        /* Coordinates are in the form ±dd.dddddd±ddd.dddddd:           */
        /* The format ±dd.dddddd indicates degrees of latitude (north   */
        /* is positive), and ±ddd.dddddd represents degrees of          */
        /* longitude (east is positive).                                */
        /* ------------------------------------------------------------ */

        psIGEOLOGCPs[iCoord].dfGCPY = 
            atof(NITFGetField( szTemp, pachCoord, 0, 10 ));
        psIGEOLOGCPs[iCoord].dfGCPX = 
            atof(NITFGetField( szTemp, pachCoord,10, 11 ));
    }
}

/************************************************************************/
/*                           NITFReadBLOCKA_GCPs()                      */
/*                                                                      */
/* The BLOCKA repeat earth coordinates image corner locations described */
/* by IGEOLO in the NITF image subheader, but provide higher precision. */
/************************************************************************/

int NITFReadBLOCKA_GCPs( NITFImage *psImage, GDAL_GCP *psIGEOLOGCPs )
{
    const char *pachTRE;
    int        nTRESize;
    int        nBlockaLines;
    char       szTemp[128];

/* -------------------------------------------------------------------- */
/*      Do we have the TRE?                                             */
/* -------------------------------------------------------------------- */
    pachTRE = NITFFindTRE( psImage->pachTRE, psImage->nTREBytes, 
                           "BLOCKA", &nTRESize );

    if( pachTRE == NULL )
        return FALSE;

    if( nTRESize != 123 )
    {
        return FALSE;
    }

/* -------------------------------------------------------------------- */
/*      Parse out field values.                                         */
/* -------------------------------------------------------------------- */

    /* ---------------------------------------------------------------- */
    /* Make sure the BLOCKA geo coordinates are set. Spaces indicate    */
    /* the value of a coordinate is unavailable or inapplicable.        */
    /* ---------------------------------------------------------------- */
    if( pachTRE[34] == ' ' || pachTRE[55] == ' ' || 
        pachTRE[76] == ' ' || pachTRE[97] == ' ' )
    {
        return FALSE;
    }

    /* ---------------------------------------------------------------- */
    /* Extract the L_LINES field of BLOCKA and see if this instance     */
    /* covers the whole image. This is the case if L_LINES is equal to  */
    /* the no of rows of this image.                                    */
    /* We use the BLOCKA only in that case!                             */
    /* ---------------------------------------------------------------- */
    nBlockaLines = atoi(NITFGetField( szTemp, pachTRE, 7, 5 ));
    if( psImage->nRows != nBlockaLines )
    {
        return FALSE;
    }

    /* ---------------------------------------------------------------- */
    /* Note that the order of these coordinates is different from       */
    /* IGEOLO.                                                          */
    /*                   IGEOLO            BLOCKA                       */
    /* psIGEOLOGCPs[0]   0, 0              0, MaxCol                    */
    /* psIGEOLOGCPs[1]   0, MaxCol         MaxRow, MaxCol               */
    /* psIGEOLOGCPs[2]   MaxRow, MaxCol    MaxRow, 0                    */
    /* psIGEOLOGCPs[3]   MaxRow, 0         0, 0                         */
    /* ---------------------------------------------------------------- */

    NITFGetGCP ( pachTRE + 34, psIGEOLOGCPs, 1 );
    NITFGetGCP ( pachTRE + 55, psIGEOLOGCPs, 2 );
    NITFGetGCP ( pachTRE + 76, psIGEOLOGCPs, 3 );
    NITFGetGCP ( pachTRE + 97, psIGEOLOGCPs, 0 );

    /* ---------------------------------------------------------------- */
    /* Regardless of the former value of ICORDS, the values are now in  */
    /* decimal degrees.                                                 */
    /* ---------------------------------------------------------------- */

    psImage->chICORDS = 'D';

    return TRUE;
}


/************************************************************************/
/*                       NITFLoadLocationTable()                        */
/************************************************************************/

static void NITFLoadLocationTable( NITFImage *psImage )

{
#ifdef notdef
/* -------------------------------------------------------------------- */
/*      Get the location table position from within the RPFHDR TRE      */
/*      structure.                                                      */
/*                                                                      */
/*      This is the old approach, but since some RPFHDR segments are    */
/*      written wrong, it is not very stable.  See bug                  */
/*      http://bugzilla.remotesensing.org/show_bug.cgi?id=1313          */
/* -------------------------------------------------------------------- */
    GUInt32  nLocTableOffset;
    GUInt16  nLocCount;
    int      iLoc;
    const char *pszTRE;

    pszTRE= NITFFindTRE( psFile->pachTRE, psFile->nTREBytes, "RPFHDR", NULL );
    if( pszTRE == NULL )
        return;

    memcpy( &nLocTableOffset, pszTRE + 44, 4 );
    nLocTableOffset = CPL_MSBWORD32( nLocTableOffset );
    
    if( nLocTableOffset == 0 )
        return;

/* -------------------------------------------------------------------- */
/*      Read the count of entries in the location table.                */
/* -------------------------------------------------------------------- */
    VSIFSeekL( psFile->fp, nLocTableOffset + 6, SEEK_SET );
    VSIFReadL( &nLocCount, 1, 2, psFile->fp );
    nLocCount = CPL_MSBWORD16( nLocCount );
    psFile->nLocCount = nLocCount;

    psFile->pasLocations = (NITFLocation *) 
        CPLCalloc(sizeof(NITFLocation), nLocCount);
#endif

/* -------------------------------------------------------------------- */
/*      Get the location table out of the RPFIMG TRE on the image.      */
/* -------------------------------------------------------------------- */
    GUInt16  nLocCount;
    int      iLoc;
    const char *pszTRE;
    int nHeaderOffset = 0;
    int i;

    pszTRE = NITFFindTRE(psImage->pachTRE, psImage->nTREBytes, "RPFIMG", NULL);
    if( pszTRE == NULL )
        return;

    pszTRE += 6;

    memcpy( &nLocCount, pszTRE, 2 );
    nLocCount = CPL_MSBWORD16( nLocCount );

    psImage->nLocCount = nLocCount;

    psImage->pasLocations = (NITFLocation *) 
        CPLCalloc(sizeof(NITFLocation), nLocCount);

    pszTRE += 8;

    
/* -------------------------------------------------------------------- */
/*      Process the locations.                                          */
/* -------------------------------------------------------------------- */
    
    for( iLoc = 0; iLoc < nLocCount; iLoc++ )
    {
        unsigned char *pabyEntry = (unsigned char *) pszTRE;
        pszTRE += 10;

        psImage->pasLocations[iLoc].nLocId = pabyEntry[0] * 256 + pabyEntry[1];

        CPL_MSBPTR32( pabyEntry + 2 );
        memcpy( &(psImage->pasLocations[iLoc].nLocSize), pabyEntry + 2, 4 );

        CPL_MSBPTR32( pabyEntry + 6 );
        memcpy( &(psImage->pasLocations[iLoc].nLocOffset), pabyEntry + 6, 4 );
    }

/* -------------------------------------------------------------------- */
/*      It seems that sometimes (at least for bug 1313) all the         */
/*      locations in the location table are off by a fixed amount.      */
/*      We can establish that amount by checking if the RPFHDR TRE      */
/*      is at the location indicated in the location table.  If not,    */
/*      offset all locations by the difference.                         */
/* -------------------------------------------------------------------- */
    for( i = 0; i < psImage->nLocCount; i++ )
    {
        if( psImage->pasLocations[i].nLocId == LID_HeaderComponent )
        {
            nHeaderOffset = psImage->pasLocations[i].nLocOffset;
            break;
        }
    }

    if( nHeaderOffset != 0 )
    {
        char achHeaderChunk[1000];

        VSIFSeekL( psImage->psFile->fp, nHeaderOffset - 11, SEEK_SET );
        VSIFReadL( achHeaderChunk, 1, sizeof(achHeaderChunk), 
                   psImage->psFile->fp );

        if( !EQUALN(achHeaderChunk,"RPFHDR",6) )
        {
            int nOffset = 0;

            for( i = 1; i < sizeof(achHeaderChunk)-6; i++ )
            {
                if( EQUALN(achHeaderChunk+i,"RPFHDR",6) )
                {
                    nOffset = i;
                    break;
                }
            }

            if( nOffset != 0 )
            {
                CPLDebug( "NITF", 
                          "Location table offsets off by %d bytes, adjusting accordingly.",
                          nOffset );
                
                for( i = 0; i < psImage->nLocCount; i++ )
                {
                    psImage->pasLocations[i].nLocOffset += nOffset;
                }
            }
        }
    }
}

/************************************************************************/
/*                          NITFLoadVQTables()                          */
/************************************************************************/

static int NITFLoadVQTables( NITFImage *psImage )

{
    int     i, nVQOffset=0, nVQSize=0;
    GByte abyTestChunk[1000];
    GByte abySignature[6];

/* -------------------------------------------------------------------- */
/*      Do we already have the VQ tables?                               */
/* -------------------------------------------------------------------- */
    if( psImage->apanVQLUT[0] != NULL )
        return TRUE;

/* -------------------------------------------------------------------- */
/*      Do we have the location information?                            */
/* -------------------------------------------------------------------- */
    for( i = 0; i < psImage->nLocCount; i++ )
    {
        if( psImage->pasLocations[i].nLocId == LID_CompressionLookupSubsection)
        {
            nVQOffset = psImage->pasLocations[i].nLocOffset;
            nVQSize = psImage->pasLocations[i].nLocSize;
        }
    }

    if( nVQOffset == 0 )
        return FALSE;

/* -------------------------------------------------------------------- */
/*      Does it look like we have the tables properly identified?       */
/* -------------------------------------------------------------------- */
    abySignature[0] = 0x00;
    abySignature[1] = 0x00;
    abySignature[2] = 0x00;
    abySignature[3] = 0x06;
    abySignature[4] = 0x00;
    abySignature[5] = 0x0E;

    VSIFSeekL( psImage->psFile->fp, nVQOffset, SEEK_SET );
    VSIFReadL( abyTestChunk, 1, sizeof(abyTestChunk), psImage->psFile->fp );

    if( memcmp(abyTestChunk,abySignature,sizeof(abySignature)) != 0 )
    {
        for( i = 0; i < sizeof(abyTestChunk) - sizeof(abySignature); i++ )
        {
            if( memcmp(abyTestChunk+i,abySignature,sizeof(abySignature)) == 0 )
            {
                nVQOffset += i; 
                CPLDebug( "NITF", 
                          "VQ CompressionLookupSubsection offsets off by %d bytes, adjusting accordingly.",
                          i );
                break;
            }
        }
    }
    
/* -------------------------------------------------------------------- */
/*      Load the tables.                                                */
/* -------------------------------------------------------------------- */
    for( i = 0; i < 4; i++ )
    {
        GUInt32 nVQVector;

        psImage->apanVQLUT[i] = (GUInt32 *) CPLCalloc(4096,sizeof(GUInt32));

        VSIFSeekL( psImage->psFile->fp, nVQOffset + 6 + i*14 + 10, SEEK_SET );
        VSIFReadL( &nVQVector, 1, 4, psImage->psFile->fp );
        nVQVector = CPL_MSBWORD32( nVQVector );
        
        VSIFSeekL( psImage->psFile->fp, nVQOffset + nVQVector, SEEK_SET );
        VSIFReadL( psImage->apanVQLUT[i], 4, 4096, psImage->psFile->fp );
    }

    return TRUE;
}

/************************************************************************/
/*                           NITFReadSTDIDC()                           */
/*                                                                      */
/*      Read a STDIDC TRE and return contents as metadata strings.      */
/************************************************************************/

char **NITFReadSTDIDC( NITFImage *psImage )

{
    const char *pachTRE;
    int  nTRESize;
    char **papszMD = NULL;

/* -------------------------------------------------------------------- */
/*      Do we have the TRE?                                             */
/* -------------------------------------------------------------------- */
    pachTRE = NITFFindTRE( psImage->pachTRE, psImage->nTREBytes, 
                           "STDIDC", &nTRESize );

    if( pachTRE == NULL )
        return NULL;

    if( nTRESize != 89 )
    {
        CPLError( CE_Warning, CPLE_AppDefined, 
                  "STDIDC TRE wrong size, ignoring." );
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Parse out field values.                                         */
/* -------------------------------------------------------------------- */
    NITFExtractMetadata( &papszMD, pachTRE,   0,  14, 
                         "NITF_STDIDC_ACQUISITION_DATE" );
    NITFExtractMetadata( &papszMD, pachTRE,  14,  14, 
                         "NITF_STDIDC_MISSION" );
    NITFExtractMetadata( &papszMD, pachTRE,  28,   2, 
                         "NITF_STDIDC_PASS" );
    NITFExtractMetadata( &papszMD, pachTRE,  30,   3, 
                         "NITF_STDIDC_OP_NUM" );
    NITFExtractMetadata( &papszMD, pachTRE,  33,   2, 
                         "NITF_STDIDC_START_SEGMENT" );
    NITFExtractMetadata( &papszMD, pachTRE,  35,   2, 
                         "NITF_STDIDC_REPRO_NUM" );
    NITFExtractMetadata( &papszMD, pachTRE,  37,   3, 
                         "NITF_STDIDC_REPLAY_REGEN" );
    /* reserved: 1 */
    NITFExtractMetadata( &papszMD, pachTRE,  41,   3, 
                         "NITF_STDIDC_START_COLUMN" );
    NITFExtractMetadata( &papszMD, pachTRE,  44,   5, 
                         "NITF_STDIDC_START_ROW" );
    NITFExtractMetadata( &papszMD, pachTRE,  49,   2, 
                         "NITF_STDIDC_END_SEGMENT" );
    NITFExtractMetadata( &papszMD, pachTRE,  51,   3, 
                         "NITF_STDIDC_END_COLUMN" );
    NITFExtractMetadata( &papszMD, pachTRE,  54,   5, 
                         "NITF_STDIDC_END_ROW" );
    NITFExtractMetadata( &papszMD, pachTRE,  59,   2, 
                         "NITF_STDIDC_COUNTRY" );
    NITFExtractMetadata( &papszMD, pachTRE,  61,   4, 
                         "NITF_STDIDC_WAC" );
    NITFExtractMetadata( &papszMD, pachTRE,  65,  11, 
                         "NITF_STDIDC_LOCATION" );
    /* reserved: 5+8 */

    return papszMD;
}

/************************************************************************/
/*                         NITFRPCGeoToImage()                          */
/************************************************************************/

int NITFRPCGeoToImage( NITFRPC00BInfo *psRPC, 
                       double dfLong, double dfLat, double dfHeight, 
                       double *pdfPixel, double *pdfLine )

{
    double dfLineNumerator, dfLineDenominator, 
        dfPixelNumerator, dfPixelDenominator;
    double dfPolyTerm[20];
    int i;

/* -------------------------------------------------------------------- */
/*      Normalize Lat/Long position.                                    */
/* -------------------------------------------------------------------- */
    dfLong = (dfLong - psRPC->LONG_OFF) / psRPC->LONG_SCALE;
    dfLat  = (dfLat - psRPC->LAT_OFF) / psRPC->LAT_SCALE;
    dfHeight = (dfHeight - psRPC->HEIGHT_OFF) / psRPC->HEIGHT_SCALE;

/* -------------------------------------------------------------------- */
/*      Compute the 20 terms.                                           */
/* -------------------------------------------------------------------- */

    dfPolyTerm[0] = 1.0;
    dfPolyTerm[1] = dfLong;
    dfPolyTerm[2] = dfLat;
    dfPolyTerm[3] = dfHeight;
    dfPolyTerm[4] = dfLong * dfLat;
    dfPolyTerm[5] = dfLong * dfHeight;
    dfPolyTerm[6] = dfLat * dfHeight;
    dfPolyTerm[7] = dfLong * dfLong;
    dfPolyTerm[8] = dfLat * dfLat;
    dfPolyTerm[9] = dfHeight * dfHeight;

    dfPolyTerm[10] = dfLong * dfLat * dfHeight;
    dfPolyTerm[11] = dfLong * dfLong * dfLong;
    dfPolyTerm[12] = dfLong * dfLat * dfLat;
    dfPolyTerm[13] = dfLong * dfHeight * dfHeight;
    dfPolyTerm[14] = dfLong * dfLong * dfLat;
    dfPolyTerm[15] = dfLat * dfLat * dfLat;
    dfPolyTerm[16] = dfLat * dfHeight * dfHeight;
    dfPolyTerm[17] = dfLong * dfLong * dfHeight;
    dfPolyTerm[18] = dfLat * dfLat * dfHeight;
    dfPolyTerm[19] = dfHeight * dfHeight * dfHeight;
    

/* -------------------------------------------------------------------- */
/*      Compute numerator and denominator sums.                         */
/* -------------------------------------------------------------------- */
    dfPixelNumerator = 0.0;
    dfPixelDenominator = 0.0;
    dfLineNumerator = 0.0;
    dfLineDenominator = 0.0;

    for( i = 0; i < 20; i++ )
    {
        dfPixelNumerator += psRPC->SAMP_NUM_COEFF[i] * dfPolyTerm[i];
        dfPixelDenominator += psRPC->SAMP_DEN_COEFF[i] * dfPolyTerm[i];
        dfLineNumerator += psRPC->LINE_NUM_COEFF[i] * dfPolyTerm[i];
        dfLineDenominator += psRPC->LINE_DEN_COEFF[i] * dfPolyTerm[i];
    }
        
/* -------------------------------------------------------------------- */
/*      Compute normalized pixel and line values.                       */
/* -------------------------------------------------------------------- */
    *pdfPixel = dfPixelNumerator / dfPixelDenominator;
    *pdfLine = dfLineNumerator / dfLineDenominator;

/* -------------------------------------------------------------------- */
/*      Denormalize.                                                    */
/* -------------------------------------------------------------------- */
    *pdfPixel = *pdfPixel * psRPC->SAMP_SCALE + psRPC->SAMP_OFF;
    *pdfLine  = *pdfLine  * psRPC->LINE_SCALE + psRPC->LINE_OFF;

    return TRUE;
}

/************************************************************************/
/*                         NITFIHFieldOffset()                          */
/*                                                                      */
/*      Find the file offset for the beginning of a particular field    */
/*      in this image header.  Only implemented for selected fields.    */
/************************************************************************/

GUInt32 NITFIHFieldOffset( NITFImage *psImage, const char *pszFieldName )

{
    char szTemp[128];
    int nNICOM;
    GUInt32 nWrkOffset;
    GUInt32 nIMOffset =
        psImage->psFile->pasSegmentInfo[psImage->iSegment].nSegmentHeaderStart;

    // We only support files we created.
    CPLAssert( EQUALN(psImage->psFile->szVersion,"NITF02.1",8) );

    if( EQUAL(pszFieldName,"IM") )
        return nIMOffset;

    if( EQUAL(pszFieldName,"PJUST") )
        return nIMOffset + 370;

    if( EQUAL(pszFieldName,"ICORDS") )
        return nIMOffset + 371;

    if( EQUAL(pszFieldName,"IGEOLO") )
    {
        if( !psImage->bHaveIGEOLO )
            return 0;
        else
            return nIMOffset + 372;
    }

/* -------------------------------------------------------------------- */
/*      Keep working offset from here on in since everything else is    */
/*      variable.                                                       */
/* -------------------------------------------------------------------- */
    nWrkOffset = 372 + nIMOffset;

    if( psImage->bHaveIGEOLO )
        nWrkOffset += 60;

/* -------------------------------------------------------------------- */
/*      Comments.                                                       */
/* -------------------------------------------------------------------- */
    nNICOM = atoi(NITFGetField(szTemp,psImage->pachHeader,
                               nWrkOffset - nIMOffset,1));
        
    if( EQUAL(pszFieldName,"NICOM") )
        return nWrkOffset;
    
    nWrkOffset++;

    if( EQUAL(pszFieldName,"ICOM") )
        return nWrkOffset;

    nWrkOffset += 80 * nNICOM;

/* -------------------------------------------------------------------- */
/*      IC                                                              */
/* -------------------------------------------------------------------- */
    if( EQUAL(pszFieldName,"IC") )
        return nWrkOffset;

    nWrkOffset += 2;

/* -------------------------------------------------------------------- */
/*      COMRAT                                                          */
/* -------------------------------------------------------------------- */

    if( psImage->szIC[0] != 'N' )
    {
        if( EQUAL(pszFieldName,"COMRAT") )
            return nWrkOffset;
        nWrkOffset += 4;
    }

/* -------------------------------------------------------------------- */
/*      NBANDS                                                          */
/* -------------------------------------------------------------------- */
    if( EQUAL(pszFieldName,"NBANDS") )
        return nWrkOffset;

    nWrkOffset += 1;

/* -------------------------------------------------------------------- */
/*      XBANDS                                                          */
/* -------------------------------------------------------------------- */
    if( EQUAL(pszFieldName,"XBANDS") )
        return nWrkOffset;

    if( psImage->nBands > 9 )
        nWrkOffset += 5;

/* -------------------------------------------------------------------- */
/*      IREPBAND                                                        */
/* -------------------------------------------------------------------- */
    if( EQUAL(pszFieldName,"IREPBAND") )
        return nWrkOffset;

//    nWrkOffset += 2 * psImage->nBands;

    return 0;
}
