/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  OGRSpatialReference interface to OGC XML (014r4).
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2001, Frank Warmerdam (warmerdam@pobox.com)
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
 * Revision 1.7  2005/05/11 14:37:25  fwarmerdam
 * add id on projectedcrs
 *
 * Revision 1.6  2005/01/17 17:04:24  fwarmerdam
 * major overhaul to more-or-less GML 3 CRS support
 *
 * Revision 1.5  2003/05/21 04:49:17  warmerda
 * avoid warnings
 *
 * Revision 1.4  2003/03/28 17:43:52  warmerda
 * rewrote to use new URN and projections proposal
 *
 * Revision 1.3  2003/03/21 22:14:43  warmerda
 * first pass re-implementation using GML 3 schemas
 *
 * Revision 1.2  2002/04/18 14:22:45  warmerda
 * made OGRSpatialReference and co 'const correct'
 *
 * Revision 1.1  2001/12/06 18:16:17  warmerda
 * new
 *
 */

#include "ogr_spatialref.h"
#include "ogr_p.h"
#include "cpl_minixml.h"

/************************************************************************/
/*                              parseURN()                              */
/*                                                                      */
/*      Parses requested sections out of URN.  The passed in URN        */
/*      *is* altered but the returned values point into the             */
/*      original string.                                                */
/************************************************************************/

static int parseURN( char *pszURN, 
                     char **ppszObjectType, 
                     char **ppszAuthority, 
                     char **ppszCode,
                     char **ppszVersion = NULL )

{
    int  i;

    if( ppszObjectType != NULL )
        *ppszObjectType = "";
    if( ppszAuthority != NULL )
        *ppszAuthority = "";
    if( ppszCode != NULL )
        *ppszCode = "";
    if( ppszVersion != NULL )
        *ppszVersion = "";

/* -------------------------------------------------------------------- */
/*      Verify prefix.                                                  */
/* -------------------------------------------------------------------- */
    if( !EQUALN(pszURN,"urn:ogc:def:",12) )
        return FALSE;

/* -------------------------------------------------------------------- */
/*      Extract object type                                             */
/* -------------------------------------------------------------------- */
    if( ppszObjectType != NULL )
        *ppszObjectType = (char *) pszURN + 12;

    i = 12;
    while( pszURN[i] != ':' && pszURN[i] != '\0' )
        i++;

    if( pszURN[i] == '\0' )
        return FALSE;

    pszURN[i] = '\0';
    i++;

/* -------------------------------------------------------------------- */
/*      Extract authority                                               */
/* -------------------------------------------------------------------- */
    if( ppszAuthority != NULL )
        *ppszAuthority = (char *) pszURN + i;

    while( pszURN[i] != ':' && pszURN[i] != '\0' )
        i++;

    if( pszURN[i] == '\0' )
        return FALSE;

    pszURN[i] = '\0';
    i++;

/* -------------------------------------------------------------------- */
/*      Extract version                                                 */
/* -------------------------------------------------------------------- */
    if( ppszVersion != NULL )
        *ppszVersion = (char *) pszURN + i;

    while( pszURN[i] != ':' && pszURN[i] != '\0' )
        i++;

    if( pszURN[i] == '\0' )
        return FALSE;

    pszURN[i] = '\0';
    i++;

/* -------------------------------------------------------------------- */
/*      Extract code.                                                   */
/* -------------------------------------------------------------------- */
    if( ppszCode != NULL )
        *ppszCode = (char *) pszURN + i;
    
    return TRUE;
}

/************************************************************************/
/*                               addURN()                               */
/************************************************************************/

static void addURN( CPLXMLNode *psTarget, 
                    const char *pszAuthority, 
                    const char *pszObjectType, 
                    int nCode,
                    const char *pszVersion = "" )

{
    char szURN[200];

    if( pszVersion == NULL )
        pszVersion = "";

    CPLAssert( strlen(pszAuthority)+strlen(pszObjectType) < sizeof(szURN)-30 );

    sprintf( szURN, "urn:ogc:def:%s:%s:%s:", 
             pszObjectType, pszAuthority, pszVersion );
    
    if( nCode != 0 )
        sprintf( szURN + strlen(szURN), "%d", nCode );
    
    CPLCreateXMLNode(
        CPLCreateXMLNode( psTarget, CXT_Attribute, "xlink:href" ),
        CXT_Text, szURN );
}

/************************************************************************/
/*                         AddValueIDWithURN()                          */
/*                                                                      */
/*      Adds element of the form <ElementName                           */
/*      xlink:href="urn_without_id">id</ElementName>"                   */
/************************************************************************/

static CPLXMLNode *
AddValueIDWithURN( CPLXMLNode *psTarget, 
                   const char *pszElement,
                   const char *pszAuthority, 
                   const char *pszObjectType, 
                   int nCode,
                   const char *pszVersion = "" )
    
{
    char szCode[128];
    CPLXMLNode *psElement;

    sprintf( szCode, "%d", nCode );

    psElement = CPLCreateXMLElementAndValue( psTarget, pszElement, szCode );
    addURN( psElement, pszAuthority, pszObjectType, 0, pszVersion );

    return psElement;
}

