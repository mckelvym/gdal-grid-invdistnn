/******************************************************************************
 * $Id$
 *
 * Project:  Erdas Imagine (.img) Translator
 * Purpose:  Implementation of the HFAType class, for managing one type
 *           defined in the HFA data dictionary.  Managed by HFADictionary.
 * Author:   Frank Warmerdam, warmerda@home.com
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
 * Revision 1.5  2001/07/18 04:51:57  warmerda
 * added CPL_CVSID
 *
 * Revision 1.4  2000/12/29 16:37:32  warmerda
 * Use GUInt32 for all file offsets
 *
 * Revision 1.3  2000/09/29 21:42:38  warmerda
 * preliminary write support implemented
 *
 * Revision 1.2  1999/01/22 17:36:47  warmerda
 * Added GetInstBytes(), track unknown sizes properly
 *
 * Revision 1.1  1999/01/04 22:52:10  warmerda
 * New
 *
 */

#include "hfa_p.h"

CPL_CVSID("$Id$");

/************************************************************************/
/* ==================================================================== */
/*      		       HFAType					*/
/* ==================================================================== */
/************************************************************************/

/************************************************************************/
/*                              HFAType()                               */
/************************************************************************/

HFAType::HFAType()

{
    nBytes = 0;
    nFields = 0;
    papoFields = NULL;
    pszTypeName = NULL;
}

/************************************************************************/
/*                              ~HFAType()                              */
/************************************************************************/

HFAType::~HFAType()

{
    int		i;

    for( i = 0; i < nFields; i++ )
    {
        delete papoFields[i];
    }

    CPLFree( papoFields );

    CPLFree( pszTypeName );
}

/************************************************************************/
/*                             Initialize()                             */
/************************************************************************/

const char *HFAType::Initialize( const char * pszInput )

{
    int		i;
    
    if( *pszInput != '{' )
        return NULL;

    pszInput++;

/* -------------------------------------------------------------------- */
/*      Read the field definitions.                                     */
/* -------------------------------------------------------------------- */
    while( pszInput != NULL && *pszInput != '}' )
    {
        HFAField	*poNewField = new HFAField();

        pszInput = poNewField->Initialize( pszInput );
        if( pszInput != NULL )
        {
            papoFields = (HFAField **)
                CPLRealloc(papoFields, sizeof(void*) * (nFields+1) );
            papoFields[nFields++] = poNewField;
        }
        else
            delete poNewField;
    }

    if( pszInput == NULL )
        return NULL;

/* -------------------------------------------------------------------- */
/*      Collect the name.                                               */
/* -------------------------------------------------------------------- */
    pszInput++; /* skip `}' */

    for( i = 0; pszInput[i] != '\0' && pszInput[i] != ','; i++ ) {}

    pszTypeName = (char *) CPLMalloc(i+1);
    strncpy( pszTypeName, pszInput, i );
    pszTypeName[i] = '\0';
    
    pszInput += i+1;

    return( pszInput );
}

/************************************************************************/
/*                            CompleteDefn()                            */
/************************************************************************/

void HFAType::CompleteDefn( HFADictionary * poDict )

{
    int		i;

/* -------------------------------------------------------------------- */
/*      This may already be done, if an earlier object required this    */
/*      object (as a field), and forced an early computation of the     */
/*      size.                                                           */
/* -------------------------------------------------------------------- */
    if( nBytes != 0 )
        return;
    
/* -------------------------------------------------------------------- */
/*      Complete each of the fields, totaling up the sizes.  This       */
/*      isn't really accurate for object with variable sized            */
/*      subobjects.                                                     */
/* -------------------------------------------------------------------- */
    for( i = 0; i < nFields; i++ )
    {
        papoFields[i]->CompleteDefn( poDict );
        if( papoFields[i]->nBytes < 0 || nBytes == -1 )
            nBytes = -1;
        else
            nBytes += papoFields[i]->nBytes;
    }
}

/************************************************************************/
/*                                Dump()                                */
/************************************************************************/

void HFAType::Dump( FILE * fp )

{
    int		i;
    
    VSIFPrintf( fp, "HFAType %s/%d bytes\n", pszTypeName, nBytes );

    for( i = 0; i < nFields; i++ )
    {
        papoFields[i]->Dump( fp );
    }

    VSIFPrintf( fp, "\n" );
}

/************************************************************************/
/*                            SetInstValue()                            */
/************************************************************************/

CPLErr 
HFAType::SetInstValue( const char * pszFieldPath,
                       GByte *pabyData, GUInt32 nDataOffset, int nDataSize,
                       char chReqType, void *pValue )

