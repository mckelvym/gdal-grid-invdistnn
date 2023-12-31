/**********************************************************************
 * $Id: avc_e00parse.c,v 1.11 2001/11/25 21:15:23 daniel Exp $
 *
 * Name:     avc_e00parse.c
 * Project:  Arc/Info vector coverage (AVC)  E00->BIN conversion library
 * Language: ANSI C
 * Purpose:  Functions to parse ASCII E00 lines and fill binary structures.
 * Author:   Daniel Morissette, danmo@videotron.ca
 *
 **********************************************************************
 * Copyright (c) 1999, 2000, Daniel Morissette
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 **********************************************************************
 *
 * $Log: avc_e00parse.c,v $
 * Revision 1.11  2001/11/25 21:15:23  daniel
 * Added hack (AVC_MAP_TYPE40_TO_DOUBLE) to map type 40 fields bigger than 8
 * digits to double precision as we generate E00 output (bug599)
 *
 * Revision 1.10  2001/11/25 19:45:32  daniel
 * Fixed reading of type 40 when not in exponent format (bug599)
 *
 * Revision 1.9  2001/07/12 20:59:34  daniel
 * Properly handle PAL entries with 0 arcs
 *
 * Revision 1.8  2000/09/22 19:45:20  daniel
 * Switch to MIT-style license
 *
 * Revision 1.7  2000/03/16 03:48:00  daniel
 * Accept 0-length text strings in TX6/TX7 objects
 *
 * Revision 1.6  2000/02/03 07:21:40  daniel
 * TXT/TX6 with string longer than 80 chars: split string in 80 chars chunks
 *
 * Revision 1.5  1999/12/05 03:40:13  daniel
 * Fixed signed/unsigned mismatch compile warning
 *
 * Revision 1.4  1999/11/23 05:27:58  daniel
 * Added AVCE00Str2Int() to extract integer values in E00 lines
 *
 * Revision 1.3  1999/08/23 18:20:49  daniel
 * Fixed support for attribute fields type 40
 *
 * Revision 1.2  1999/05/17 16:20:48  daniel
 * Added RXP + TXT/TX6/TX7 write support + some simple problems fixed
 *
 * Revision 1.1  1999/05/11 02:34:46  daniel
 * Initial revision
 *
 **********************************************************************/

#include "avc.h"

#include <ctype.h>      /* toupper() */


/**********************************************************************
 *                          AVCE00Str2Int()
 *
 * Convert a portion of a string to an integer value.
 * The difference between this function and atoi() is that this version
 * takes only the specified number of characters... so it can handle the 
 * case of 2 numbers that are part of the same string but are not separated 
 * by a space.
 **********************************************************************/
int    AVCE00Str2Int(const char *pszStr, int numChars)
{
    int nValue = 0;

    if (pszStr && numChars >= (int)strlen(pszStr))
        return atoi(pszStr);
    else if (pszStr)
    {
        char cNextDigit;
        char *pszTmp;

        /* Get rid of const */
        pszTmp = (char*)pszStr;

        cNextDigit = pszTmp[numChars];
        pszTmp[numChars] = '\0';
        nValue = atoi(pszTmp);
        pszTmp[numChars] = cNextDigit;
    }

    return nValue;
}

/**********************************************************************
 *                          AVCE00ParseInfoAlloc()
 *
 * Allocate and initialize a new AVCE00ParseInfo structure.
 *
 * AVCE00ParseStartSection() will have to be called at least once
 * to specify the type of objects to parse.
 *
 * The structure will eventually have to be freed with AVCE00ParseInfoFree().
 **********************************************************************/
AVCE00ParseInfo  *AVCE00ParseInfoAlloc()
{
    AVCE00ParseInfo       *psInfo;

    psInfo = (AVCE00ParseInfo*)CPLCalloc(1,sizeof(AVCE00ParseInfo));

    psInfo->eFileType = AVCFileUnknown;
    psInfo->eSuperSectionType = AVCFileUnknown;

    /* Allocate output buffer.  
     * 2k should be enough... the biggest thing we'll need to store
     * in it will be 1 complete INFO table record.
     */
    psInfo->nBufSize = 2048;
    psInfo->pszBuf = (char *)CPLMalloc(psInfo->nBufSize*sizeof(char));

    /* Set a default precision, but this value will be set on a section
     * by section basis inside AVCE00ParseStartSection()
     */
    psInfo->nPrecision = AVC_SINGLE_PREC;

    return psInfo;
}

/**********************************************************************
 *                         _AVCE00ParseDestroyCurObject()
 *
 * Release mem. associated with the psInfo->cur.* object we are
 * currently using.
 **********************************************************************/
void    _AVCE00ParseDestroyCurObject(AVCE00ParseInfo  *psInfo)
{
    if (psInfo->eFileType == AVCFileUnknown)
        return;

    if (psInfo->eFileType == AVCFileARC)
    {
        CPLFree(psInfo->cur.psArc->pasVertices);
        CPLFree(psInfo->cur.psArc);
    }
    else if (psInfo->eFileType == AVCFilePAL ||
             psInfo->eFileType == AVCFileRPL )
    {
        CPLFree(psInfo->cur.psPal->pasArcs);
        CPLFree(psInfo->cur.psPal);
    }
    else if (psInfo->eFileType == AVCFileCNT)
    {
        CPLFree(psInfo->cur.psCnt->panLabelIds);
        CPLFree(psInfo->cur.psCnt);
    }
    else if (psInfo->eFileType == AVCFileLAB)
    {
        CPLFree(psInfo->cur.psLab);
    }
    else if (psInfo->eFileType == AVCFileTOL)
    {
        CPLFree(psInfo->cur.psTol);
    }
    else if (psInfo->eFileType == AVCFilePRJ)
    {
        CSLDestroy(psInfo->cur.papszPrj);
    }
    else if (psInfo->eFileType == AVCFileTXT || 
             psInfo->eFileType == AVCFileTX6)
    {
        CPLFree(psInfo->cur.psTxt->pasVertices);
        CPLFree(psInfo->cur.psTxt->pszText);
        CPLFree(psInfo->cur.psTxt);
    }
    else if (psInfo->eFileType == AVCFileRXP)
    {
        CPLFree(psInfo->cur.psRxp);
    }
    else if (psInfo->eFileType == AVCFileTABLE)
    {
        _AVCDestroyTableFields(psInfo->hdr.psTableDef, psInfo->cur.pasFields);
        _AVCDestroyTableDef(psInfo->hdr.psTableDef);
        psInfo->bTableHdrComplete = FALSE;
    }
    else
    {
        CPLError(CE_Failure, CPLE_NotSupported,
                 "_AVCE00ParseDestroyCurObject(): Unsupported file type!");
    }

    psInfo->eFileType = AVCFileUnknown;
    psInfo->cur.psArc = NULL;
}

/**********************************************************************
 *                          AVCE00ParseInfoFree()
 *
 * Free any memory associated with a AVCE00ParseInfo structure.
 **********************************************************************/
void    AVCE00ParseInfoFree(AVCE00ParseInfo  *psInfo)
{
    if (psInfo)
    {
        CPLFree(psInfo->pszSectionHdrLine);
        psInfo->pszSectionHdrLine = NULL;
        CPLFree(psInfo->pszBuf);
        _AVCE00ParseDestroyCurObject(psInfo);
    }

    CPLFree(psInfo);
}

/**********************************************************************
 *                          AVCE00ParseReset()
 *
 * Reset the fields in a AVCE00ParseInfo structure so that further calls
 * to the API will be ready to process a new object.
 **********************************************************************/
void    AVCE00ParseReset(AVCE00ParseInfo  *psInfo)
{
    psInfo->iCurItem = psInfo->numItems = 0;
    psInfo->bForceEndOfSection = FALSE;
}


/**********************************************************************
 *                          AVCE00ParseSuperSectionHeader()
 *
 * Check if pszLine is a valid "supersection" header line, if it is one 
 * then store the supersection type in the ParseInfo structure.
 *
 * What I call a "supersection" is a section that contains several
 * files, such as the TX6/TX7, RPL, RXP, ... and also the IFO (TABLEs).
 *
 * The ParseInfo structure won't be ready to read objects until
 * a call to AVCE00ParseSectionHeader() (see below) succesfully
 * recognizes the beginning of a subsection of this type.
 *
 * Returns the new supersection type, or AVCFileUnknown if the line is
 * not recognized.
 **********************************************************************/
AVCFileType  AVCE00ParseSuperSectionHeader(AVCE00ParseInfo  *psInfo,
                                           const char *pszLine)
{
    /*-----------------------------------------------------------------
     * If we're already inside a supersection or a section, then
     * return AVCFileUnknown right away.
     *----------------------------------------------------------------*/
    if (psInfo == NULL ||
        psInfo->eSuperSectionType != AVCFileUnknown ||
        psInfo->eFileType != AVCFileUnknown )
    {
        return AVCFileUnknown;
    }

    /*-----------------------------------------------------------------
     * Check if pszLine is a valid supersection header line.
     *----------------------------------------------------------------*/
    if (EQUALN(pszLine, "RPL  ", 5))
        psInfo->eSuperSectionType = AVCFileRPL;
    else if (EQUALN(pszLine, "TX6  ", 5) || EQUALN(pszLine, "TX7  ", 5))
        psInfo->eSuperSectionType = AVCFileTX6;
    else if (EQUALN(pszLine, "RXP  ", 5))
        psInfo->eSuperSectionType = AVCFileRXP;
    else if (EQUALN(pszLine, "IFO  ", 5))
        psInfo->eSuperSectionType = AVCFileTABLE;
    else
        return AVCFileUnknown;

    /*-----------------------------------------------------------------
     * OK, we have a valid new section header. Set the precision and 
     * get ready to read objects from it.
     *----------------------------------------------------------------*/
    if (atoi(pszLine+4) == 2)
        psInfo->nPrecision = AVC_SINGLE_PREC;
    else if (atoi(pszLine+4) == 3)
        psInfo->nPrecision = AVC_DOUBLE_PREC;
    else
    {
        CPLError(CE_Failure, CPLE_AppDefined, 
                 "Parse Error: Invalid section header line (\"%s\")!", 
                 pszLine);
        psInfo->eSuperSectionType = AVCFileUnknown;
    }

    return psInfo->eSuperSectionType;
}