/************************************************************************/
/*                          addAuthorityIDBlock()                          */
/*                                                                      */
/*      Creates a structure like:                                       */
/*      <srsId>                                                         */
/*        <name codeSpace="urn">code</name>                             */
/*      </srsId>                                                        */
/************************************************************************/
static CPLXMLNode *addAuthorityIDBlock( CPLXMLNode *psTarget, 
                                     const char *pszElement,
                                     const char *pszAuthority, 
                                     const char *pszObjectType, 
                                     int nCode,
                                     const char *pszVersion = "" )

{
    char szURN[200];

/* -------------------------------------------------------------------- */
/*      Prepare partial URN without the actual code.                    */
/* -------------------------------------------------------------------- */
    if( pszVersion == NULL )
        pszVersion = "";

    CPLAssert( strlen(pszAuthority)+strlen(pszObjectType) < sizeof(szURN)-30 );

    sprintf( szURN, "urn:ogc:def:%s:%s:%s:", 
             pszObjectType, pszAuthority, pszVersion );
    
/* -------------------------------------------------------------------- */
/*      Prepare the base name, eg. <srsID>.                             */
/* -------------------------------------------------------------------- */
    CPLXMLNode *psElement = 
        CPLCreateXMLNode( psTarget, CXT_Element, pszElement );

/* -------------------------------------------------------------------- */
/*      Prepare the name element.                                       */
/* -------------------------------------------------------------------- */
    CPLXMLNode * psName = 
        CPLCreateXMLNode( psElement, CXT_Element, "gml:name" );

/* -------------------------------------------------------------------- */
/*      Prepare the codespace attribute.                                */
/* -------------------------------------------------------------------- */
    CPLCreateXMLNode(
        CPLCreateXMLNode( psName, CXT_Attribute, "gml:codeSpace" ),
        CXT_Text, szURN );

/* -------------------------------------------------------------------- */
/*      Attach code value to name node.                                 */
/* -------------------------------------------------------------------- */
    char szCode[32];
    sprintf( szCode, "%d", nCode );

    CPLCreateXMLNode( psName, CXT_Text, szCode );

    return psElement;
}
    

/************************************************************************/
/*                              addGMLId()                              */
/************************************************************************/

static void addGMLId( CPLXMLNode *psParent )

{
    CPLXMLNode *psId;
    static int nNextGMLId = 1;
    char   szIdText[40];

    sprintf( szIdText, "ogrcrs%d", nNextGMLId++ );
    
    psId = 
        CPLCreateXMLNode( 
            CPLCreateXMLNode( psParent, CXT_Attribute, "gml:id" ),
            CXT_Text, szIdText );
}

/************************************************************************/
/*                        exportAuthorityToXML()                        */
/************************************************************************/

static CPLXMLNode *exportAuthorityToXML( const OGR_SRSNode *poAuthParent,
                                         const char *pszTagName,
                                         CPLXMLNode *psXMLParent,
                                         const char *pszObjectType,
                                         int bUseSubName = TRUE )

{
    const OGR_SRSNode *poAuthority;

/* -------------------------------------------------------------------- */
/*      Get authority node from parent.                                 */
/* -------------------------------------------------------------------- */
    if( poAuthParent->FindChild( "AUTHORITY" ) == -1 )
        return NULL;

    poAuthority = poAuthParent->GetChild( 
        poAuthParent->FindChild( "AUTHORITY" ));

/* -------------------------------------------------------------------- */
/*      Create identification.                                          */
/* -------------------------------------------------------------------- */
    const char *pszCode, *pszCodeSpace, *pszEdition;

    pszCode = poAuthority->GetChild(1)->GetValue();
    pszCodeSpace = poAuthority->GetChild(0)->GetValue();
    pszEdition = NULL;

    if( bUseSubName )
        return addAuthorityIDBlock( psXMLParent, pszTagName, pszCodeSpace, 
                                 pszObjectType, atoi(pszCode), pszEdition );
    else
        return AddValueIDWithURN( psXMLParent, pszTagName, pszCodeSpace, 
                                  pszObjectType, atoi(pszCode), pszEdition );
                              
}

/************************************************************************/
/*                             addProjArg()                             */
/************************************************************************/

static void addProjArg( const OGRSpatialReference *poSRS, CPLXMLNode *psBase, 
                        const char *pszMeasureType, double dfDefault,
                        int nParameterID, const char *pszWKTName )

{
    CPLXMLNode *psNode, *psValue;

    psNode = CPLCreateXMLNode( psBase, CXT_Element, "gml:usesParameterValue" );

/* -------------------------------------------------------------------- */
/*      Handle the UOM.                                                 */
/* -------------------------------------------------------------------- */
    const char *pszUOMValue;

    if( EQUAL(pszMeasureType,"Angular") )
        pszUOMValue = "urn:ogc:def:uom:EPSG::9102";
    else
        pszUOMValue = "urn:ogc:def:uom:EPSG::9001";

    psValue = CPLCreateXMLNode( psNode, CXT_Element, "gml:value" );

    CPLCreateXMLNode( 
        CPLCreateXMLNode( psValue, CXT_Attribute, "gml:uom" ),
        CXT_Text, pszUOMValue );
    
/* -------------------------------------------------------------------- */
/*      Add the parameter value itself.                                 */
/* -------------------------------------------------------------------- */
    double dfParmValue
        = poSRS->GetNormProjParm( pszWKTName, dfDefault, NULL );
        
    CPLCreateXMLNode( psValue, CXT_Text, 
                      CPLSPrintf( "%.16g", dfParmValue ) );

/* -------------------------------------------------------------------- */
/*      Add the valueOfParameter.                                       */
/* -------------------------------------------------------------------- */
    AddValueIDWithURN( psNode, "gml:valueOfParameter", "EPSG", "parameter", 
                       nParameterID );
}