{
    int		nArrayIndex = 0, nNameLen, iField, nByteOffset;
    const char	*pszRemainder;

/* -------------------------------------------------------------------- */
/*      Parse end of field name, possible index value and               */
/*      establish where the remaining fields (if any) would start.      */
/* -------------------------------------------------------------------- */
    if( strchr(pszFieldPath,'[') != NULL )
    {
        const char	*pszEnd = strchr(pszFieldPath,'[');
        
        nArrayIndex = atoi(pszEnd+1);
        nNameLen = pszEnd - pszFieldPath;

        pszRemainder = strchr(pszFieldPath,'.');
        if( pszRemainder != NULL )
            pszRemainder++;
    }

    else if( strchr(pszFieldPath,'.') != NULL )
    {
        const char	*pszEnd = strchr(pszFieldPath,'.');
        
        nNameLen = pszEnd - pszFieldPath;

        pszRemainder = pszEnd + 1;
    }

    else
    {
        nNameLen = strlen(pszFieldPath);
        pszRemainder = NULL;
    }
    
/* -------------------------------------------------------------------- */
/*      Find this field within this type, if possible.                  */
/* -------------------------------------------------------------------- */
    nByteOffset = 0;
    for( iField = 0; iField < nFields; iField++ )
    {
        if( EQUALN(pszFieldPath,papoFields[iField]->pszFieldName,nNameLen)
            && papoFields[iField]->pszFieldName[nNameLen] == '\0' )
        {
            break;
        }

        nByteOffset += papoFields[iField]->GetInstBytes(pabyData+nByteOffset);
    }

    if( iField == nFields )
        return CE_Failure;

/* -------------------------------------------------------------------- */
/*      Extract this field value, and return.                           */
/* -------------------------------------------------------------------- */
    return( papoFields[iField]->SetInstValue( pszRemainder, nArrayIndex,
                                              pabyData + nByteOffset,
                                              nDataOffset + nByteOffset,
                                              nDataSize - nByteOffset,
                                              chReqType, pValue ) );
}

/************************************************************************/
/*                          ExtractInstValue()                          */
/*                                                                      */
/*      Extract the value of a field instance within this type.         */
/*      Most of the work is done by the ExtractInstValue() for the      */
/*      HFAField, but this methond does the field name parsing.         */
/*                                                                      */
/*      field names have the form:                                      */
/*                                                                      */
/*        fieldname{[index]}{.fieldname...}                             */
/*                                                                      */
/*      eg.                                                             */
/*        abc                                   - field abc[0]          */
/*        abc[3]                                - field abc[3]          */
/*        abc[2].def                            - field def[0] of       */
/*                                                the third abc struct. */
/************************************************************************/

void *
HFAType::ExtractInstValue( const char * pszFieldPath,
                           GByte *pabyData, GUInt32 nDataOffset, int nDataSize,
                           char chReqType )

{
    int		nArrayIndex = 0, nNameLen, iField, nByteOffset;
    const char	*pszRemainder;

/* -------------------------------------------------------------------- */
/*      Parse end of field name, possible index value and               */
/*      establish where the remaining fields (if any) would start.      */
/* -------------------------------------------------------------------- */
    if( strchr(pszFieldPath,'[') != NULL )
    {
        const char	*pszEnd = strchr(pszFieldPath,'[');
        
        nArrayIndex = atoi(pszEnd+1);
        nNameLen = pszEnd - pszFieldPath;

        pszRemainder = strchr(pszFieldPath,'.');
        if( pszRemainder != NULL )
            pszRemainder++;
    }

    else if( strchr(pszFieldPath,'.') != NULL )
    {
        const char	*pszEnd = strchr(pszFieldPath,'.');
        
        nNameLen = pszEnd - pszFieldPath;

        pszRemainder = pszEnd + 1;
    }

    else
    {
        nNameLen = strlen(pszFieldPath);
        pszRemainder = NULL;
    }
    
/* -------------------------------------------------------------------- */
/*      Find this field within this type, if possible.                  */
/* -------------------------------------------------------------------- */
    nByteOffset = 0;
    for( iField = 0; iField < nFields; iField++ )
    {
        if( EQUALN(pszFieldPath,papoFields[iField]->pszFieldName,nNameLen)
            && papoFields[iField]->pszFieldName[nNameLen] == '\0' )
        {
            break;
        }

        nByteOffset += papoFields[iField]->GetInstBytes(pabyData+nByteOffset);
    }

    if( iField == nFields )
        return NULL;

/* -------------------------------------------------------------------- */
/*      Extract this field value, and return.                           */
/* -------------------------------------------------------------------- */
    return( papoFields[iField]->
            ExtractInstValue( pszRemainder, nArrayIndex,
                              pabyData + nByteOffset,
                              nDataOffset + nByteOffset,
                              nDataSize - nByteOffset,
                              chReqType ) );
}


/************************************************************************/
/*                           DumpInstValue()                            */
/************************************************************************/

void HFAType::DumpInstValue( FILE * fpOut,
                           GByte *pabyData, GUInt32 nDataOffset, int nDataSize,
                           const char * pszPrefix )

{
    int		iField;
    
    for( iField = 0; iField < nFields; iField++ )
    {
        HFAField	*poField = papoFields[iField];
        int		nInstBytes;
        
        poField->DumpInstValue( fpOut, pabyData, nDataOffset,
                                nDataSize, pszPrefix );

        nInstBytes = poField->GetInstBytes( pabyData );

        pabyData += nInstBytes;
        nDataOffset += nInstBytes;
        nDataSize -= nInstBytes;
    }    
}

/************************************************************************/
/*                            GetInstBytes()                            */
/*                                                                      */
/*      How many bytes in this particular instance of this type?        */
/************************************************************************/

int HFAType::GetInstBytes( GByte * pabyData )

{
    if( nBytes >= 0 )
        return( nBytes );
    else
    {
        int	nTotal = 0;
        int	iField;
    
        for( iField = 0; iField < nFields; iField++ )
        {
            HFAField	*poField = papoFields[iField];
            int		nInstBytes;

            nInstBytes = poField->GetInstBytes( pabyData );

            pabyData += nInstBytes;
            nTotal += nInstBytes;
        }

        return( nTotal );
    }
}
