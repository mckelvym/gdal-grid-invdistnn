/******************************************************************************
 * $Id$
 *
 * Project:  EPIInfo .REC Reader
 * Purpose:  Implements OGRRECLayer class.
 * Author:   Frank Warmerdam <warmerdam@pobox.com>
 *
 ******************************************************************************
 * Copyright (c) 2003, Frank Warmerdam <warmerdam@pobox.com>
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
 * Revision 1.3  2003/02/04 21:40:38  warmerda
 * Fixed handling of Cntl-Z.
 *
 * Revision 1.2  2003/02/04 20:42:45  warmerda
 * skip zero length fields, improve type handling
 *
 * Revision 1.1  2003/02/03 21:11:35  warmerda
 * New
 *
 */

#include "ogr_rec.h"
#include "cpl_conv.h"
#include "cpl_string.h"

CPL_CVSID("$Id$");

/************************************************************************/
/*                            RECGetField()                             */
/************************************************************************/

static const char *RECGetField( const char *pszSrc, int nStart, int nWidth )

{
    static char szWorkField[128];
    int         i;
    
    strncpy( szWorkField, pszSrc+nStart-1, nWidth );
    szWorkField[nWidth] = '\0';

    i = strlen(szWorkField)-1;
    
    while( i >= 0 && szWorkField[i] == ' ' )
        szWorkField[i--] = '\0';

    return szWorkField;
}


/************************************************************************/
/*                            OGRRECLayer()                             */
/*                                                                      */
/*      Note that the OGRRECLayer assumes ownership of the passed       */
/*      file pointer.                                                   */
/************************************************************************/

OGRRECLayer::OGRRECLayer( const char *pszLayerNameIn, 
                          FILE * fp, int nFieldCountIn )

{
    fpREC = fp;
    bIsValid = FALSE;
    nStartOfData = 0;

    nNextFID = 1;

    poFeatureDefn = new OGRFeatureDefn( pszLayerNameIn );

    nFieldCount = 0;
    panFieldOffset = (int *) CPLCalloc(sizeof(int),nFieldCountIn);
    panFieldWidth = (int *) CPLCalloc(sizeof(int),nFieldCountIn);

/* -------------------------------------------------------------------- */
/*      Read field definition lines.                                    */
/* -------------------------------------------------------------------- */
    int         iField;

    for( nFieldCount=0, iField = 0; iField < nFieldCountIn; iField++ )
    {
        const char *pszLine = CPLReadLine( fp );
        int         nTypeCode;
        OGRFieldType eFType = OFTString;

        if( pszLine == NULL )
            return;

        if( strlen(pszLine) < 44 )
            return;

        // Extract field width. 
        panFieldWidth[nFieldCount] = atoi( RECGetField( pszLine, 37, 4 ) );

        // Is this an real, integer or string field?  Default to string.
        nTypeCode = atoi(RECGetField(pszLine,33,4));
        if( nTypeCode == 12 )
            eFType = OFTInteger;
        else if( nTypeCode > 100 && nTypeCode < 120 )
        {
            eFType = OFTReal;
        }
        else if( nTypeCode == 0 || nTypeCode == 6 || nTypeCode == 102 )
        {
            if( panFieldWidth[nFieldCount] < 3 )
                eFType = OFTInteger;
            else
                eFType = OFTReal;
        }
        else
            eFType = OFTString;

        OGRFieldDefn oField( RECGetField( pszLine, 2, 10 ), eFType );

        // Establish field offset. 
        if( nFieldCount > 0 )
            panFieldOffset[nFieldCount]
                = panFieldOffset[nFieldCount-1] + panFieldWidth[nFieldCount-1];

        if( nTypeCode > 100 && nTypeCode < 120 )
        {
            oField.SetWidth( panFieldWidth[nFieldCount] );
            oField.SetPrecision( nTypeCode - 100 );
        }
        else if( eFType == OFTReal )
        {
            oField.SetWidth( panFieldWidth[nFieldCount]*2 );
            oField.SetPrecision( panFieldWidth[nFieldCount]-1 );
        }
        else
            oField.SetWidth( panFieldWidth[nFieldCount] );

        // Skip fields that are only screen labels.
        if( panFieldWidth[nFieldCount] == 0 )
            continue;

        poFeatureDefn->AddFieldDefn( &oField );
        nFieldCount++;
    }

    nRecordLength = panFieldOffset[nFieldCount-1]+panFieldWidth[nFieldCount-1];
    bIsValid = TRUE;

    nStartOfData = VSIFTell( fp );
}