/************************************************************************/
/*                              addAxis()                               */
/*                                                                      */
/*      Added the <usesAxis> element and down.                          */
/************************************************************************/

static CPLXMLNode *addAxis( CPLXMLNode *psXMLParent, 
                            const char *pszAxis, // "Lat", "Long", "E" or "N"
                            const OGR_SRSNode * /* poUnitsSrc */ )

{
    CPLXMLNode *psAxisXML;

    psAxisXML = 
        CPLCreateXMLNode( 
            CPLCreateXMLNode( psXMLParent, CXT_Element, "gml:usesAxis" ),
            CXT_Element, "gml:CoordinateSystemAxis" );
    addGMLId( psAxisXML );

    if( EQUAL(pszAxis,"Lat") )
    {
        CPLCreateXMLNode( 
            CPLCreateXMLNode( psAxisXML, CXT_Attribute, "gml:uom" ),
            CXT_Text, "urn:ogc:def:uom:EPSG::9102" );

        CPLCreateXMLElementAndValue( psAxisXML, "gml:name",
                                     "Geodetic latitude" );
        addAuthorityIDBlock( psAxisXML, "gml:axisID", "EPSG", "axis", 9901 );
        CPLCreateXMLElementAndValue( psAxisXML, "gml:axisAbbrev", "Lat" );
        CPLCreateXMLElementAndValue( psAxisXML, "gml:axisDirection", "north" );
    }
    else if( EQUAL(pszAxis,"Long") )
    {
        CPLCreateXMLNode( 
            CPLCreateXMLNode( psAxisXML, CXT_Attribute, "gml:uom" ),
            CXT_Text, "urn:ogc:def:uom:EPSG::9102" );

        CPLCreateXMLElementAndValue( psAxisXML, "gml:name",
                                     "Geodetic longitude" );
        addAuthorityIDBlock( psAxisXML, "gml:axisID", "EPSG", "axis", 9902 );
        CPLCreateXMLElementAndValue( psAxisXML, "gml:axisAbbrev", "Lon" );
        CPLCreateXMLElementAndValue( psAxisXML, "gml:axisDirection", "east" );
    }
    else if( EQUAL(pszAxis,"E") )
    {
        CPLCreateXMLNode( 
            CPLCreateXMLNode( psAxisXML, CXT_Attribute, "gml:uom" ),
            CXT_Text, "urn:ogc:def:uom:EPSG::9001" );

        CPLCreateXMLElementAndValue( psAxisXML, "gml:name", "Easting" );
        addAuthorityIDBlock( psAxisXML, "gml:axisID", "EPSG", "axis", 9906 );
        CPLCreateXMLElementAndValue( psAxisXML, "gml:axisAbbrev", "E" );
        CPLCreateXMLElementAndValue( psAxisXML, "gml:axisDirection", "east" );
    }
    else if( EQUAL(pszAxis,"N") )
    {
        CPLCreateXMLNode( 
            CPLCreateXMLNode( psAxisXML, CXT_Attribute, "gml:uom" ),
            CXT_Text, "urn:ogc:def:uom:EPSG::9001" );

        CPLCreateXMLElementAndValue( psAxisXML, "gml:name", "Northing" );
        addAuthorityIDBlock( psAxisXML, "gml:axisID", "EPSG", "axis", 9907 );
        CPLCreateXMLElementAndValue( psAxisXML, "gml:axisAbbrev", "N" );
        CPLCreateXMLElementAndValue( psAxisXML, "gml:axisDirection", "north" );
    }
    else
    {
        CPLAssert( FALSE );
    }

    return psAxisXML;
}

/************************************************************************/
/*                         exportGeogCSToXML()                          */
/************************************************************************/

static CPLXMLNode *exportGeogCSToXML( const OGRSpatialReference *poSRS )