/**********************************************************************
 *                          AVCE00ParseSuperSectionEnd()
 *
 * Check if pszLine marks the end of a supersection, and if it is the
 * case, then reset the supersection flag in the ParseInfo.
 * 
 * Supersections always end with the line "JABBERWOCKY", except for
 * the IFO section.
 **********************************************************************/
GBool  AVCE00ParseSuperSectionEnd(AVCE00ParseInfo  *psInfo,
                                  const char *pszLine )
{
    if (psInfo->eFileType == AVCFileUnknown &&
        psInfo->eSuperSectionType != AVCFileUnknown &&
        (EQUALN(pszLine, "JABBERWOCKY", 11) ||
         (psInfo->eSuperSectionType == AVCFileTABLE && 
          EQUALN(pszLine, "EOI", 3) )  ) )
    {
        psInfo->eSuperSectionType = AVCFileUnknown;
        return TRUE;
    }

    return FALSE;
}


/**********************************************************************
 *                          AVCE00ParseSectionHeader()
 *
 * Check if pszLine is a valid section header line, then initialize the
 * ParseInfo structure to be ready to parse of object from that section.
 *
 * Returns the new section type, or AVCFileUnknown if the line is
 * not recognized as a valid section header.
 *
 * Note: by section header lines, we mean the "ARC  2", "PAL  2", etc.
 **********************************************************************/
AVCFileType  AVCE00ParseSectionHeader(AVCE00ParseInfo  *psInfo,
                                      const char *pszLine)
{
    AVCFileType  eNewType = AVCFileUnknown;

    if (psInfo == NULL ||
        psInfo->eFileType != AVCFileUnknown)
    {
        return AVCFileUnknown;
    }

    /*-----------------------------------------------------------------
     * Check if pszLine is a valid section header line.
     *----------------------------------------------------------------*/
    if (psInfo->eSuperSectionType == AVCFileUnknown)
    {
        /*-------------------------------------------------------------
         * We're looking for a top-level section...
         *------------------------------------------------------------*/
        if (EQUALN(pszLine, "ARC  ", 5))
            eNewType = AVCFileARC;
        else if (EQUALN(pszLine, "PAL  ", 5))
            eNewType = AVCFilePAL;
        else if (EQUALN(pszLine, "CNT  ", 5))
            eNewType = AVCFileCNT;
        else if (EQUALN(pszLine, "LAB  ", 5))
            eNewType = AVCFileLAB;
        else if (EQUALN(pszLine, "TOL  ", 5))
            eNewType = AVCFileTOL;
        else if (EQUALN(pszLine, "PRJ  ", 5))
            eNewType = AVCFilePRJ;
        else if (EQUALN(pszLine, "TXT  ", 5))
            eNewType = AVCFileTXT;
        else
        {
            eNewType = AVCFileUnknown;
            return AVCFileUnknown;
        }

        /*-------------------------------------------------------------
         * OK, we have a valid new section header. Set the precision and 
         * get ready to read objects from it.
         *------------------------------------------------------------*/
        if (atoi(pszLine+4) == 2)
            psInfo->nPrecision = AVC_SINGLE_PREC;
        else if (atoi(pszLine+4) == 3)
            psInfo->nPrecision = AVC_DOUBLE_PREC;
        else
        {
            CPLError(CE_Failure, CPLE_AppDefined, 
                     "Parse Error: Invalid section header line (\"%s\")!", 
                     pszLine);
            eNewType = AVCFileUnknown;
            return AVCFileUnknown;
        }

    }
    else
    {
        /*-------------------------------------------------------------
         * We're looking for a section inside a super-section...
         * in this case, the header line contains the subclass name,
         * so any non-empty line is acceptable!
         * Note: the precision is already set from the previous call to
         *       AVCE00ParseSuperSectionHeader()
         * Note2: Inside a double precision RPL supersection, the end of
         *        each sub-section is marked by 2 lines, just like what
         *        happens with double precision PALs... we have to make
         *        sure we don't catch that second line as the beginning
         *        of a new RPL sub-section.
         *------------------------------------------------------------*/
        if (strlen(pszLine) > 0 && !isspace(pszLine[0]) && 
            !EQUALN(pszLine, "JABBERWOCKY", 11) &&
            !EQUALN(pszLine, "EOI", 3) &&
            ! ( psInfo->eSuperSectionType == AVCFileRPL &&
                EQUALN(pszLine, " 0.00000", 6)  ) )
        {
            eNewType = psInfo->eSuperSectionType;
        }
        else
        {
            eNewType = AVCFileUnknown;
            return AVCFileUnknown;
        }
    }

    /*-----------------------------------------------------------------
     * nCurObjectId is used to keep track of sequential ids that are 
     * not explicitly stored in E00.  e.g. polygon Id in a PAL section.
     *----------------------------------------------------------------*/
    psInfo->nCurObjectId = 0;

    /*-----------------------------------------------------------------
     * Allocate a temp. structure to use to store the objects we read
     * (Using Calloc() will automatically initialize the struct contents
     *  to NULL... this is very important for ARCs and PALs)
     *----------------------------------------------------------------*/
    _AVCE00ParseDestroyCurObject(psInfo);

    if (eNewType == AVCFileARC)
    {
        psInfo->cur.psArc = (AVCArc*)CPLCalloc(1, sizeof(AVCArc));
    }
    else if (eNewType == AVCFilePAL ||
             eNewType == AVCFileRPL )
    {
        psInfo->cur.psPal = (AVCPal*)CPLCalloc(1, sizeof(AVCPal));
    }
    else if (eNewType == AVCFileCNT)
    {
        psInfo->cur.psCnt = (AVCCnt*)CPLCalloc(1, sizeof(AVCCnt));
    }
    else if (eNewType == AVCFileLAB)
    {
        psInfo->cur.psLab = (AVCLab*)CPLCalloc(1, sizeof(AVCLab));
    }
    else if (eNewType == AVCFileTOL)
    {
        psInfo->cur.psTol = (AVCTol*)CPLCalloc(1, sizeof(AVCTol));
    }
    else if (eNewType == AVCFilePRJ)
    {
        psInfo->cur.psTol = (AVCTol*)CPLCalloc(1, sizeof(AVCTol));
    }
    else if (eNewType == AVCFilePRJ)
    {
        psInfo->cur.papszPrj = NULL;
    }
    else if (eNewType == AVCFileTXT ||
             eNewType == AVCFileTX6)
    {
        psInfo->cur.psTxt = (AVCTxt*)CPLCalloc(1, sizeof(AVCTxt));
    }
    else if (eNewType == AVCFileRXP)
    {
        psInfo->cur.psRxp = (AVCRxp*)CPLCalloc(1, sizeof(AVCRxp));
    }
    else if (eNewType == AVCFileTABLE)
    {
        psInfo->cur.pasFields = NULL;
        psInfo->hdr.psTableDef = NULL;
        psInfo->bTableHdrComplete = FALSE;
    }
    else
    {
        CPLError(CE_Failure, CPLE_NotSupported,
                 "AVCE00ParseSectionHeader(): Unsupported file type!");
        eNewType = AVCFileUnknown;
    }

    /*-----------------------------------------------------------------
     * Keep track of section header line... this is used for some file
     * types, specially the ones enclosed inside supersections.
     *----------------------------------------------------------------*/
    CPLFree(psInfo->pszSectionHdrLine);
    psInfo->pszSectionHdrLine = CPLStrdup(pszLine);

    psInfo->eFileType = eNewType;

    return psInfo->eFileType;
}


/**********************************************************************
 *                          AVCE00ParseSectionEnd()
 *
 * Check if pszLine marks the end of the current section.  
 * 
 * Passing bResetParseInfo=TRUE will reset the parser struct if an end of
 * section is found.  Passing FALSE simply tests for the end of section 
 * without affecting the parse info struct.
 *
 * Return TRUE if this is the end of the section (and reset the
 * ParseInfo structure) , or FALSE otherwise.
 **********************************************************************/
GBool  AVCE00ParseSectionEnd(AVCE00ParseInfo  *psInfo, const char *pszLine,
                             GBool bResetParseInfo)
{
    if ( psInfo->bForceEndOfSection ||
         ((psInfo->eFileType == AVCFileARC ||
           psInfo->eFileType == AVCFilePAL ||
           psInfo->eFileType == AVCFileLAB ||
           psInfo->eFileType == AVCFileRPL ||
           psInfo->eFileType == AVCFileCNT ||
           psInfo->eFileType == AVCFileTOL ||
           psInfo->eFileType == AVCFileTXT ||
           psInfo->eFileType == AVCFileTX6 ||
           psInfo->eFileType == AVCFileRXP )  && 
          EQUALN(pszLine, "        -1         0", 20)  ) ||
         ( psInfo->eFileType == AVCFilePRJ && EQUALN(pszLine, "EOP", 3) ) )
    {
        /* Reset ParseInfo only if explicitly requested. 
         */
        if (bResetParseInfo)
        {
            _AVCE00ParseDestroyCurObject(psInfo);
            AVCE00ParseReset(psInfo);
            psInfo->eFileType = AVCFileUnknown;

            CPLFree(psInfo->pszSectionHdrLine);
            psInfo->pszSectionHdrLine = NULL;

            psInfo->bForceEndOfSection = FALSE;
        }

        return TRUE;  /* YES, we reached the end */
    }

    return FALSE;  /* NO, it's not the end of section line */
}

