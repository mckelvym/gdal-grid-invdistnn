/******************************************************************************
 * $Id$
 *
 * Project:  KML Driver
 * Purpose:  Implementation of KMLFeature class.
 * Author:   Christopher Condit, condit@sdsc.edu
 *
 ******************************************************************************
 * Copyright (c) 2006, Christopher Condit
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
 * Revision 1.2  2006/07/27 19:53:01  mloskot
 * Added common file header to KML driver source files.
 *
 *
 */
#include "kmlreader.h"
#include "cpl_conv.h"
#include "cpl_error.h"

/************************************************************************/
/*                             KMLFeature()                             */
/************************************************************************/
KMLFeature::KMLFeature( KMLFeatureClass *poClass )
{
    CPLAssert( NULL != poClass );

    m_poClass = poClass;
    m_pszFID = NULL;
    m_pszGeometry = NULL;
    
    m_nPropertyCount = 0;
    m_papszProperty = NULL;
}

/************************************************************************/
/*                            ~KMLFeature()                             */
/************************************************************************/
KMLFeature::~KMLFeature()
{
    CPLFree( m_pszFID );    
    
    CPLFree( m_pszGeometry );
}

/************************************************************************/
/*                               SetFID()                               */
/************************************************************************/
void KMLFeature::SetFID( const char *pszFID )
{
    CPLFree( m_pszFID );
    if( pszFID != NULL )
        m_pszFID = CPLStrdup( pszFID );
    else
        m_pszFID = NULL;
}

/************************************************************************/
/*                                Dump()                                */
/************************************************************************/
void KMLFeature::Dump( FILE * fp )
{
    printf( "KMLFeature(%s):\n", m_poClass->GetName() );
    
    if( m_pszFID != NULL )
        printf( "  FID = %s\n", m_pszFID );       

    if( m_pszGeometry )
        printf( "  %s\n", m_pszGeometry );
}

/************************************************************************/
/*                        SetGeometryDirectly()                         */
/************************************************************************/
void KMLFeature::SetGeometryDirectly( char *pszGeometry )
{
    CPLAssert( NULL != pszGeometry );

    // Free current geometry instance
    CPLFree( m_pszGeometry );

    // Assign passed geometry directly, not a copy
    m_pszGeometry = pszGeometry;
}