{
    CPLXMLNode  *psGCS_XML;
    const OGR_SRSNode *poGeogCS = poSRS->GetAttrNode( "GEOGCS" );

    if( poGeogCS == NULL )
        return NULL;
    
/* -------------------------------------------------------------------- */
/*      Establish initial infrastructure.                               */
/* -------------------------------------------------------------------- */
    psGCS_XML = CPLCreateXMLNode( NULL, CXT_Element, "gml:GeographicCRS" );
    addGMLId( psGCS_XML );
    
/* -------------------------------------------------------------------- */
/*      Attach symbolic name (srsName).                                 */
/* -------------------------------------------------------------------- */
    CPLCreateXMLElementAndValue( psGCS_XML, "gml:srsName", 
                                 poGeogCS->GetChild(0)->GetValue() );

/* -------------------------------------------------------------------- */
/*      Does the overall coordinate system have an authority?  If so    */
/*      attach as an identification section.                            */
/* -------------------------------------------------------------------- */
    exportAuthorityToXML( poGeogCS, "gml:srsID", psGCS_XML, "crs" );

/* -------------------------------------------------------------------- */
/*      Insert a big whack of fixed stuff defining the                  */
/*      ellipsoidalCS.  Basically this defines the axes and their       */
/*      units.                                                          */
/* -------------------------------------------------------------------- */
    CPLXMLNode *psECS;

    psECS = CPLCreateXMLNode( 
        CPLCreateXMLNode( psGCS_XML, CXT_Element, "gml:usesEllipsoidalCS" ),
        CXT_Element, "gml:EllipsoidalCS" );

    addGMLId( psECS  );

    CPLCreateXMLElementAndValue( psECS, "gml:csName", "ellipsoidal" );

    addAuthorityIDBlock( psECS, "gml:csID", "EPSG", "cs", 6402 );

    addAxis( psECS, "Lat", NULL );
    addAxis( psECS, "Long", NULL );

/* -------------------------------------------------------------------- */
/*      Start with the datum.                                           */
/* -------------------------------------------------------------------- */
    const OGR_SRSNode    *poDatum = poGeogCS->GetNode( "DATUM" );
    CPLXMLNode     *psDatumXML;

    if( poDatum == NULL )
    {
        CPLDestroyXMLNode( psGCS_XML );
        return NULL;
    }

    psDatumXML = CPLCreateXMLNode( 
        CPLCreateXMLNode( psGCS_XML, CXT_Element, "gml:usesGeodeticDatum" ),
        CXT_Element, "gml:GeodeticDatum" );
    
    addGMLId( psDatumXML );

/* -------------------------------------------------------------------- */
/*      Set the datumName.                                              */
/* -------------------------------------------------------------------- */
    CPLCreateXMLElementAndValue( psDatumXML, "gml:datumName", 
                                 poDatum->GetChild(0)->GetValue() );

/* -------------------------------------------------------------------- */
/*      Set authority id info if available.                             */
/* -------------------------------------------------------------------- */
    exportAuthorityToXML( poDatum, "gml:datumID", psDatumXML, "datum" );

/* -------------------------------------------------------------------- */
/*      Setup prime meridian information.                               */
/* -------------------------------------------------------------------- */
    const OGR_SRSNode *poPMNode = poGeogCS->GetNode( "PRIMEM" );
    CPLXMLNode *psPM;
    char *pszPMName = "Greenwich";
    double dfPMOffset = poSRS->GetPrimeMeridian( &pszPMName );

    psPM = CPLCreateXMLNode( 
        CPLCreateXMLNode( psDatumXML, CXT_Element, "gml:usesPrimeMeridian" ),
        CXT_Element, "gml:PrimeMeridian" );

    addGMLId( psPM );

    CPLCreateXMLElementAndValue( psPM, "gml:meridianName", pszPMName );

    if( poPMNode )
        exportAuthorityToXML( poPMNode, "gml:meridianID", psPM, "meridian" );

    CPLXMLNode *psAngle;

    psAngle = 
        CPLCreateXMLNode( 
            CPLCreateXMLNode( psPM, CXT_Element, "gml:greenwichLongitude" ),
            CXT_Element, "gml:angle" );
    
    CPLCreateXMLNode( CPLCreateXMLNode( psAngle, CXT_Attribute, "gml:uom" ),
                      CXT_Text, "urn:ogc:def:uom:EPSG::9102" );

    CPLCreateXMLNode( psAngle, CXT_Text, CPLSPrintf( "%.16g", dfPMOffset ) );

/* -------------------------------------------------------------------- */
/*      Translate the ellipsoid.                                        */
/* -------------------------------------------------------------------- */
    const OGR_SRSNode *poEllipsoid = poDatum->GetNode( "SPHEROID" );

    if( poEllipsoid != NULL )
    {
        CPLXMLNode *psEllipseXML;

        psEllipseXML = 
            CPLCreateXMLNode( 
                CPLCreateXMLNode(psDatumXML,CXT_Element,"gml:usesEllipsoid" ),
                CXT_Element, "gml:Ellipsoid" );

        addGMLId( psEllipseXML );

        CPLCreateXMLElementAndValue( psEllipseXML, "gml:ellipsoidName", 
                                     poEllipsoid->GetChild(0)->GetValue() );

        exportAuthorityToXML( poEllipsoid, "gml:ellipsoidID", psEllipseXML,
                              "ellipsoid");
        
        CPLXMLNode *psParmXML;

        psParmXML = CPLCreateXMLNode( psEllipseXML, CXT_Element, 
                                      "gml:semiMajorAxis" );

        CPLCreateXMLNode( CPLCreateXMLNode(psParmXML,CXT_Attribute,"gml:uom"),
                          CXT_Text, "urn:ogc:def:uom:EPSG::9001" );

        CPLCreateXMLNode( psParmXML, CXT_Text, 
                          poEllipsoid->GetChild(1)->GetValue() );

        psParmXML = 
            CPLCreateXMLNode( 
                CPLCreateXMLNode( psEllipseXML, CXT_Element, 
                                  "gml:secondDefiningParameter" ),
                CXT_Element, "gml:inverseFlattening" );
        
        CPLCreateXMLNode( CPLCreateXMLNode(psParmXML,CXT_Attribute,"gml:uom"),
                          CXT_Text, "urn:ogc:def:uom:EPSG::9201" );

        CPLCreateXMLNode( psParmXML, CXT_Text, 
                          poEllipsoid->GetChild(2)->GetValue() );
    }

    return psGCS_XML;
}

/************************************************************************/
/*                         exportProjCSToXML()                          */
/************************************************************************/

static CPLXMLNode *exportProjCSToXML( const OGRSpatialReference *poSRS )