/**********************************************************************
 *                          AVCE00ParseNextLine()
 *
 * Take the next line of E00 input and parse it.
 *
 * Returns NULL if the current object is not complete yet (expecting
 * more lines of input) or a reference to a complete object if it
 * is complete.
 *
 * The returned object is a reference to an internal data structure.
 * It should not be modified or freed by the caller.
 *
 * If the input is invalid or other problems happen, then a CPLError()
 * will be generated.  CPLGetLastErrorNo() should be called to check
 * that the line was parsed succesfully.
 *
 * Note for TABLES:
 * When parsing input from info tables, the first valid object that
 * will be returned will be the AVCTableDef, and then the data records
 * will follow.  When all the records have been read, then the
 * psInfo->bForceEndOfSection flag will be set to TRUE since there is
 * no explicit "end of table" line in E00.
 **********************************************************************/
void   *AVCE00ParseNextLine(AVCE00ParseInfo  *psInfo, const char *pszLine)
{
    void *psObj = NULL;

    CPLAssert(psInfo);
    switch(psInfo->eFileType)
    {
      case AVCFileARC:
        psObj = (void*)AVCE00ParseNextArcLine(psInfo, pszLine);
        break;
      case AVCFilePAL:
      case AVCFileRPL:
        psObj = (void*)AVCE00ParseNextPalLine(psInfo, pszLine);
        break;
      case AVCFileCNT:
        psObj = (void*)AVCE00ParseNextCntLine(psInfo, pszLine);
        break;
      case AVCFileLAB:
        psObj = (void*)AVCE00ParseNextLabLine(psInfo, pszLine);
        break;
      case AVCFileTOL:
        psObj = (void*)AVCE00ParseNextTolLine(psInfo, pszLine);
        break;
      case AVCFilePRJ:
        psObj = (void*)AVCE00ParseNextPrjLine(psInfo, pszLine);
        break;
      case AVCFileTXT:
        psObj = (void*)AVCE00ParseNextTxtLine(psInfo, pszLine);
        break;
      case AVCFileTX6:
        psObj = (void*)AVCE00ParseNextTx6Line(psInfo, pszLine);
        break;
      case AVCFileRXP:
        psObj = (void*)AVCE00ParseNextRxpLine(psInfo, pszLine);
        break;
      case AVCFileTABLE:
        if ( ! psInfo->bTableHdrComplete )
            psObj = (void*)AVCE00ParseNextTableDefLine(psInfo, pszLine);
        else
            psObj = (void*)AVCE00ParseNextTableRecLine(psInfo, pszLine);
        break;
      default:
        CPLError(CE_Failure, CPLE_NotSupported,
                 "AVCE00ParseNextLine(): Unsupported file type!");
    }

    return psObj;
}


/**********************************************************************
 *                          AVCE00ParseNextArcLine()
 *
 * Take the next line of E00 input for an ARC object and parse it.
 *
 * Returns NULL if the current object is not complete yet (expecting
 * more lines of input) or a reference to a complete object if it
 * is complete.
 *
 * The returned object is a reference to an internal data structure.
 * It should not be modified or freed by the caller.
 *
 * If the input is invalid or other problems happen, then a CPLError()
 * will be generated.  CPLGetLastErrorNo() should be called to check
 * that the line was parsed succesfully.
 **********************************************************************/
AVCArc   *AVCE00ParseNextArcLine(AVCE00ParseInfo *psInfo, const char *pszLine)
{
    AVCArc *psArc;
    int     nLen;

    CPLAssert(psInfo->eFileType == AVCFileARC);

    psArc = psInfo->cur.psArc;

    nLen = strlen(pszLine);

    if (psInfo->numItems == 0)
    {
        /*-------------------------------------------------------------
         * Begin processing a new object, read header line:
         *    ArcId, UserId, FNode, TNode, LPoly, RPoly, numVertices
         *------------------------------------------------------------*/
        if (nLen < 70)
        {
            CPLError(CE_Failure, CPLE_AppDefined, 
                     "Error parsing E00 ARC line: \"%s\"", pszLine);
            return NULL;
        }
        else
        {
            psArc->nArcId = AVCE00Str2Int(pszLine, 10);
            psArc->nUserId = AVCE00Str2Int(pszLine+10, 10);
            psArc->nFNode = AVCE00Str2Int(pszLine+20, 10);
            psArc->nTNode = AVCE00Str2Int(pszLine+30, 10);
            psArc->nLPoly = AVCE00Str2Int(pszLine+40, 10);
            psArc->nRPoly = AVCE00Str2Int(pszLine+50, 10);
            psArc->numVertices = AVCE00Str2Int(pszLine+60, 10);
            
            /* Realloc the array of vertices 
             */
            psArc->pasVertices = (AVCVertex*)CPLRealloc(psArc->pasVertices,
                                                        psArc->numVertices*
                                                        sizeof(AVCVertex));

            /* psInfo->iCurItem is the last vertex that was read.
             * psInfo->numItems is the number of vertices to read.
             */
            psInfo->iCurItem = 0;
            psInfo->numItems = psArc->numVertices;
        }
    }
    else if (psInfo->iCurItem < psInfo->numItems && 
             psInfo->nPrecision == AVC_SINGLE_PREC &&
             ( (psInfo->iCurItem==psInfo->numItems-1 && nLen >= 28) ||
               nLen >= 56 )  )
    {
        /*-------------------------------------------------------------
         * Single precision ARCs: 2 pairs of X,Y values per line
         * Except on the last line with an odd number of vertices)
         *------------------------------------------------------------*/
        psArc->pasVertices[psInfo->iCurItem].x = atof(pszLine);
        psArc->pasVertices[psInfo->iCurItem++].y = atof(pszLine+14);
        if (psInfo->iCurItem < psInfo->numItems && nLen >= 56)
        {
            psArc->pasVertices[psInfo->iCurItem].x = atof(pszLine+28);
            psArc->pasVertices[psInfo->iCurItem++].y = atof(pszLine+42);
        }
    }
    else if (psInfo->iCurItem < psInfo->numItems && 
             psInfo->nPrecision == AVC_DOUBLE_PREC &&
             nLen >= 42)
    {
        /*-------------------------------------------------------------
         * Double precision ARCs: 1 pair of X,Y values per line
         *------------------------------------------------------------*/
        psArc->pasVertices[psInfo->iCurItem].x = atof(pszLine);
        psArc->pasVertices[psInfo->iCurItem++].y = atof(pszLine+21);
    }
    else
    {
        CPLError(CE_Failure, CPLE_AppDefined, 
                 "Error parsing E00 ARC line: \"%s\"", pszLine);
        psInfo->numItems = psInfo->iCurItem = 0;
        return NULL;
    }

    /*-----------------------------------------------------------------
     * If we're done parsing this ARC, then reset the ParseInfo,
     * and return a reference to the ARC structure
     * Otherwise return NULL, which means that we are expecting more
     * more lines of input.
     *----------------------------------------------------------------*/
    if (psInfo->iCurItem >= psInfo->numItems)
    {
        psInfo->numItems = psInfo->iCurItem = 0;
        return psArc;
    }

    return NULL;
}

/**********************************************************************
 *                          AVCE00ParseNextPalLine()
 *
 * Take the next line of E00 input for an PAL object and parse it.
 *
 * Returns NULL if the current object is not complete yet (expecting
 * more lines of input) or a reference to a complete object if it
 * is complete.
 *
 * The returned object is a reference to an internal data structure.
 * It should not be modified or freed by the caller.
 *
 * If the input is invalid or other problems happen, then a CPLError()
 * will be generated.  CPLGetLastErrorNo() should be called to check
 * that the line was parsed succesfully.
 **********************************************************************/
AVCPal   *AVCE00ParseNextPalLine(AVCE00ParseInfo *psInfo, const char *pszLine)
{
    AVCPal *psPal;
    int     nLen;

    CPLAssert(psInfo->eFileType == AVCFilePAL ||
              psInfo->eFileType == AVCFileRPL );

    psPal = psInfo->cur.psPal;

    nLen = strlen(pszLine);

    if (psInfo->numItems == 0)
    {
        /*-------------------------------------------------------------
         * Begin processing a new object, read header line:
         *    numArcs, MinX, MinY, MaxX, MaxY
         * For Double precision, MaxX, MaxY are on a separate line.
         *------------------------------------------------------------*/
        if (nLen < 52)
        {
            CPLError(CE_Failure, CPLE_AppDefined, 
                     "Error parsing E00 PAL line: \"%s\"", pszLine);
            return NULL;
        }
        else
        {
            /* Polygon Id is not stored in the E00 file.  Polygons are
             * stored in increasing order, starting at 1... so we just 
             * increment the previous value.
             */
            psPal->nPolyId = ++psInfo->nCurObjectId;

            psPal->numArcs = AVCE00Str2Int(pszLine, 10);

            /* If a PAL record has 0 arcs, it really has a single "0 0 0"
             * triplet as its data.
             */
            if ( psPal->numArcs == 0 )
            {
               psPal->numArcs = 1;
            }

            /* Realloc the array of Arcs
             */
            psPal->pasArcs = (AVCPalArc*)CPLRealloc(psPal->pasArcs,
                                                    psPal->numArcs*
                                                    sizeof(AVCPalArc));

            /* psInfo->iCurItem is the index of the last arc that was read.
             * psInfo->numItems is the number of arcs to read.
             */
            psInfo->iCurItem = 0;
            psInfo->numItems = psPal->numArcs;

            if (psInfo->nPrecision == AVC_SINGLE_PREC)
            {
                psPal->sMin.x = atof(pszLine + 10);
                psPal->sMin.y = atof(pszLine + 24);
                psPal->sMax.x = atof(pszLine + 38);
                psPal->sMax.y = atof(pszLine + 52);
            }
            else
            {
                psPal->sMin.x = atof(pszLine + 10);
                psPal->sMin.y = atof(pszLine + 31);
                /* Set psInfo->iCurItem = -1 since we still have 2 values
                 * from the header to read on the next line.
                 */
                psInfo->iCurItem = -1;
            }

        }
    }
    else if (psInfo->iCurItem == -1 && nLen >= 42)
    {
        psPal->sMax.x = atof(pszLine);
        psPal->sMax.y = atof(pszLine + 21);
        psInfo->iCurItem++;
    }
    else if (psInfo->iCurItem < psPal->numArcs && 
             (nLen >= 60 ||
              (psInfo->iCurItem == psPal->numArcs-1 && nLen >= 30)) )
    {
        /*-------------------------------------------------------------
         * 2 PAL entries (ArcId, FNode, AdjPoly) per line, 
         * (Except on the last line with an odd number of vertices)
         *------------------------------------------------------------*/
        psPal->pasArcs[psInfo->iCurItem].nArcId = AVCE00Str2Int(pszLine, 10);
        psPal->pasArcs[psInfo->iCurItem].nFNode = AVCE00Str2Int(pszLine+10,10);
        psPal->pasArcs[psInfo->iCurItem++].nAdjPoly = AVCE00Str2Int(pszLine+20,
                                                                    10);

        if (psInfo->iCurItem < psInfo->numItems)
        {
            psPal->pasArcs[psInfo->iCurItem].nArcId = AVCE00Str2Int(pszLine+30,
                                                                    10);
            psPal->pasArcs[psInfo->iCurItem].nFNode = AVCE00Str2Int(pszLine+40,
                                                                    10);
            psPal->pasArcs[psInfo->iCurItem++].nAdjPoly = 
                                                AVCE00Str2Int(pszLine+50, 10);
        }
 
    }
    else
    {
        CPLError(CE_Failure, CPLE_AppDefined, 
                 "Error parsing E00 PAL line: \"%s\"", pszLine);
        psInfo->numItems = psInfo->iCurItem = 0;
        return NULL;
    }

    /*-----------------------------------------------------------------
     * If we're done parsing this PAL, then reset the ParseInfo,
     * and return a reference to the PAL structure
     * Otherwise return NULL, which means that we are expecting more
     * more lines of input.
     *----------------------------------------------------------------*/
    if (psInfo->iCurItem >= psInfo->numItems)
    {
        psInfo->numItems = psInfo->iCurItem = 0;
        return psPal;
    }

    return NULL;
}


