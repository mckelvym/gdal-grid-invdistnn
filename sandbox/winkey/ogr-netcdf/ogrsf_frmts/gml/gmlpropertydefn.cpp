/**********************************************************************
 * $Id$
 *
 * Project:  GML Reader
 * Purpose:  Implementation of GMLPropertyDefn
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#include "gmlreader.h"
#include "cpl_conv.h"
#include "cpl_string.h"

/************************************************************************/
/*                           GMLPropertyDefn                            */
/************************************************************************/

GMLPropertyDefn::GMLPropertyDefn( const char *pszName, 
                                  const char *pszSrcElement )

{
    m_pszName = CPLStrdup( pszName );
    if( pszSrcElement != NULL )
        m_pszSrcElement = CPLStrdup( pszSrcElement );
    else
        m_pszSrcElement = NULL;
    m_eType = GMLPT_Untyped;
    m_nWidth = 0; 
    m_nPrecision = 0;
    m_nIndex = -1;
}

/************************************************************************/
/*                          ~GMLPropertyDefn()                          */
/************************************************************************/

GMLPropertyDefn::~GMLPropertyDefn()

{
    CPLFree( m_pszName );
    CPLFree( m_pszSrcElement );
}

/************************************************************************/
/*                           SetSrcElement()                            */
/************************************************************************/

void GMLPropertyDefn::SetSrcElement( const char *pszSrcElement )

{
    CPLFree( m_pszSrcElement );
    if( pszSrcElement != NULL )
        m_pszSrcElement = CPLStrdup( pszSrcElement );
    else
        m_pszSrcElement = NULL;
}

/************************************************************************/
/*                        AnalysePropertyValue()                        */
/*                                                                      */
/*      Examine the passed property value, and see if we need to        */
/*      make the field type more specific, or more general.             */
/************************************************************************/

void GMLPropertyDefn::AnalysePropertyValue( const GMLProperty* psGMLProperty )

{
/* -------------------------------------------------------------------- */
/*      Does the string consist entirely of numeric values?             */
/* -------------------------------------------------------------------- */
    int bIsReal = FALSE;

    int j;
    for(j=0;j<psGMLProperty->nSubProperties;j++)
    {
        if (j > 0)
        {
            if( m_eType == GMLPT_Integer )
                m_eType = GMLPT_IntegerList;
            else if( m_eType == GMLPT_Real )
                m_eType = GMLPT_RealList;
            else if( m_eType == GMLPT_String )
            {
                m_eType = GMLPT_StringList;
                m_nWidth = 0;
            }
        }
        const char* pszValue = psGMLProperty->papszSubProperties[j];
/* -------------------------------------------------------------------- */
/*      If it is a zero length string, just return.  We can't deduce    */
/*      much from this.                                                 */
/* -------------------------------------------------------------------- */
        if( *pszValue == '\0' )
            continue;

        CPLValueType valueType = CPLGetValueType(pszValue);
        if (valueType == CPL_VALUE_STRING
            && m_eType != GMLPT_String 
            && m_eType != GMLPT_StringList )
        {
            if( m_eType == GMLPT_IntegerList
                || m_eType == GMLPT_RealList )
                m_eType = GMLPT_StringList;
            else
                m_eType = GMLPT_String;
        }
        else
            bIsReal = (valueType == CPL_VALUE_REAL);
    
        if( m_eType == GMLPT_String )
        {
            /* grow the Width to the length of the string passed in */
            int nWidth;
            nWidth = strlen(pszValue);
            if ( m_nWidth < nWidth ) 
                SetWidth( nWidth );
        }
        else if( m_eType == GMLPT_Untyped || m_eType == GMLPT_Integer )
        {
            if( bIsReal )
                m_eType = GMLPT_Real;
            else
                m_eType = GMLPT_Integer;
        }
        else if( m_eType == GMLPT_IntegerList && bIsReal )
        {
            m_eType = GMLPT_RealList;
        }
    }
}