{
    const OGR_SRSNode *poProjCS = poSRS->GetAttrNode( "PROJCS" );

    if( poProjCS == NULL )
        return NULL;
    
/* -------------------------------------------------------------------- */
/*      Establish initial infrastructure.                               */
/* -------------------------------------------------------------------- */
    CPLXMLNode   *psCRS_XML;

    psCRS_XML = CPLCreateXMLNode( NULL, CXT_Element, "gml:ProjectedCRS" );
    addGMLId( psCRS_XML );
    
/* -------------------------------------------------------------------- */
/*      Attach symbolic name (a name in a nameset).                     */
/* -------------------------------------------------------------------- */
    CPLCreateXMLElementAndValue( psCRS_XML, "gml:srsName", 
                                 poProjCS->GetChild(0)->GetValue() );

/* -------------------------------------------------------------------- */
/*      Add authority info if we have it.                               */
/* -------------------------------------------------------------------- */
    exportAuthorityToXML( poProjCS, "gml:srsID", psCRS_XML, "crs" );

/* -------------------------------------------------------------------- */
/*      Use the GEOGCS as a <baseCRS>                                   */
/* -------------------------------------------------------------------- */
    CPLXMLNode *psBaseCRSXML =
        CPLCreateXMLNode( psCRS_XML, CXT_Element, "gml:baseCRS" );

    CPLAddXMLChild( psBaseCRSXML, exportGeogCSToXML( poSRS ) );

/* -------------------------------------------------------------------- */
/*      Our projected coordinate system is "defined by Conversion".     */
/* -------------------------------------------------------------------- */
    CPLXMLNode *psDefinedBy;

    psDefinedBy = CPLCreateXMLNode( psCRS_XML, CXT_Element, 
                                    "gml:definedByConversion" );
    
/* -------------------------------------------------------------------- */
/*      Projections are handled as ParameterizedTransformations.        */
/* -------------------------------------------------------------------- */
    const char *pszProjection = poSRS->GetAttrValue("PROJECTION");
    CPLXMLNode *psConv;

    psConv = CPLCreateXMLNode( psDefinedBy, CXT_Element, "gml:Conversion");
    addGMLId( psConv );

/* -------------------------------------------------------------------- */
/*      Transverse Mercator                                             */
/* -------------------------------------------------------------------- */
    if( EQUAL(pszProjection,SRS_PT_TRANSVERSE_MERCATOR) )
    {
        AddValueIDWithURN( psConv, "gml:usesMethod", "EPSG", "method", 
                           9807 );

        addProjArg( poSRS, psConv, "Angular", 0.0,
                    8801, SRS_PP_LATITUDE_OF_ORIGIN );
        addProjArg( poSRS, psConv, "Angular", 0.0,
                    8802, SRS_PP_CENTRAL_MERIDIAN );
        addProjArg( poSRS, psConv, "Unitless", 1.0,
                    8805, SRS_PP_SCALE_FACTOR );
        addProjArg( poSRS, psConv, "Linear", 0.0,
                    8806, SRS_PP_FALSE_EASTING );
        addProjArg( poSRS, psConv, "Linear", 0.0,
                    8807, SRS_PP_FALSE_NORTHING );
    }

/* -------------------------------------------------------------------- */
/*      Define the cartesian coordinate system.                         */
/* -------------------------------------------------------------------- */
    CPLXMLNode  *psCCS;

    psCCS = 
        CPLCreateXMLNode( 
            CPLCreateXMLNode( psCRS_XML, CXT_Element, "gml:usesCartesianCS" ),
            CXT_Element, "gml:CartesianCS" );

    addGMLId( psCCS );

    CPLCreateXMLElementAndValue( psCCS, "gml:csName", "Cartesian" );
    addAuthorityIDBlock( psCCS, "gml:csID", "EPSG", "cs", 4400 );
    addAxis( psCCS, "E", NULL );
    addAxis( psCCS, "N", NULL );

    return psCRS_XML;
}

/************************************************************************/
/*                            exportToXML()                             */
/************************************************************************/

OGRErr OGRSpatialReference::exportToXML( char **ppszRawXML, 
                                         const char * /*pszDialect*/ ) const

{
    CPLXMLNode *psXMLTree = NULL;

    if( IsGeographic() )
    {
        psXMLTree = exportGeogCSToXML( this );
    }
    else if( IsProjected() )
    {
        psXMLTree = exportProjCSToXML( this );
    }
    else
        return OGRERR_UNSUPPORTED_SRS;

    *ppszRawXML = CPLSerializeXMLTree( psXMLTree );

    return OGRERR_NONE;
}

/************************************************************************/
/*                           OSRExportToXML()                           */
/************************************************************************/

OGRErr OSRExportToXML( OGRSpatialReferenceH hSRS, char **ppszRawXML, 
                       const char *pszDialect )

{
    return ((OGRSpatialReference *) hSRS)->exportToXML( ppszRawXML, 
                                                        pszDialect );
}

#ifdef notdef
/************************************************************************/
/*                           importXMLUnits()                           */
/************************************************************************/

static void importXMLUnits( CPLXMLNode *psSrcXML, const char *pszClass,
                            OGRSpatialReference *poSRS, const char *pszTarget)