/**********************************************************************
 *                          AVCE00ParseNextCntLine()
 *
 * Take the next line of E00 input for an CNT object and parse it.
 *
 * Returns NULL if the current object is not complete yet (expecting
 * more lines of input) or a reference to a complete object if it
 * is complete.
 *
 * The returned object is a reference to an internal data structure.
 * It should not be modified or freed by the caller.
 *
 * If the input is invalid or other problems happen, then a CPLError()
 * will be generated.  CPLGetLastErrorNo() should be called to check
 * that the line was parsed succesfully.
 **********************************************************************/
AVCCnt   *AVCE00ParseNextCntLine(AVCE00ParseInfo *psInfo, const char *pszLine)
{
    AVCCnt *psCnt;
    int     nLen;

    CPLAssert(psInfo->eFileType == AVCFileCNT);

    psCnt = psInfo->cur.psCnt;

    nLen = strlen(pszLine);

    if (psInfo->numItems == 0)
    {
        /*-------------------------------------------------------------
         * Begin processing a new object, read header line:
         *    numLabels, X, Y
         *------------------------------------------------------------*/
        if (nLen < 38)
        {
            CPLError(CE_Failure, CPLE_AppDefined, 
                     "Error parsing E00 CNT line: \"%s\"", pszLine);
            return NULL;
        }
        else
        {
            /* Polygon Id is not stored in the E00 file.  Centroids are
             * stored in increasing order of Polygon Id, starting at 1...
             * so we just increment the previous value.
             */
            psCnt->nPolyId = ++psInfo->nCurObjectId;

            psCnt->numLabels = AVCE00Str2Int(pszLine, 10);

            /* Realloc the array of Labels Ids
             * Avoid allocating a 0-length segment since centroids can have
             * 0 labels attached to them.
             */
            if (psCnt->numLabels > 0)
                psCnt->panLabelIds = (GInt32 *)CPLRealloc(psCnt->panLabelIds,
                                                          psCnt->numLabels*
                                                          sizeof(GInt32));

            if (psInfo->nPrecision == AVC_SINGLE_PREC)
            {
                psCnt->sCoord.x = atof(pszLine + 10);
                psCnt->sCoord.y = atof(pszLine + 24);
            }
            else
            {
                psCnt->sCoord.x = atof(pszLine + 10);
                psCnt->sCoord.y = atof(pszLine + 31);
            }

            /* psInfo->iCurItem is the index of the last label that was read.
             * psInfo->numItems is the number of label ids to read.
             */
            psInfo->iCurItem = 0;
            psInfo->numItems = psCnt->numLabels;

        }
    }
    else if (psInfo->iCurItem < psInfo->numItems )
    {
        /*-------------------------------------------------------------
         * Each line can contain up to 8 label ids (10 chars each)
         *------------------------------------------------------------*/
        int i=0;
        while(psInfo->iCurItem < psInfo->numItems && nLen >= (i+1)*10)
        {
            psCnt->panLabelIds[psInfo->iCurItem++] = 
                                  AVCE00Str2Int(pszLine + i*10, 10);
            i++;
        }

    }
    else
    {
        CPLError(CE_Failure, CPLE_AppDefined, 
                 "Error parsing E00 CNT line: \"%s\"", pszLine);
        psInfo->numItems = psInfo->iCurItem = 0;
        return NULL;
    }

    /*-----------------------------------------------------------------
     * If we're done parsing this CNT, then reset the ParseInfo,
     * and return a reference to the CNT structure
     * Otherwise return NULL, which means that we are expecting more
     * more lines of input.
     *----------------------------------------------------------------*/
    if (psInfo->iCurItem >= psInfo->numItems)
    {
        psInfo->numItems = psInfo->iCurItem = 0;
        return psCnt;
    }

    return NULL;
}

/**********************************************************************
 *                          AVCE00ParseNextLabLine()
 *
 * Take the next line of E00 input for an LAB object and parse it.
 *
 * Returns NULL if the current object is not complete yet (expecting
 * more lines of input) or a reference to a complete object if it
 * is complete.
 *
 * The returned object is a reference to an internal data structure.
 * It should not be modified or freed by the caller.
 *
 * If the input is invalid or other problems happen, then a CPLError()
 * will be generated.  CPLGetLastErrorNo() should be called to check
 * that the line was parsed succesfully.
 **********************************************************************/
AVCLab   *AVCE00ParseNextLabLine(AVCE00ParseInfo *psInfo, const char *pszLine)
{
    AVCLab *psLab;
    int     nLen;

    CPLAssert(psInfo->eFileType == AVCFileLAB);

    psLab = psInfo->cur.psLab;

    nLen = strlen(pszLine);

    if (psInfo->numItems == 0)
    {
        /*-------------------------------------------------------------
         * Begin processing a new object, read header line:
         *    LabelValue, PolyId, X1, Y1
         *------------------------------------------------------------*/
        if (nLen < 48)
        {
            CPLError(CE_Failure, CPLE_AppDefined,
                     "Error parsing E00 LAB line: \"%s\"", pszLine);
            return NULL;
        }
        else
        {
            psLab->nValue = AVCE00Str2Int(pszLine, 10);
            psLab->nPolyId = AVCE00Str2Int(pszLine+10, 10);

            if (psInfo->nPrecision == AVC_SINGLE_PREC)
            {
                psLab->sCoord1.x = atof(pszLine + 20);
                psLab->sCoord1.y = atof(pszLine + 34);
            }
            else
            {
                psLab->sCoord1.x = atof(pszLine + 20);
                psLab->sCoord1.y = atof(pszLine + 41);
            }

            /* psInfo->iCurItem is the index of the last X,Y pair we read.
             * psInfo->numItems is the number of X,Y pairs to read.
             */
            psInfo->iCurItem = 1;
            psInfo->numItems = 3;


        }
    }
    else if (psInfo->iCurItem == 1 && psInfo->nPrecision == AVC_SINGLE_PREC &&
             nLen >= 56 )
    {
        psLab->sCoord2.x = atof(pszLine);
        psLab->sCoord2.y = atof(pszLine + 14);
        psLab->sCoord3.x = atof(pszLine + 28);
        psLab->sCoord3.y = atof(pszLine + 42);
        psInfo->iCurItem += 2;
    }
    else if (psInfo->iCurItem == 1 && psInfo->nPrecision == AVC_DOUBLE_PREC &&
             nLen >= 42 )
    {
        psLab->sCoord2.x = atof(pszLine);
        psLab->sCoord2.y = atof(pszLine + 21);
        psInfo->iCurItem++;
    }
    else if (psInfo->iCurItem == 2 && psInfo->nPrecision == AVC_DOUBLE_PREC &&
             nLen >= 42 )
    {
        psLab->sCoord3.x = atof(pszLine);
        psLab->sCoord3.y = atof(pszLine + 21);
        psInfo->iCurItem++;
    }
    else
    {
        CPLError(CE_Failure, CPLE_AppDefined, 
                 "Error parsing E00 LAB line: \"%s\"", pszLine);
        psInfo->numItems = psInfo->iCurItem = 0;
        return NULL;
    }

    /*-----------------------------------------------------------------
     * If we're done parsing this LAB, then reset the ParseInfo,
     * and return a reference to the LAB structure
     * Otherwise return NULL, which means that we are expecting more
     * more lines of input.
     *----------------------------------------------------------------*/
    if (psInfo->iCurItem >= psInfo->numItems)
    {
        psInfo->numItems = psInfo->iCurItem = 0;
        return psLab;
    }

    return NULL;
}



/**********************************************************************
 *                          AVCE00ParseNextTolLine()
 *
 * Take the next line of E00 input for an TOL object and parse it.
 *
 * Returns NULL if the current object is not complete yet (expecting
 * more lines of input) or a reference to a complete object if it
 * is complete.
 *
 * The returned object is a reference to an internal data structure.
 * It should not be modified or freed by the caller.
 *
 * If the input is invalid or other problems happen, then a CPLError()
 * will be generated.  CPLGetLastErrorNo() should be called to check
 * that the line was parsed succesfully.
 **********************************************************************/
