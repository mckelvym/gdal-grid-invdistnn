/******************************************************************************
 * $Id$
 *
 * Project:  GML Reader
 * Purpose:  Public Declarations for OGR free GML Reader code.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
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
 ******************************************************************************
 *
 * $Log$
 * Revision 1.5  2002/01/25 21:23:21  warmerda
 * handle IGMLReader destructor properly in gmlreader.cpp
 *
 * Revision 1.4  2002/01/25 21:17:34  warmerda
 * provided IGMLReader destructor implementation
 *
 * Revision 1.3  2002/01/25 20:38:01  warmerda
 * added prescan and resetreading methods
 *
 * Revision 1.2  2002/01/24 17:39:08  warmerda
 * added xml serialization and geometry support
 *
 * Revision 1.1  2002/01/04 19:46:30  warmerda
 * New
 */

#ifndef _GMLREADER_H_INCLUDED
#define _GMLREADER_H_INCLUDED

#include "cpl_port.h"
#include "cpl_minixml.h"

typedef enum {
    GMLPT_Untyped = 0,
    GMLPT_String = 1,
    GMLPT_Integer = 2,
    GMLPT_Real = 3,
    GMLPT_Complex = 4
} GMLPropertyType;

/************************************************************************/
/*                           GMLPropertyDefn                            */
/************************************************************************/
class CPL_DLL GMLPropertyDefn
{
    char	     *m_pszName;
    GMLPropertyType   m_eType;
    char             *m_pszSrcElement;

public:
    
    	GMLPropertyDefn( const char *pszName, const char *pszSrcElement=NULL );
       ~GMLPropertyDefn();

    const char *GetName() { return m_pszName; } const

    GMLPropertyType GetType() { return m_eType; } const
    void        SetType( GMLPropertyType eType ) { m_eType = eType; }

    void        SetSrcElement( const char *pszSrcElement );
    const char *GetSrcElement() { return m_pszSrcElement; }
};

/************************************************************************/
/*                           GMLFeatureClass                            */
/************************************************************************/
class CPL_DLL GMLFeatureClass
{
    char        *m_pszName;
    char        *m_pszElementName;
    char	*m_pszGeometryElement;
    int          m_nPropertyCount;
    GMLPropertyDefn **m_papoProperty;

    int          m_bSchemaLocked;

public:
	    GMLFeatureClass( const char *pszName = "" );
           ~GMLFeatureClass();

    const char *GetElementName() const;
    void        SetElementName( const char *pszElementName );

    const char *GetGeometryElement() const { return m_pszGeometryElement; }
    void        SetGeometryElement( const char *pszElementName );

    const char *GetName() { return m_pszName; } const
    int         GetPropertyCount() const { return m_nPropertyCount; }
    GMLPropertyDefn *GetProperty( int iIndex ) const;
    int GetPropertyIndex( const char *pszName ) const;
    GMLPropertyDefn *GetProperty( const char *pszName ) const 
        { return GetProperty( GetPropertyIndex(pszName) ); }

    int         AddProperty( GMLPropertyDefn * );

    int         IsSchemaLocked() const { return m_bSchemaLocked; }
    void        SetSchemaLocked( int bLock ) { m_bSchemaLocked = bLock; }

    CPLXMLNode *SerializeToXML();
    int         InitializeFromXML( CPLXMLNode * );
};

/************************************************************************/
/*                              GMLFeature                              */
/************************************************************************/
class CPL_DLL GMLFeature
{
    GMLFeatureClass *m_poClass;
    char            *m_pszFID;

    int		     m_nPropertyCount;
    char           **m_papszProperty;

    char	    *m_pszGeometry;

public:
		    GMLFeature( GMLFeatureClass * );
	           ~GMLFeature();

    GMLFeatureClass*GetClass() const { return m_poClass; }

    void	    SetGeometryDirectly( char * );
    const char     *GetGeometry() const { return m_pszGeometry; }

    void            SetProperty( int i, const char *pszValue );
    void            SetProperty( const char *pszName, const char *pszValue )
        { SetProperty( m_poClass->GetPropertyIndex(pszName), pszValue ); }

    const char      *GetProperty( int i ) const;
    const char      *GetProperty( const char *pszName ) const
        { return GetProperty( m_poClass->GetPropertyIndex(pszName) ); }

    const char      *GetFID() const { return m_pszFID; }
    void             SetFID( const char *pszFID );

    void	     Dump( FILE *fp );
};

/************************************************************************/
/*                              IGMLReader                              */
/************************************************************************/
class CPL_DLL IGMLReader
{
public:
    virtual 	~IGMLReader();

    virtual int  IsClassListLocked() const = 0;
    virtual void SetClassListLocked( int bFlag ) = 0;

    virtual void SetSourceFile( const char *pszFilename ) = 0;

    virtual int	 GetClassCount() const = 0;
    virtual GMLFeatureClass *GetClass( int i ) const = 0;
    virtual GMLFeatureClass *GetClass( const char *pszName ) const = 0;

    virtual int        AddClass( GMLFeatureClass *poClass ) = 0;
    virtual void       ClearClasses() = 0;

    virtual GMLFeature *NextFeature() = 0;
    virtual void       ResetReading() = 0;

    virtual int  LoadClasses( const char *pszFile = NULL ) = 0;
    virtual int  SaveClasses( const char *pszFile = NULL ) = 0;

    virtual int PrescanForSchema() = 0;
};

IGMLReader *CreateGMLReader();


#endif /* _GMLREADER_H_INCLUDED */