{
    const char *pszUnitName, *pszUnitsPer;
    OGR_SRSNode *poNode = poSRS->GetAttrNode( pszTarget );
    OGR_SRSNode *poUnits;

    CPLAssert( EQUAL(pszClass,"AngularUnit") 
               || EQUAL(pszClass,"LinearUnit") );
        
    psSrcXML = CPLGetXMLNode( psSrcXML, pszClass );
    if( psSrcXML == NULL )
        goto DefaultTarget;

    pszUnitName = CPLGetXMLValue( psSrcXML, "NameSet.name", "unnamed" );
    if( EQUAL(pszClass,"AngularUnit") )
        pszUnitsPer = CPLGetXMLValue( psSrcXML, "radiansPerUnit", NULL );
    else
        pszUnitsPer = CPLGetXMLValue( psSrcXML, "metresPerUnit", NULL );
    
    if( pszUnitsPer == NULL )
    {
        CPLDebug( "OGR_SRS_XML", 
                  "Missing PerUnit value for %s.", 
                  pszClass );
        goto DefaultTarget;
    }
    
    if( poNode == NULL )
    {
        CPLDebug( "OGR_SRS_XML", "Can't find %s in importXMLUnits.", 
                  pszTarget );
        goto DefaultTarget;
    }

    if( poNode->FindChild("UNIT") != -1 )
    {
        poUnits = poNode->GetChild( poNode->FindChild( "UNIT" ) );
        poUnits->GetChild(0)->SetValue( pszUnitName );
        poUnits->GetChild(1)->SetValue( pszUnitsPer );
    }
    else
    {
        poUnits = new OGR_SRSNode( "UNIT" );
        poUnits->AddChild( new OGR_SRSNode( pszUnitName ) );
        poUnits->AddChild( new OGR_SRSNode( pszUnitsPer ) );
        
        poNode->AddChild( poUnits );
    }
    return;

  DefaultTarget:
    poUnits = new OGR_SRSNode( "UNIT" );
    if( EQUAL(pszClass,"AngularUnit") )
    {
        poUnits->AddChild( new OGR_SRSNode( SRS_UA_DEGREE ) );
        poUnits->AddChild( new OGR_SRSNode( SRS_UA_DEGREE_CONV ) );
    }
    else
    {
        poUnits->AddChild( new OGR_SRSNode( SRS_UL_METER ) );
        poUnits->AddChild( new OGR_SRSNode( "1.0" ) );
    }

    poNode->AddChild( poUnits );
}
#endif

/************************************************************************/
/*                         importXMLAuthority()                         */
/************************************************************************/

static void importXMLAuthority( CPLXMLNode *psSrcXML, 
                                OGRSpatialReference *poSRS, 
                                const char *pszSourceKey,
                                const char *pszTargetKey )

{
    CPLXMLNode *psIDNode = CPLGetXMLNode( psSrcXML, pszSourceKey );
    CPLXMLNode *psNameNode = CPLGetXMLNode( psIDNode, "name" );
    CPLXMLNode *psCodeSpace = CPLGetXMLNode( psNameNode, "codeSpace" );
    char *pszAuthority, *pszCode, *pszURN;
    int nCode = 0;

    if( psIDNode == NULL || psNameNode == NULL || psCodeSpace == NULL )
        return;

    pszURN = CPLStrdup(CPLGetXMLValue( psCodeSpace, "", "" ));
    if( !parseURN( pszURN, NULL, &pszAuthority, &pszCode ) )
    {
        CPLFree( pszURN );
        return;
    }

    if( strlen(pszCode) == 0 )
        pszCode = (char *) CPLGetXMLValue( psNameNode, "", "" );

    if( pszCode != NULL )
        nCode = atoi(pszCode);

    if( nCode != 0 )
        poSRS->SetAuthority( pszTargetKey, pszAuthority, nCode );

    CPLFree( pszURN );
}

/************************************************************************/
/*                         getProjectionParm()                          */
/************************************************************************/

static double getProjectionParm( CPLXMLNode *psRootNode, 
                                  int nParameterCode, 
                                 const char * /*pszMeasureType */, 
                                 double dfDefault )

{
    CPLXMLNode *psUsesParameter;

    for( psUsesParameter = psRootNode->psChild; 
         psUsesParameter != NULL;
         psUsesParameter = psUsesParameter->psNext )
    {
        if( psUsesParameter->eType != CXT_Element
            || !EQUAL(psUsesParameter->pszValue,"usesParameterValue") )
            continue;

        if( atoi(CPLGetXMLValue( psUsesParameter, "valueOfParameter", "0" ))
            == nParameterCode )
        {
            const char *pszValue = CPLGetXMLValue( psUsesParameter, "value", 
                                                   NULL );

            if( pszValue != NULL )
                return atof(pszValue);
            else
                return dfDefault;
        }
    }

    return dfDefault;
}

/************************************************************************/
/*                         getNormalizedValue()                         */
/*                                                                      */
/*      Parse a node to get it's numerical value, and then normalize    */
/*      into meters of degrees depending on the measure type.           */
/************************************************************************/

static double getNormalizedValue( CPLXMLNode *psNode, const char *pszPath,
                                  const char * /*pszMeasure*/, 
                                  double dfDefault )

{
    CPLXMLNode *psTargetNode;
    CPLXMLNode *psValueNode;

    if( pszPath == NULL || strlen(pszPath) == 0 )
        psTargetNode = psNode;
    else
        psTargetNode = CPLGetXMLNode( psNode, pszPath );
    
    if( psTargetNode == NULL )
        return dfDefault;

    for( psValueNode = psTargetNode->psChild; 
         psValueNode != NULL && psValueNode->eType != CXT_Text;
         psValueNode = psValueNode->psNext ) {}

    if( psValueNode == NULL )
        return dfDefault;
    
    // Add normalization later.

    return atof(psValueNode->pszValue);
}