AVCTol   *AVCE00ParseNextTolLine(AVCE00ParseInfo *psInfo, const char *pszLine)
{
    AVCTol *psTol;
    int     nLen;

    CPLAssert(psInfo->eFileType == AVCFileTOL);

    psTol = psInfo->cur.psTol;

    nLen = strlen(pszLine);

    if (nLen >= 34)
    {
        /*-------------------------------------------------------------
         * TOL Entries are only one line each:
         *   TolIndex, TolFlag, TolValue
         *------------------------------------------------------------*/
        psTol->nIndex = AVCE00Str2Int(pszLine, 10);
        psTol->nFlag  = AVCE00Str2Int(pszLine + 10, 10);

        psTol->dValue = atof(pszLine + 20);
    }
    else
    {
        CPLError(CE_Failure, CPLE_AppDefined, 
                 "Error parsing E00 TOL line: \"%s\"", pszLine);
        psInfo->numItems = psInfo->iCurItem = 0;
        return NULL;
    }

    /*-----------------------------------------------------------------
     * If we're done parsing this TOL, then reset the ParseInfo,
     * and return a reference to the TOL structure
     * Otherwise return NULL, which means that we are expecting more
     * more lines of input.
     *----------------------------------------------------------------*/
    if (psInfo->iCurItem >= psInfo->numItems)
    {
        psInfo->numItems = psInfo->iCurItem = 0;
        return psTol;
    }

    return NULL;
}

/**********************************************************************
 *                          AVCE00ParseNextPrjLine()
 *
 * Take the next line of E00 input for a PRJ object and parse it.
 *
 * Returns NULL if the current object is not complete yet (expecting
 * more lines of input) or a reference to a complete object if it
 * is complete.
 *
 * Since a PRJ section contains only ONE projection, the function will
 * always return NULL, until it reaches the end-of-section (EOP) line.
 * This is behavior is a bit different from the other section types that
 * will usually return a valid object immediately before the last line
 * of the section (the end-of-section line).
 *
 * The returned object is a reference to an internal data structure.
 * It should not be modified or freed by the caller.
 *
 * If the input is invalid or other problems happen, then a CPLError()
 * will be generated.  CPLGetLastErrorNo() should be called to check
 * that the line was parsed succesfully.
 **********************************************************************/
char  **AVCE00ParseNextPrjLine(AVCE00ParseInfo *psInfo, const char *pszLine)
{
    CPLAssert(psInfo->eFileType == AVCFilePRJ);

    /*-------------------------------------------------------------
     * Since a PRJ section contains only ONE projection, this function will
     * always return NULL until it reaches the end-of-section (EOP) line.
     * This is behavior is a bit different from the other section types that
     * will usually return a valid object immediately before the last line
     * of the section (the end-of-section line).
     *------------------------------------------------------------*/

    if (EQUALN(pszLine, "EOP", 3))
    {
        /*-------------------------------------------------------------
         * We reached end of section... return the PRJ.
         *------------------------------------------------------------*/
        psInfo->bForceEndOfSection = FALSE;
        return psInfo->cur.papszPrj;
    }

    if ( pszLine[0] != '~' )
    {
        /*-------------------------------------------------------------
         * This is a new line... add it to the papszPrj stringlist.
         *------------------------------------------------------------*/
        psInfo->cur.papszPrj = CSLAddString(psInfo->cur.papszPrj, pszLine);
    }
    else if ( strlen(pszLine) > 1 )
    {
        /*-------------------------------------------------------------
         * '~' is a line continuation char.  Append what follows the '~'
         * to the end of the previous line.
         *------------------------------------------------------------*/
        int  iLastLine, nNewLen;

        iLastLine = CSLCount(psInfo->cur.papszPrj) - 1;
        nNewLen = strlen(psInfo->cur.papszPrj[iLastLine])+strlen(pszLine)-1+1;
        if (iLastLine >= 0)
        {
            psInfo->cur.papszPrj[iLastLine] = 
                  (char*)CPLRealloc(psInfo->cur.papszPrj[iLastLine],
                                    nNewLen * sizeof(char));

            strcat(psInfo->cur.papszPrj[iLastLine], pszLine+1);
        }
    }
    
    return NULL;
}


/**********************************************************************
 *                          AVCE00ParseNextTxtLine()
 *
 * Take the next line of E00 input for an TXT object and parse it.
 *
 * Returns NULL if the current object is not complete yet (expecting
 * more lines of input) or a reference to a complete object if it
 * is complete.
 *
 * The returned object is a reference to an internal data structure.
 * It should not be modified or freed by the caller.
 *
 * If the input is invalid or other problems happen, then a CPLError()
 * will be generated.  CPLGetLastErrorNo() should be called to check
 * that the line was parsed succesfully.
 **********************************************************************/
AVCTxt   *AVCE00ParseNextTxtLine(AVCE00ParseInfo *psInfo, const char *pszLine)
{
    AVCTxt *psTxt;
    int     i, nLen, numFixedLines;

    CPLAssert(psInfo->eFileType == AVCFileTXT);

    psTxt = psInfo->cur.psTxt;

    nLen = strlen(pszLine);

    /* numFixedLines is the number of lines to expect before the line(s)
     * with the text string 
     */
    if (psInfo->nPrecision == AVC_SINGLE_PREC)
        numFixedLines = 4;
    else
        numFixedLines = 6;

    if (psInfo->numItems == 0)
    {
        /*-------------------------------------------------------------
         * Begin processing a new object, read header line:
         *------------------------------------------------------------*/
        if (nLen < 50)
        {
            CPLError(CE_Failure, CPLE_AppDefined, 
                     "Error parsing E00 TXT line: \"%s\"", pszLine);
            return NULL;
        }
        else
        {
            int numVertices;
            /*---------------------------------------------------------
             * With TXT, there are several unused fields that have to be
             * set to default values... usually 0.
             *--------------------------------------------------------*/
            psTxt->nUserId = 0;
            psTxt->n28 = 0;
            for(i=0; i<20; i++)
                psTxt->anJust1[i] = psTxt->anJust2[i] = 0;
            psTxt->dV2 = psTxt->dV3 = 0.0;

            /*---------------------------------------------------------
             * System Id is not stored in the E00 file.  Annotations are
             * stored in increasing order of System Id, starting at 1...
             * so we just increment the previous value.
             *--------------------------------------------------------*/
            psTxt->nTxtId = ++psInfo->nCurObjectId;

            psTxt->nLevel          = AVCE00Str2Int(pszLine, 10);

            /* Add 1 to numVerticesLine because the first vertex is 
             * always duplicated in the TXT binary structure...
             */
            psTxt->numVerticesLine = AVCE00Str2Int(pszLine+10, 10) + 1;

            psTxt->numVerticesArrow= AVCE00Str2Int(pszLine+20, 10);
            psTxt->nSymbol         = AVCE00Str2Int(pszLine+30, 10);
            psTxt->numChars        = AVCE00Str2Int(pszLine+40, 10);


            /*---------------------------------------------------------
             * Realloc the string buffer and array of vertices
             *--------------------------------------------------------*/
            psTxt->pszText = (char *)CPLRealloc(psTxt->pszText,
                                                (psTxt->numChars+1)*
                                                    sizeof(char));
            numVertices = ABS(psTxt->numVerticesLine) + 
                                 ABS(psTxt->numVerticesArrow);
            if (numVertices > 0)
                psTxt->pasVertices = (AVCVertex*)CPLRealloc(psTxt->pasVertices,
                                              numVertices*sizeof(AVCVertex));

            /*---------------------------------------------------------
             * Fill the whole string buffer with spaces we'll just
             * paste lines in it using strncpy()
             *--------------------------------------------------------*/
            memset(psTxt->pszText, ' ', psTxt->numChars);
            psTxt->pszText[psTxt->numChars] = '\0';

            /*---------------------------------------------------------
             * psInfo->iCurItem is the index of the last line that was read.
             * psInfo->numItems is the number of lines to read.
             *--------------------------------------------------------*/
            psInfo->iCurItem = 0;
            psInfo->numItems = numFixedLines + ((psTxt->numChars-1)/80 + 1);

        }
    }
    else if (psInfo->iCurItem < psInfo->numItems &&
             psInfo->iCurItem < numFixedLines-1 && nLen >=63)
    {
        /*-------------------------------------------------------------
         * Then we have a set of 15 coordinate values... unused ones
         * are present but are set to 0.00E+00
         *
         * Vals 1 to 4 are X coords of line along which text is drawn
         * Vals 5 to 8 are the corresponding Y coords
         * Vals 9 to 11 are the X coords of the text arrow
         * Vals 12 to 14 are the corresponding Y coords
         * The 15th value is the height
         *
         * Note that the first vertex (values 1 and 5) is duplicated
         * in the TXT structure... go wonder why???
         *------------------------------------------------------------*/
        int iCurCoord=0, numCoordPerLine, nItemSize, iVertex;
        if (psInfo->nPrecision == AVC_SINGLE_PREC)
        {
            numCoordPerLine = 5;
            nItemSize = 14;  /* Num of chars for single precision float*/
        }
        else
        {
            numCoordPerLine = 3;
            nItemSize = 21;  /* Num of chars for double precision float*/
        }
        iCurCoord = psInfo->iCurItem * numCoordPerLine;

        for(i=0; i<numCoordPerLine; i++, iCurCoord++)
        {
            if (iCurCoord < 4 && 
                (iVertex = iCurCoord % 4) < psTxt->numVerticesLine-1)
            {
                psTxt->pasVertices[iVertex+1].x = atof(pszLine+i*nItemSize);
                /* The first vertex is always duplicated */
                if (iVertex == 0)
                    psTxt->pasVertices[0].x = psTxt->pasVertices[1].x;
            }
            else if (iCurCoord >= 4 && iCurCoord < 8 &&
                     (iVertex = iCurCoord % 4) < psTxt->numVerticesLine-1)
            {
                psTxt->pasVertices[iVertex+1].y = atof(pszLine+i*nItemSize);
                /* The first vertex is always duplicated */
                if (iVertex == 0)
                    psTxt->pasVertices[0].y = psTxt->pasVertices[1].y;
            }
            else if (iCurCoord >= 8 && iCurCoord < 11 &&
                     (iVertex = (iCurCoord-8) % 3) < psTxt->numVerticesArrow)
            {
                psTxt->pasVertices[iVertex+psTxt->numVerticesLine].x =
                                                    atof(pszLine+i*nItemSize);
            }
            else if (iCurCoord >= 11 && iCurCoord < 14 &&
                     (iVertex = (iCurCoord-8) % 3) < psTxt->numVerticesArrow)
            {
                psTxt->pasVertices[iVertex+psTxt->numVerticesLine].y =
                                                    atof(pszLine+i*nItemSize);
            }
            else if (iCurCoord == 14)
            {
                psTxt->dHeight = atof(pszLine+i*nItemSize);
            }

        }

        psInfo->iCurItem++;
    }
    else if (psInfo->iCurItem < psInfo->numItems &&
             psInfo->iCurItem == numFixedLines-1  && nLen >=14)
    {
        /*-------------------------------------------------------------
         * Line with a -1.000E+02 value, ALWAYS SINGLE PRECISION !!!
         *------------------------------------------------------------*/
        psTxt->f_1e2 = (float)atof(pszLine);

        psInfo->iCurItem++;
    }
    else if (psInfo->iCurItem < psInfo->numItems &&
             psInfo->iCurItem >= numFixedLines)
    {
        /*-------------------------------------------------------------
         * Last line, contains the text string
         * Note that text can be split in 80 chars chunk and that buffer
         * has been previously initialized with spaces and '\0'-terminated
         *------------------------------------------------------------*/
        int numLines, iLine;
        numLines = (psTxt->numChars-1)/80 + 1;
        iLine = numLines - (psInfo->numItems - psInfo->iCurItem);

        if (iLine == numLines-1)
        {
            strncpy(psTxt->pszText+(iLine*80), pszLine, 
                    MIN( nLen, (psTxt->numChars - (iLine*80)) ) );
        }
        else
        {
            strncpy(psTxt->pszText+(iLine*80), pszLine, MIN(nLen, 80));
        }

        psInfo->iCurItem++;
    }
    else
    {
        CPLError(CE_Failure, CPLE_AppDefined, 
                 "Error parsing E00 TXT line: \"%s\"", pszLine);
        psInfo->numItems = psInfo->iCurItem = 0;
        return NULL;
    }

    /*-----------------------------------------------------------------
     * If we're done parsing this TXT, then reset the ParseInfo,
     * and return a reference to the TXT structure
     * Otherwise return NULL, which means that we are expecting more
     * more lines of input.
     *----------------------------------------------------------------*/
    if (psInfo->iCurItem >= psInfo->numItems)
    {
        psInfo->numItems = psInfo->iCurItem = 0;
        return psTxt;
    }

    return NULL;
}