/************************************************************************/
/*                           ~OGRRECLayer()                           */
/************************************************************************/

OGRRECLayer::~OGRRECLayer()

{
    delete poFeatureDefn;
    CPLFree( panFieldOffset );
    CPLFree( panFieldWidth );
}

/************************************************************************/
/*                            ResetReading()                            */
/************************************************************************/

void OGRRECLayer::ResetReading()

{
    VSIFSeek( fpREC, nStartOfData, SEEK_SET );
    nNextFID = 1;
}

/************************************************************************/
/*                      GetNextUnfilteredFeature()                      */
/************************************************************************/

OGRFeature * OGRRECLayer::GetNextUnfilteredFeature()

{
/* -------------------------------------------------------------------- */
/*      Read and assemble the source data record.                       */
/* -------------------------------------------------------------------- */
    int        nDataLen = 0;
    char       *pszRecord = (char *) CPLMalloc(nRecordLength + 2 );

    while( nDataLen < nRecordLength )
    {
        const char *pszLine = CPLReadLine( fpREC );
        int         iSegLen;

        if( pszLine == NULL )
        {
            CPLFree( pszRecord );
            return NULL;
        }

        if( *pszLine == 26 /* Cntl-Z - DOS EOF */ )
        {
            CPLFree( pszRecord );
            return NULL;
        }

        // Strip off end-of-line '!' marker. 
        iSegLen = strlen(pszLine);
        if( pszLine[iSegLen-1] != '!' )
        {
            CPLError( CE_Failure, CPLE_AppDefined, 
                      "Apparent corrupt data line .. record FID=%d", 
                      nNextFID );
            CPLFree( pszRecord );
            return NULL;
        }

        iSegLen--;
        if( nDataLen + iSegLen > nRecordLength )
        {
            CPLError( CE_Failure, CPLE_AppDefined, 
                      "Too much data for record %d.", 
                      nNextFID );
            CPLFree( pszRecord );
            return NULL;
        }

        strncpy( pszRecord+nDataLen, pszLine, iSegLen );
        pszRecord[nDataLen+iSegLen] = '\0';
        nDataLen += iSegLen;
    }

/* -------------------------------------------------------------------- */
/*      Create the OGR feature.                                         */
/* -------------------------------------------------------------------- */
    OGRFeature *poFeature;

    poFeature = new OGRFeature( poFeatureDefn );

/* -------------------------------------------------------------------- */
/*      Set attributes for any indicated attribute records.             */
/* -------------------------------------------------------------------- */
    int         iAttr;
    
    for( iAttr = 0; iAttr < nFieldCount; iAttr++)
    {
        const char *pszFieldText = 
            RECGetField( pszRecord, 
                         panFieldOffset[iAttr] + 1,
                         panFieldWidth[iAttr] );

        if( strlen(pszFieldText) != 0 )
            poFeature->SetField( iAttr, pszFieldText );
    }
    
/* -------------------------------------------------------------------- */
/*      Translate the record id.                                        */
/* -------------------------------------------------------------------- */
    poFeature->SetFID( nNextFID++ );

    return poFeature;
}


/************************************************************************/
/*                           GetNextFeature()                           */
/************************************************************************/

OGRFeature *OGRRECLayer::GetNextFeature()

{
    OGRFeature  *poFeature = NULL;
    
/* -------------------------------------------------------------------- */
/*      Read features till we find one that satisfies our current       */
/*      spatial criteria.                                               */
/* -------------------------------------------------------------------- */
    while( TRUE )
    {
        poFeature = GetNextUnfilteredFeature();
        if( poFeature == NULL )
            break;

        if( /* (poFilterGeom == NULL
               || poFeature->GetGeometryRef() == NULL 
               || poFilterGeom->Intersect( poFeature->GetGeometryRef() ) ) 
            && */ (m_poAttrQuery == NULL
                || m_poAttrQuery->Evaluate( poFeature )) )
            break;

        delete poFeature;
    }

    return poFeature;
}

/************************************************************************/
/*                           TestCapability()                           */
/************************************************************************/

int OGRRECLayer::TestCapability( const char * pszCap )

{
    return FALSE;
}