/************************************************************************/
/*                        importGeogCSFromXML()                         */
/************************************************************************/

static OGRErr importGeogCSFromXML( OGRSpatialReference *poSRS, 
                                   CPLXMLNode *psCRS )

{
    const char     *pszGeogName, *pszDatumName, *pszEllipsoidName, *pszPMName;
    double         dfSemiMajor, dfInvFlattening, dfPMOffset = 0.0;

/* -------------------------------------------------------------------- */
/*      Set the GEOGCS name from the srsName.                           */
/* -------------------------------------------------------------------- */
    pszGeogName = 
        CPLGetXMLValue( psCRS, "srsName", "Unnamed GeogCS" );

/* -------------------------------------------------------------------- */
/*      If we don't seem to have a detailed coordinate system           */
/*      definition, check if we can define based on an EPSG code.       */
/* -------------------------------------------------------------------- */
    CPLXMLNode     *psDatum;

    psDatum = CPLGetXMLNode( psCRS, "usesGeodeticDatum.GeodeticDatum" );

    if( psDatum == NULL )
    {
        OGRSpatialReference oIdSRS;

        oIdSRS.SetLocalCS( "dummy" );
        importXMLAuthority( psCRS, &oIdSRS, "srsID", "LOCAL_CS" );

        if( oIdSRS.GetAuthorityCode( "LOCAL_CS" ) != NULL
            && oIdSRS.GetAuthorityName( "LOCAL_CS" ) != NULL
            && EQUAL(oIdSRS.GetAuthorityName("LOCAL_CS"),"EPSG") )
        {
            return poSRS->importFromEPSG( 
                atoi(oIdSRS.GetAuthorityCode("LOCAL_CS")) );
        }
    }

/* -------------------------------------------------------------------- */
/*      Get datum name.                                                 */
/* -------------------------------------------------------------------- */
    pszDatumName = 
        CPLGetXMLValue( psDatum, "datumName", "Unnamed Datum" );

/* -------------------------------------------------------------------- */
/*      Get ellipsoid information.                                      */
/* -------------------------------------------------------------------- */
    CPLXMLNode *psE;

    psE = CPLGetXMLNode( psDatum, "usesEllipsoid.Ellipsoid" );
    pszEllipsoidName = 
        CPLGetXMLValue( psE, "ellipsoidName", "Unnamed Ellipsoid" );

    dfSemiMajor = getNormalizedValue( psE, "semiMajorAxis", "Linear", 
                                      SRS_WGS84_SEMIMAJOR );

    dfInvFlattening = 
        getNormalizedValue( psE, "secondDefiningParameter.inverseFlattening", 
                            "Unitless", 0.0 );

    if( dfInvFlattening == 0.0 )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Ellipsoid inverseFlattening corrupt or missing." );
        return OGRERR_CORRUPT_DATA;
    }

/* -------------------------------------------------------------------- */
/*      Get the prime meridian.                                         */
/* -------------------------------------------------------------------- */
    CPLXMLNode *psPM;

    psPM = CPLGetXMLNode( psDatum, "usesPrimeMeridian.PrimeMeridian" );
    if( psPM == NULL )
    {
        pszPMName = "Greenwich";
        dfPMOffset = 0.0;
    }
    else
    {
        pszPMName = CPLGetXMLValue( psPM, "meridianName", 
                                    "Unnamed Prime Meridian");
        dfPMOffset = 
            getNormalizedValue( psPM, "greenwichLongitude.angle",
                                "Angular", 0.0 );
    }

/* -------------------------------------------------------------------- */
/*      Set the geographic definition.                                  */
/* -------------------------------------------------------------------- */
    poSRS->SetGeogCS( pszGeogName, pszDatumName, 
                      pszEllipsoidName, dfSemiMajor, dfInvFlattening, 
                      pszPMName, dfPMOffset );

/* -------------------------------------------------------------------- */
/*      Look for angular units.  We don't check that all axes match     */
/*      at this time.                                                   */
/* -------------------------------------------------------------------- */
#ifdef notdef
    CPLXMLNode *psAxis;

    psAxis = CPLGetXMLNode( psGeo2DCRS, 
                            "EllipsoidalCoordinateSystem.CoordinateAxis" );
    importXMLUnits( psAxis, "AngularUnit", poSRS, "GEOGCS" );
#endif

/* -------------------------------------------------------------------- */
/*      Can we set authorities for any of the levels?                   */
/* -------------------------------------------------------------------- */
    importXMLAuthority( psCRS, poSRS, "srsID", "GEOGCS" );
    importXMLAuthority( psDatum, poSRS, "datumID", "GEOGCS|DATUM" );
    importXMLAuthority( psE, poSRS, "ellipsoidID", 
                        "GEOGCS|DATUM|SPHEROID" );
    importXMLAuthority( psDatum, poSRS, 
                        "usesPrimeMeridian.PrimeMeridian.meridianID",
                        "GEOGCS|PRIMEM" );

    poSRS->Fixup();
    
    return OGRERR_NONE;
}

/************************************************************************/
/*                        importProjCSFromXML()                         */
/************************************************************************/

static OGRErr importProjCSFromXML( OGRSpatialReference *poSRS, 
                                   CPLXMLNode *psCRS )