/**********************************************************************
 *                          AVCE00ParseNextTx6Line()
 *
 * Take the next line of E00 input for an TX6/TX7 object and parse it.
 *
 * Returns NULL if the current object is not complete yet (expecting
 * more lines of input) or a reference to a complete object if it
 * is complete.
 *
 * The returned object is a reference to an internal data structure.
 * It should not be modified or freed by the caller.
 *
 * If the input is invalid or other problems happen, then a CPLError()
 * will be generated.  CPLGetLastErrorNo() should be called to check
 * that the line was parsed succesfully.
 **********************************************************************/
AVCTxt   *AVCE00ParseNextTx6Line(AVCE00ParseInfo *psInfo, const char *pszLine)
{
    AVCTxt *psTxt;
    int     i, nLen;

    CPLAssert(psInfo->eFileType == AVCFileTX6);

    psTxt = psInfo->cur.psTxt;

    nLen = strlen(pszLine);

    if (psInfo->numItems == 0)
    {
        /*-------------------------------------------------------------
         * Begin processing a new object, read header line:
         *------------------------------------------------------------*/
        if (nLen < 70)
        {
            CPLError(CE_Failure, CPLE_AppDefined, 
                     "Error parsing E00 TX6/TX7 line: \"%s\"", pszLine);
            return NULL;
        }
        else
        {
            int numVertices;
            /*---------------------------------------------------------
             * System Id is not stored in the E00 file.  Annotations are
             * stored in increasing order of System Id, starting at 1...
             * so we just increment the previous value.
             *--------------------------------------------------------*/
            psTxt->nTxtId = ++psInfo->nCurObjectId;

            psTxt->nUserId         = AVCE00Str2Int(pszLine, 10);
            psTxt->nLevel          = AVCE00Str2Int(pszLine+10, 10);
            psTxt->numVerticesLine = AVCE00Str2Int(pszLine+20, 10);
            psTxt->numVerticesArrow= AVCE00Str2Int(pszLine+30, 10);
            psTxt->nSymbol         = AVCE00Str2Int(pszLine+40, 10);
            psTxt->n28             = AVCE00Str2Int(pszLine+50, 10);
            psTxt->numChars        = AVCE00Str2Int(pszLine+60, 10);

            /*---------------------------------------------------------
             * Realloc the string buffer and array of vertices
             *--------------------------------------------------------*/
            psTxt->pszText = (char *)CPLRealloc(psTxt->pszText,
                                                (psTxt->numChars+1)*
                                                sizeof(char));

            numVertices = ABS(psTxt->numVerticesLine) + 
                                 ABS(psTxt->numVerticesArrow);
            if (numVertices > 0)
                psTxt->pasVertices = (AVCVertex*)CPLRealloc(psTxt->pasVertices,
                                              numVertices*sizeof(AVCVertex));

            /*---------------------------------------------------------
             * Fill the whole string buffer with spaces we'll just
             * paste lines in it using strncpy()
             *--------------------------------------------------------*/
            memset(psTxt->pszText, ' ', psTxt->numChars);
            psTxt->pszText[psTxt->numChars] = '\0';

            /*---------------------------------------------------------
             * psInfo->iCurItem is the index of the last line that was read.
             * psInfo->numItems is the number of lines to read.
             *--------------------------------------------------------*/
            psInfo->iCurItem = 0;
            psInfo->numItems = 8 + numVertices + ((psTxt->numChars-1)/80 + 1);
        }
    }
    else if (psInfo->iCurItem < psInfo->numItems && 
             psInfo->iCurItem < 6 && nLen >=60)
    {
        /*-------------------------------------------------------------
         * Text Justification stuff... 2 sets of 20 int16 values.
         *------------------------------------------------------------*/
        GInt16  *pValue;
        int     numValPerLine=7;

        if (psInfo->iCurItem < 3)
            pValue = psTxt->anJust2 + psInfo->iCurItem * 7;
        else
            pValue = psTxt->anJust1 + (psInfo->iCurItem-3) * 7;

        /* Last line of each set contains only 6 values instead of 7 */
        if (psInfo->iCurItem == 2 || psInfo->iCurItem == 5)
            numValPerLine = 6;

        for(i=0; i<numValPerLine; i++)
            pValue[i] = AVCE00Str2Int(pszLine + i*10, 10);

        psInfo->iCurItem++;
    }
    else if (psInfo->iCurItem < psInfo->numItems && 
             psInfo->iCurItem == 6 && nLen >=14)
    {
        /*-------------------------------------------------------------
         * Line with a -1.000E+02 value, ALWAYS SINGLE PRECISION !!!
         *------------------------------------------------------------*/
        psTxt->f_1e2 = (float)atof(pszLine);
        psInfo->iCurItem++;
    }
    else if (psInfo->iCurItem < psInfo->numItems && 
             psInfo->iCurItem == 7 && nLen >=42)
    {
        /*-------------------------------------------------------------
         * Line with 3 values, 1st value is text height.
         *------------------------------------------------------------*/
        psTxt->dHeight = atof(pszLine);
        if (psInfo->nPrecision == AVC_SINGLE_PREC)
        {
            psTxt->dV2     = atof(pszLine+14);
            psTxt->dV3     = atof(pszLine+28);
        }
        else
        {
            psTxt->dV2     = atof(pszLine+21);
            psTxt->dV3     = atof(pszLine+42);
        }

        psInfo->iCurItem++;
    }
    else if (psInfo->iCurItem < (8 + ABS(psTxt->numVerticesLine) + 
                                   ABS(psTxt->numVerticesArrow)) && nLen >= 28)
    {
        /*-------------------------------------------------------------
         * One line for each pair of X,Y coordinates
         * (Lines 8 to 8+numVertices-1)
         *------------------------------------------------------------*/
        psTxt->pasVertices[ psInfo->iCurItem-8 ].x = atof(pszLine);
        if (psInfo->nPrecision == AVC_SINGLE_PREC)
            psTxt->pasVertices[ psInfo->iCurItem-8 ].y = atof(pszLine+14);
        else
            psTxt->pasVertices[ psInfo->iCurItem-8 ].y = atof(pszLine+21);

        psInfo->iCurItem++;
    }
    else if (psInfo->iCurItem < psInfo->numItems)
    {
        /*-------------------------------------------------------------
         * Last line, contains the text string
         * Note that text can be split in 80 chars chunk and that buffer
         * has been previously initialized with spaces and '\0'-terminated
         *------------------------------------------------------------*/
        int numLines, iLine;
        numLines = (psTxt->numChars-1)/80 + 1;
        iLine = numLines - (psInfo->numItems - psInfo->iCurItem);

        if (iLine == numLines-1)
        {
            strncpy(psTxt->pszText+(iLine*80), pszLine, 
                    MIN( nLen, (psTxt->numChars - (iLine*80)) ) );
        }
        else
        {
            strncpy(psTxt->pszText+(iLine*80), pszLine, MIN(nLen, 80));
        }

        psInfo->iCurItem++;
    }
    else
    {
        CPLError(CE_Failure, CPLE_AppDefined, 
                 "Error parsing E00 TX6/TX7 line: \"%s\"", pszLine);
        psInfo->numItems = psInfo->iCurItem = 0;
        return NULL;
    }

    /*-----------------------------------------------------------------
     * If we're done parsing this TX6/TX7, then reset the ParseInfo,
     * and return a reference to the TXT structure
     * Otherwise return NULL, which means that we are expecting more
     * more lines of input.
     *----------------------------------------------------------------*/
    if (psInfo->iCurItem >= psInfo->numItems)
    {
        psInfo->numItems = psInfo->iCurItem = 0;
        return psTxt;
    }

    return NULL;
}



/**********************************************************************
 *                          AVCE00ParseNextRxpLine()
 *
 * Take the next line of E00 input for an RXP object and parse it.
 *
 * Returns NULL if the current object is not complete yet (expecting
 * more lines of input) or a reference to a complete object if it
 * is complete.
 *
 * The returned object is a reference to an internal data structure.
 * It should not be modified or freed by the caller.
 *
 * If the input is invalid or other problems happen, then a CPLError()
 * will be generated.  CPLGetLastErrorNo() should be called to check
 * that the line was parsed succesfully.
 **********************************************************************/
AVCRxp   *AVCE00ParseNextRxpLine(AVCE00ParseInfo *psInfo, const char *pszLine)
{
    AVCRxp *psRxp;
    int     nLen;

    CPLAssert(psInfo->eFileType == AVCFileRXP);

    psRxp = psInfo->cur.psRxp;

    nLen = strlen(pszLine);

    if (nLen >= 20)
    {
        /*-------------------------------------------------------------
         * RXP Entries are only one line each:
         *   Value1, Value2 (meaning of the value??? Don't know!!!)
         *------------------------------------------------------------*/
        psRxp->n1 = AVCE00Str2Int(pszLine, 10);
        psRxp->n2 = AVCE00Str2Int(pszLine + 10, 10);
    }
    else
    {
        CPLError(CE_Failure, CPLE_AppDefined, 
                 "Error parsing E00 RXP line: \"%s\"", pszLine);
        psInfo->numItems = psInfo->iCurItem = 0;
        return NULL;
    }

    /*-----------------------------------------------------------------
     * If we're done parsing this RXP, then reset the ParseInfo,
     * and return a reference to the RXP structure
     * Otherwise return NULL, which means that we are expecting more
     * more lines of input.
     *----------------------------------------------------------------*/
    if (psInfo->iCurItem >= psInfo->numItems)
    {
        psInfo->numItems = psInfo->iCurItem = 0;
        return psRxp;
    }

    return NULL;
}


/*=====================================================================
                            TABLE stuff
 =====================================================================*/

/**********************************************************************
 *                          AVCE00ParseNextTableDefLine()
 *
 * Take the next line of E00 input for an TableDef object and parse it.
 *
 * Returns NULL if the current object is not complete yet (expecting
 * more lines of input) or a reference to a complete object if it
 * is complete.
 *
 * The returned object is a reference to an internal data structure.
 * It should not be modified or freed by the caller.
 *
 * If the input is invalid or other problems happen, then a CPLError()
 * will be generated.  CPLGetLastErrorNo() should be called to check
 * that the line was parsed succesfully.
 **********************************************************************/
AVCTableDef   *AVCE00ParseNextTableDefLine(AVCE00ParseInfo *psInfo, 
                                           const char *pszLine)
{
    AVCTableDef *psTableDef;
    int     nLen;

    CPLAssert(psInfo->eFileType == AVCFileTABLE);

    psTableDef = psInfo->hdr.psTableDef;  /* May be NULL on first call */

    nLen = strlen(pszLine);

    if (psInfo->numItems == 0)
    {
        /*-------------------------------------------------------------
         * Begin processing a new TableDef.  Read header line:
         *    TableName, extFlag, numFields, RecSize, numRecords
         *------------------------------------------------------------*/
        if (nLen < 56)
        {
            CPLError(CE_Failure, CPLE_AppDefined,
                     "Error parsing E00 Table Definition line: \"%s\"", 
                     pszLine);
            return NULL;
        }
        else
        {
            /*---------------------------------------------------------
             * Parse header line and alloc and init. a new psTableDef struct
             *--------------------------------------------------------*/
            psTableDef = psInfo->hdr.psTableDef = 
                               (AVCTableDef*)CPLCalloc(1, sizeof(AVCTableDef));
            psInfo->bTableHdrComplete = FALSE;

            strncpy(psTableDef->szTableName, pszLine, 32);
            psTableDef->szTableName[32] = '\0';
            strncpy(psTableDef->szExternal, pszLine+32, 2);
            psTableDef->szExternal[2] = '\0';

            psTableDef->numFields  = AVCE00Str2Int(pszLine+34, 4);
            psTableDef->nRecSize   = AVCE00Str2Int(pszLine+42, 4);
            psTableDef->numRecords = AVCE00Str2Int(pszLine+46, 10);

            /*---------------------------------------------------------
             * Alloc array of fields defs, will be filled in further calls
             *--------------------------------------------------------*/
            psTableDef->pasFieldDef = 
                    (AVCFieldInfo*)CPLCalloc(psTableDef->numFields,
                                             sizeof(AVCFieldInfo));

            /*---------------------------------------------------------
             * psInfo->iCurItem is the index of the last field def we read.
             * psInfo->numItems is the number of field defs to read,
             *                     including deleted ones.
             *--------------------------------------------------------*/
            psInfo->numItems = AVCE00Str2Int(pszLine+38, 4);
            psInfo->iCurItem = 0;
            psInfo->nCurObjectId = 0;  /* We'll use it as a field index */
        }
    }
    else if (psInfo->iCurItem < psInfo->numItems && nLen >= 69 )
    {
        /*-------------------------------------------------------------
         * Read an attribute field definition
         * If field index is -1, then we ignore this line... we do not
         * even count it in psInfo->iCurItem.
         *------------------------------------------------------------*/
        int nIndex;

        nIndex = AVCE00Str2Int(pszLine + 65, 4);

        if (nIndex > 0 && psInfo->nCurObjectId >= psTableDef->numFields)
        {
            CPLError(CE_Failure, CPLE_AppDefined, 
                     "Error parsing E00 INFO Table Header: "
                     "number of fields is invalid "
                     "(expected %d, got at least %d)",
                     psTableDef->numFields, psInfo->nCurObjectId+1);
            psInfo->numItems = psInfo->iCurItem = psInfo->nCurObjectId;
            return NULL;
        }

        if (nIndex > 0)
        {
            AVCFieldInfo *psDef;
            psDef = &(psTableDef->pasFieldDef[psInfo->iCurItem]);

            psDef->nIndex   = nIndex;
            
            strncpy(psDef->szName, pszLine, 16);
            psDef->szName[16] = '\0';

            psDef->nSize    = AVCE00Str2Int(pszLine + 16, 3);
            psDef->v2       = AVCE00Str2Int(pszLine + 19, 2);

            psDef->nOffset  = AVCE00Str2Int(pszLine + 21, 4);

            psDef->v4       = AVCE00Str2Int(pszLine + 25, 1);
            psDef->v5       = AVCE00Str2Int(pszLine + 26, 2);
            psDef->nFmtWidth= AVCE00Str2Int(pszLine + 28, 4);
            psDef->nFmtPrec = AVCE00Str2Int(pszLine + 32, 2);
            psDef->nType1   = AVCE00Str2Int(pszLine + 34, 3)/10;
            psDef->nType2   = AVCE00Str2Int(pszLine + 34, 3)%10;
            psDef->v10      = AVCE00Str2Int(pszLine + 37, 2);
            psDef->v11      = AVCE00Str2Int(pszLine + 39, 4);
            psDef->v12      = AVCE00Str2Int(pszLine + 43, 4);
            psDef->v13      = AVCE00Str2Int(pszLine + 47, 2);

            strncpy(psDef->szAltName, pszLine+49, 16);
            psDef->szAltName[16] = '\0';

            psInfo->nCurObjectId++;
        }
        psInfo->iCurItem++;
    }
    else
    {
        CPLError(CE_Failure, CPLE_AppDefined, 
                 "Error parsing E00 Table Definition line: \"%s\"", pszLine);
        psInfo->numItems = psInfo->iCurItem = 0;
        return NULL;
    }

    /*-----------------------------------------------------------------
     * If we're done parsing this TableDef, then reset the ParseInfo,
     * and return a reference to the TableDef structure.
     * Next calls should go to AVCE00ParseNextTableRecLine() to
     * read data records.
     * Otherwise return NULL, which means that we are expecting more
     * more lines of input.
     *----------------------------------------------------------------*/
    if (psInfo->iCurItem >= psInfo->numItems)
    {
        psInfo->numItems = psInfo->iCurItem = 0;
        psInfo->nCurObjectId = 0;

        psInfo->bTableHdrComplete = TRUE;

        /*---------------------------------------------------------
         * It is possible to have a table with 0 records... in this
         * case we are already at the end of the section for that table.
         *--------------------------------------------------------*/
        if (psTableDef->numRecords == 0)
            psInfo->bForceEndOfSection = TRUE;

        return psTableDef;
    }

    return NULL;
}

/**********************************************************************
 *                         _AVCE00ParseTableRecord()
 *
 * Parse the record data present inside psInfo->pszBuf and fill and
 * return the psInfo->cur.pasFields[].
 *
 * This function should not be called directly... it is used by 
 * AVCE00ParseNextTableRecLine().
 **********************************************************************/