{
    CPLXMLNode *psSubXML;
    OGRErr eErr;

/* -------------------------------------------------------------------- */
/*      Setup the PROJCS node with a name.                              */
/* -------------------------------------------------------------------- */
    poSRS->SetProjCS( CPLGetXMLValue( psCRS, "srsName", "Unnamed" ) );

/* -------------------------------------------------------------------- */
/*      Get authority information if available.  If we got it, and      */
/*      we seem to be lacking inline definition values, try and         */
/*      define according to the EPSG code for the PCS.                  */
/* -------------------------------------------------------------------- */
    importXMLAuthority( psCRS, poSRS, "srsID", "PROJCS" );

    if( poSRS->GetAuthorityCode( "PROJCS" ) != NULL
        && poSRS->GetAuthorityName( "PROJCS" ) != NULL
        && EQUAL(poSRS->GetAuthorityName("PROJCS"),"EPSG")
        && (CPLGetXMLNode( psCRS, "definedByConversion.Conversion" ) == NULL
            || CPLGetXMLNode( psCRS, "baseCRS.GeographicCRS" ) == NULL) )
    {
        return poSRS->importFromEPSG( atoi(poSRS->GetAuthorityCode("PROJCS")) );
    }

/* -------------------------------------------------------------------- */
/*      Try to set the GEOGCS info.                                     */
/* -------------------------------------------------------------------- */
    
    psSubXML = CPLGetXMLNode( psCRS, "baseCRS.GeographicCRS" );
    if( psSubXML != NULL )
    {
        eErr = importGeogCSFromXML( poSRS, psSubXML );
        if( eErr != OGRERR_NONE )
            return eErr;
    }

/* -------------------------------------------------------------------- */
/*      Get the conversion node.  It should be the only child of the    */
/*      definedByConversion node.                                       */
/* -------------------------------------------------------------------- */
    CPLXMLNode *psConv = NULL;

    psConv = CPLGetXMLNode( psCRS, "definedByConversion.Conversion" );
    if( psConv == NULL || psConv->eType != CXT_Element )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "Unable to find a conversion node under the definedByConversion\n"
                  "node of the ProjectedCRS." );
        return OGRERR_CORRUPT_DATA;
    }

/* -------------------------------------------------------------------- */
/*      Determine the conversion method in effect.  We really ought     */
/*      to  check the xlink:href URN to see if this is an EPSG          */
/*      method value, but it is unlikely that other code spaces will    */
/*      conflict.                                                       */
/* -------------------------------------------------------------------- */
    int nMethod = atoi(
        CPLGetXMLValue( psConv, "usesMethod", "0" ) );
    
/* -------------------------------------------------------------------- */
/*      Transverse Mercator.                                            */
/* -------------------------------------------------------------------- */
    if( nMethod == 9807 ) 
    {
        poSRS->SetTM( 
            getProjectionParm( psConv, 8801, "Angular", 0.0 ), 
            getProjectionParm( psConv, 8802, "Angular", 0.0 ), 
            getProjectionParm( psConv, 8805, "Unitless", 1.0 ), 
            getProjectionParm( psConv, 8806, "Linear", 0.0 ),
            getProjectionParm( psConv, 8807, "Linear", 0.0 ) );
    }

/* -------------------------------------------------------------------- */
/*      Didn't recognise?                                               */
/* -------------------------------------------------------------------- */
    else
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "Conversion method %d not recognised.", 
                  nMethod );
        return OGRERR_CORRUPT_DATA;
    }


/* -------------------------------------------------------------------- */
/*      Cleanup and return.                                             */
/* -------------------------------------------------------------------- */
    poSRS->Fixup();

    // Need to get linear units here!

    return OGRERR_NONE;
}

/************************************************************************/
/*                           importFromXML()                            */
/************************************************************************/

OGRErr OGRSpatialReference::importFromXML( const char *pszXML )

{
    CPLXMLNode *psTree;
    OGRErr  eErr = OGRERR_UNSUPPORTED_SRS;

    this->Clear();
        
/* -------------------------------------------------------------------- */
/*      Parse the XML.                                                  */
/* -------------------------------------------------------------------- */
    psTree = CPLParseXMLString( pszXML );
    
    if( psTree == NULL )
        return OGRERR_CORRUPT_DATA;

    CPLStripXMLNamespace( psTree, "gml", TRUE );

/* -------------------------------------------------------------------- */
/*      Import according to the root node type.  We walk through        */
/*      root elements as there is sometimes prefix stuff like           */
/*      <?xml>.                                                         */
/* -------------------------------------------------------------------- */
    CPLXMLNode *psNode = psTree;

    for( psNode = psTree; psNode != NULL; psNode = psNode->psNext )
    {
        if( EQUAL(psNode->pszValue,"GeographicCRS") )
        {
            eErr = importGeogCSFromXML( this, psNode );
            break;
        }
        
        else if( EQUAL(psNode->pszValue,"ProjectedCRS") )
        {
            eErr = importProjCSFromXML( this, psNode );
            break;
        }
    }

    CPLDestroyXMLNode( psTree );

    return eErr;
}

/************************************************************************/
/*                          OSRImportFromXML()                          */
/************************************************************************/

OGRErr OSRImportFromXML( OGRSpatialReferenceH hSRS, const char *pszXML )

{
    return ((OGRSpatialReference *) hSRS)->importFromXML( pszXML );
}