static AVCField   *_AVCE00ParseTableRecord(AVCE00ParseInfo *psInfo)
{
    AVCField    *pasFields;
    AVCFieldInfo *pasDef;
    AVCTableDef *psTableDef;
    int         i, nType, nSize;
    char        *pszBuf, szTmp[30];

    pasFields =  psInfo->cur.pasFields;
    psTableDef = psInfo->hdr.psTableDef;
    pasDef = psTableDef->pasFieldDef;

    pszBuf = psInfo->pszBuf;

    for(i=0; i<psTableDef->numFields; i++)
    {
        nType = pasDef[i].nType1*10;
        nSize = pasDef[i].nSize;

        if (nType ==  AVC_FT_DATE || nType == AVC_FT_CHAR ||
            nType == AVC_FT_FIXINT )
        {
            strncpy(pasFields[i].pszStr, pszBuf, nSize);
            pasFields[i].pszStr[nSize] = '\0';
            pszBuf += nSize;
        }
        else if (nType == AVC_FT_FIXNUM)
        {
            /* TYPE 40 attributes are stored with 1 byte per digit
             * in binary format, and as single precision floats in
             * E00 tables, even in double precision coverages.
             */
            const char *pszTmpStr;
            strncpy(szTmp, pszBuf, 14);
            szTmp[14] = '\0';
            pszBuf += 14;

            /* Compensate for a very odd behavior observed in some E00 files.
             * A type 40 field can be written in decimal format instead of
             * exponent format, but in this case the decimal point is shifted
             * one position to the right, resulting in a value 10 times bigger
             * than expected.  So if the value is not in exponent format then
             * we should shift the decimal point to the left before we 
             * interpret it.  (bug 599)
             */
            if (!strstr(szTmp, "E+") && !strstr(szTmp, "e+"))
            {
                char *pszTmp;
                if ( (pszTmp=strchr(szTmp, '.')) != NULL && pszTmp != szTmp )
                {
                    *pszTmp = *(pszTmp-1);
                    *(pszTmp-1) = '.';
                }
            }

            /* We use nSize and nFmtPrec for the format because nFmtWidth can
             * be different from nSize, but nSize has priority since it
             * is the actual size of the field in memory.
             */
            pszTmpStr = CPLSPrintf("%*.*f", 
                                   nSize, pasDef[i].nFmtPrec, atof(szTmp));

            /* If value is bigger than size, then it's too bad... we 
             * truncate it... but this should never happen in clean datasets.
             */
            if ((int)strlen(pszTmpStr) > nSize)
                pszTmpStr = pszTmpStr + strlen(pszTmpStr) - nSize;
            strncpy(pasFields[i].pszStr, pszTmpStr, nSize);
            pasFields[i].pszStr[nSize] = '\0';
        }
        else if (nType == AVC_FT_BININT && nSize == 4)
        {
            pasFields[i].nInt32 = AVCE00Str2Int(pszBuf, 11);
            pszBuf += 11;
        }
        else if (nType == AVC_FT_BININT && nSize == 2)
        {
            pasFields[i].nInt16 = AVCE00Str2Int(pszBuf, 6);
            pszBuf += 6;
        }
        else if (nType == AVC_FT_BINFLOAT && pasDef[i].nSize == 4)
        {
            /* NOTE: The E00 representation for a binary float is
             * defined by its binary size, not by the coverage's
             * precision.
             */
            strncpy(szTmp, pszBuf, 14);
            szTmp[14] = '\0';
            pasFields[i].fFloat = (float)atof(szTmp);
            pszBuf += 14;
        }
        else if (nType == AVC_FT_BINFLOAT && pasDef[i].nSize == 8)
        {
            /* NOTE: The E00 representation for a binary float is
             * defined by its binary size, not by the coverage's
             * precision.
             */
            strncpy(szTmp, pszBuf, 24);
            szTmp[24] = '\0';
            pasFields[i].dDouble = atof(szTmp);
            pszBuf += 24;
        }
        else
        {
            /*-----------------------------------------------------
             * Hummm... unsupported field type...
             *----------------------------------------------------*/
            CPLError(CE_Failure, CPLE_NotSupported,
                     "_AVCE00ParseTableRecord(): Unsupported field type "
                     "(type=%d, size=%d)",
                     nType, pasDef[i].nSize);
            return NULL;
        }
    }

    CPLAssert(pszBuf == psInfo->pszBuf + psInfo->nTableE00RecLength);

    return pasFields;
}

/**********************************************************************
 *                          AVCE00ParseNextTableRecLine()
 *
 * Take the next line of E00 input for an Table data record and parse it.
 *
 * Returns NULL if the current record is not complete yet (expecting
 * more lines of input) or a reference to a complete record if it
 * is complete.
 *
 * The returned record is a reference to an internal data structure.
 * It should not be modified or freed by the caller.
 *
 * If the input is invalid or other problems happen, then a CPLError()
 * will be generated.  CPLGetLastErrorNo() should be called to check
 * that the line was parsed succesfully.
 **********************************************************************/
AVCField   *AVCE00ParseNextTableRecLine(AVCE00ParseInfo *psInfo, 
                                        const char *pszLine)
{
    AVCField    *pasFields = NULL;
    AVCTableDef *psTableDef;
    int         i;

    CPLAssert(psInfo->eFileType == AVCFileTABLE);

    psTableDef = psInfo->hdr.psTableDef;

    if (psInfo->bForceEndOfSection || psTableDef->numFields == 0 ||
        psTableDef->numRecords == 0)
    {
        psInfo->bForceEndOfSection = TRUE;
        return NULL;
    }

    /*-----------------------------------------------------------------
     * On the first call for a new table, we have some allocations to
     * do:
     * - make sure the psInfo->szBuf is big enough to hold one complete
     *   E00 data record.
     * - Alloc the array of Field values (psInfo->cur.pasFields[])
     *   for the number of fields in this table.
     *----------------------------------------------------------------*/
    if (psInfo->numItems == 0 && psInfo->nCurObjectId == 0)
    {
        /*-------------------------------------------------------------
         * Realloc E00 buffer
         *------------------------------------------------------------*/
        psInfo->nTableE00RecLength = 
            _AVCE00ComputeRecSize(psTableDef->numFields, 
                                  psTableDef->pasFieldDef, FALSE);

        if (psInfo->nBufSize < psInfo->nTableE00RecLength + 1)
        {
            psInfo->nBufSize = psInfo->nTableE00RecLength + 1;
            psInfo->pszBuf = (char*)CPLRealloc(psInfo->pszBuf,
                                               psInfo->nBufSize);
        }

        /*---------------------------------------------------------
         * Alloc psInfo->cur.pasFields[]
         * Also alloc buffers for string attributes.
         *--------------------------------------------------------*/
        psInfo->cur.pasFields = (AVCField*)CPLCalloc(psTableDef->numFields,
                                                     sizeof(AVCField));
        for(i=0; i<psTableDef->numFields; i++)
        {
            if (psTableDef->pasFieldDef[i].nType1*10 == AVC_FT_DATE ||
                psTableDef->pasFieldDef[i].nType1*10 == AVC_FT_CHAR ||
                psTableDef->pasFieldDef[i].nType1*10 == AVC_FT_FIXINT ||
                psTableDef->pasFieldDef[i].nType1*10 == AVC_FT_FIXNUM )
            {
                psInfo->cur.pasFields[i].pszStr = 
                    (char*)CPLCalloc(psTableDef->pasFieldDef[i].nSize+1, 
                                     sizeof(char));
            }
        }

    }

    if (psInfo->numItems == 0)
    {
    /*-----------------------------------------------------------------
     * Begin processing a new record... we'll accumulate the 80
     * chars lines until we have the whole record in our buffer 
     * and parse it only at the end.
     * Lines shorter than 80 chars are legal, and in this case
     * they will be padded with spaces up to 80 chars.
     *----------------------------------------------------------------*/

        /*---------------------------------------------------------
         * First fill the whole record buffer with spaces we'll just
         * paste lines in it using strncpy()
         *--------------------------------------------------------*/
        memset(psInfo->pszBuf, ' ', psInfo->nTableE00RecLength);
        psInfo->pszBuf[psInfo->nTableE00RecLength] = '\0';


        /*---------------------------------------------------------
         * psInfo->iCurItem is the number of chars buffered so far.
         * psInfo->numItems is the number of chars to expect in one record.
         *--------------------------------------------------------*/
        psInfo->numItems = psInfo->nTableE00RecLength;
        psInfo->iCurItem = 0;
    }


    if (psInfo->iCurItem < psInfo->numItems)
    {
        /*-------------------------------------------------------------
         * Continue to accumulate the 80 chars lines until we have 
         * the whole record in our buffer.  We'll parse it only at the end.
         * Lines shorter than 80 chars are legal, and in this case
         * they padded with spaces up to 80 chars.
         *------------------------------------------------------------*/
        int nSrcLen, nLenToCopy;

        nSrcLen = strlen(pszLine);
        nLenToCopy = MIN(80, MIN(nSrcLen,(psInfo->numItems-psInfo->iCurItem)));
        strncpy(psInfo->pszBuf+psInfo->iCurItem, pszLine, nLenToCopy);

        psInfo->iCurItem+=80;
    }


    if (psInfo->iCurItem >= psInfo->numItems)
    {
        /*-------------------------------------------------------------
         * OK, we've got one full record in the buffer... parse it and
         * return the pasFields[]
         *------------------------------------------------------------*/
        pasFields = _AVCE00ParseTableRecord(psInfo);

        if (pasFields == NULL)
        {
            CPLError(CE_Failure, CPLE_AppDefined, 
                     "Error parsing E00 Table Record: \"%s\"", 
                     psInfo->pszBuf);
            return NULL;
        }

        psInfo->numItems = psInfo->iCurItem = 0;
        psInfo->nCurObjectId++;
    }

    /*-----------------------------------------------------------------
     * Since there is no explicit "end of table" line, we set the 
     * bForceEndOfSection flag when the last record is read.
     *----------------------------------------------------------------*/
    if (psInfo->nCurObjectId >= psTableDef->numRecords)
    {
        psInfo->bForceEndOfSection = TRUE;
    }

    return pasFields;
}


