/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Implements OGRDGNLayer class.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2000, Frank Warmerdam (warmerdam@pobox.com)
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
 * Revision 1.27  2003/05/21 03:42:01  warmerda
 * Expanded tabs
 *
 * Revision 1.26  2003/05/15 14:47:24  warmerda
 * implement quaternion support on write
 *
 * Revision 1.25  2003/05/12 18:48:57  warmerda
 * added preliminary 3D write support
 *
 * Revision 1.24  2003/01/02 21:45:23  warmerda
 * move OGRBuildPolygonsFromEdges into C API
 *
 * Revision 1.23  2002/11/11 20:34:22  warmerda
 * added create support
 *
 * Revision 1.22  2002/10/29 19:45:29  warmerda
 * OGR driver now always builds an index if any features are to be read.  This
 * is primarily done to ensure that color tables appearing late in the file
 * will still affect feature elements occuring before them ... such as with
 * the m_epoche.dgn file.  Also implement fast feature counting and spatial
 * extents support based on the index.
 *
 * Revision 1.21  2002/05/31 19:03:46  warmerda
 * removed old CollectLines code
 *
 * Revision 1.20  2002/05/31 16:57:21  warmerda
 * made Text, MSLink and EntityNum attributes available
 *
 * Revision 1.19  2002/03/27 21:36:50  warmerda
 * added implementation of GetFeature()
 *
 * Revision 1.18  2002/02/22 22:18:44  warmerda
 * added support for commplex shapes
 *
 * Revision 1.17  2002/02/20 22:27:35  warmerda
 * fixed problem with units on text height for very small values
 *
 * Revision 1.16  2002/01/21 21:36:16  warmerda
 * removed DGNid
 *
 * Revision 1.15  2002/01/21 20:55:10  warmerda
 * use dgnlib spatial filtering support
 *
 * Revision 1.14  2002/01/18 18:52:21  warmerda
 * set rotation angle for labels
 *
 * Revision 1.13  2002/01/15 06:39:56  warmerda
 * remove _gv_color, flesh out pen and brush style settings
 *
 * Revision 1.12  2002/01/09 14:12:12  warmerda
 * Treat SHAPE elements as polygon geometries, not linestrings.
 *
 * Revision 1.11  2001/11/09 18:14:36  warmerda
 * added size info for LABELs
 *
 * Revision 1.10  2001/11/09 15:59:23  warmerda
 * set style information for text, drop _gv_ogrfs
 *
 * Revision 1.9  2001/11/06 14:44:41  warmerda
 * Removed printf() statement.
 *
 * Revision 1.8  2001/07/18 04:55:16  warmerda
 * added CPL_CSVID
 *
 * Revision 1.7  2001/06/19 15:50:23  warmerda
 * added feature attribute query support
 *
 * Revision 1.6  2001/03/07 19:29:46  warmerda
 * added support for stroking curves
 *
 * Revision 1.5  2001/03/07 15:20:13  warmerda
 * Only apply the _gv_color property if the color lookup is successful.
 *
 * Revision 1.4  2001/01/16 21:19:29  warmerda
 * Added preliminary text support
 *
 * Revision 1.3  2001/01/16 18:11:12  warmerda
 * majorly extended, added arc support
 *
 * Revision 1.2  2000/12/28 21:29:17  warmerda
 * use stype field
 *
 * Revision 1.1  2000/11/28 19:03:47  warmerda
 * New
 *
 */

#include "ogr_dgn.h"
#include "cpl_conv.h"
#include "ogr_featurestyle.h"
#include "ogr_api.h"

CPL_CVSID("$Id$");

/************************************************************************/
/*                           OGRDGNLayer()                              */
/************************************************************************/

OGRDGNLayer::OGRDGNLayer( const char * pszName, DGNHandle hDGN,
                          int bUpdate )
    
{
    poFilterGeom = NULL;
    
    this->hDGN = hDGN;
    this->bUpdate = bUpdate;

    poFeatureDefn = new OGRFeatureDefn( pszName );
    
    OGRFieldDefn        oField( "", OFTInteger );

/* -------------------------------------------------------------------- */
/*      Element type                                                    */
/* -------------------------------------------------------------------- */
    oField.SetName( "Type" );
    oField.SetType( OFTInteger );
    oField.SetWidth( 2 );
    oField.SetPrecision( 0 );
    poFeatureDefn->AddFieldDefn( &oField );

/* -------------------------------------------------------------------- */
/*      Level number.                                                   */
/* -------------------------------------------------------------------- */
    oField.SetName( "Level" );
    oField.SetType( OFTInteger );
    oField.SetWidth( 2 );
    oField.SetPrecision( 0 );
    poFeatureDefn->AddFieldDefn( &oField );

/* -------------------------------------------------------------------- */
/*      graphic group                                                   */
/* -------------------------------------------------------------------- */
    oField.SetName( "GraphicGroup" );
    oField.SetType( OFTInteger );
    oField.SetWidth( 4 );
    oField.SetPrecision( 0 );
    poFeatureDefn->AddFieldDefn( &oField );

/* -------------------------------------------------------------------- */
/*      ColorIndex                                                      */
/* -------------------------------------------------------------------- */
    oField.SetName( "ColorIndex" );
    oField.SetType( OFTInteger );
    oField.SetWidth( 3 );
    oField.SetPrecision( 0 );
    poFeatureDefn->AddFieldDefn( &oField );

/* -------------------------------------------------------------------- */
/*      Weight                                                          */
/* -------------------------------------------------------------------- */
    oField.SetName( "Weight" );
    oField.SetType( OFTInteger );
    oField.SetWidth( 2 );
    oField.SetPrecision( 0 );
    poFeatureDefn->AddFieldDefn( &oField );

/* -------------------------------------------------------------------- */
/*      Style                                                           */
/* -------------------------------------------------------------------- */
    oField.SetName( "Style" );
    oField.SetType( OFTInteger );
    oField.SetWidth( 1 );
    oField.SetPrecision( 0 );
    poFeatureDefn->AddFieldDefn( &oField );

/* -------------------------------------------------------------------- */
/*      EntityNum                                                       */
/* -------------------------------------------------------------------- */
    oField.SetName( "EntityNum" );
    oField.SetType( OFTInteger );
    oField.SetWidth( 8 );
    oField.SetPrecision( 0 );
    poFeatureDefn->AddFieldDefn( &oField );

/* -------------------------------------------------------------------- */
/*      MSLink                                                          */
/* -------------------------------------------------------------------- */
    oField.SetName( "MSLink" );
    oField.SetType( OFTInteger );
    oField.SetWidth( 10 );
    oField.SetPrecision( 0 );
    poFeatureDefn->AddFieldDefn( &oField );

/* -------------------------------------------------------------------- */
/*      Text                                                            */
/* -------------------------------------------------------------------- */
    oField.SetName( "Text" );
    oField.SetType( OFTString );
    oField.SetWidth( 0 );
    oField.SetPrecision( 0 );
    poFeatureDefn->AddFieldDefn( &oField );
}

/************************************************************************/
/*                           ~OGRDGNLayer()                             */
/************************************************************************/

OGRDGNLayer::~OGRDGNLayer()

{
    delete poFeatureDefn;

    if( poFilterGeom != NULL )
        delete poFilterGeom;
}

/************************************************************************/
/*                          SetSpatialFilter()                          */
/************************************************************************/

void OGRDGNLayer::SetSpatialFilter( OGRGeometry * poGeomIn )

{
    if( poFilterGeom != NULL )
    {
        delete poFilterGeom;
        poFilterGeom = NULL;
    }

    if( poGeomIn != NULL )
    {
        OGREnvelope     oEnvelope;
        
        poFilterGeom = poGeomIn->clone();

        poGeomIn->getEnvelope( &oEnvelope );
        DGNSetSpatialFilter( hDGN, 
                             oEnvelope.MinX, 
                             oEnvelope.MinY, 
                             oEnvelope.MaxX, 
                             oEnvelope.MaxY );
    }
    else
    {
        DGNSetSpatialFilter( hDGN, 0.0, 0.0, 0.0, 0.0 );
    }

    ResetReading();
}

/************************************************************************/
/*                            ResetReading()                            */
/************************************************************************/

void OGRDGNLayer::ResetReading()

{
    iNextShapeId = 0;
    DGNRewind( hDGN );
}

/************************************************************************/
/*                             GetFeature()                             */
/************************************************************************/

OGRFeature *OGRDGNLayer::GetFeature( long nFeatureId )

{
    OGRFeature *poFeature;
    DGNElemCore *psElement;

    if( !DGNGotoElement( hDGN, nFeatureId ) )
        return NULL;

    // We should likely clear the spatial search region as it affects 
    // DGNReadElement() but I will defer that for now. 

    psElement = DGNReadElement( hDGN );
    poFeature = ElementToFeature( psElement );
    DGNFreeElement( hDGN, psElement );

    if( poFeature == NULL )
        return NULL;

    if( poFeature->GetFID() != nFeatureId )
    {
        delete poFeature;
        return NULL;
    }

    return poFeature;
}

/************************************************************************/
/*                           ConsiderBrush()                            */
/*                                                                      */
/*      Method to set the style for a polygon, including a brush if     */
/*      appropriate.                                                    */
/************************************************************************/

void OGRDGNLayer::ConsiderBrush( DGNElemCore *psElement, const char *pszPen,
                                 OGRFeature *poFeature )

{
    int         gv_red, gv_green, gv_blue;
    char                szFullStyle[256];
    int                 nFillColor;

    if( DGNGetShapeFillInfo( hDGN, psElement, &nFillColor ) 
        && DGNLookupColor( hDGN, nFillColor, 
                           &gv_red, &gv_green, &gv_blue ) )
    {
        sprintf( szFullStyle, 
                 "BRUSH(fc:#%02x%02x%02x,id:\"ogr-brush-0\")",
                 gv_red, gv_green, gv_blue );
              
        if( nFillColor != psElement->color )
        {
            strcat( szFullStyle, ";" );
            strcat( szFullStyle, pszPen );
        }
        poFeature->SetStyleString( szFullStyle );
    }
    else
        poFeature->SetStyleString( pszPen );
}

/************************************************************************/
/*                          ElementToFeature()                          */
/************************************************************************/

OGRFeature *OGRDGNLayer::ElementToFeature( DGNElemCore *psElement )

{
    OGRFeature  *poFeature = new OGRFeature( poFeatureDefn );

    poFeature->SetFID( psElement->element_id );
    poFeature->SetField( "Type", psElement->type );
    poFeature->SetField( "Level", psElement->level );
    poFeature->SetField( "GraphicGroup", psElement->graphic_group );
    poFeature->SetField( "ColorIndex", psElement->color );
    poFeature->SetField( "Weight", psElement->weight );
    poFeature->SetField( "Style", psElement->style );
    
/* -------------------------------------------------------------------- */
/*      Apply first MSLink if available.                                */
/* -------------------------------------------------------------------- */
    unsigned char *pabyData;
    int nEntityNum=0, nMSLink=0;

    pabyData = DGNGetLinkage( hDGN, psElement, 0, NULL, &nEntityNum, &nMSLink, 
                              NULL );
    if( pabyData && (nMSLink != 0 || nEntityNum != 0) )
    {
        poFeature->SetField( "EntityNum", nEntityNum );
        poFeature->SetField( "MSLink", nMSLink );
    }

/* -------------------------------------------------------------------- */
/*      Lookup color.                                                   */
/* -------------------------------------------------------------------- */
    char        gv_color[128];
    int         gv_red, gv_green, gv_blue;
    char        szFSColor[128], szPen[256];

    szFSColor[0] = '\0';
    if( DGNLookupColor( hDGN, psElement->color, 
                        &gv_red, &gv_green, &gv_blue ) )
    {
        sprintf( gv_color, "%f %f %f 1.0", 
                 gv_red / 255.0, gv_green / 255.0, gv_blue / 255.0 );

        sprintf( szFSColor, "c:#%02x%02x%02x", 
                 gv_red, gv_green, gv_blue );
    }

/* -------------------------------------------------------------------- */
/*      Generate corresponding PEN style.                               */
/* -------------------------------------------------------------------- */
    if( psElement->style == DGNS_SOLID )
        sprintf( szPen, "PEN(id:\"ogr-pen-0\"" );
    else if( psElement->style == DGNS_DOTTED )
        sprintf( szPen, "PEN(id:\"ogr-pen-5\"" );
    else if( psElement->style == DGNS_MEDIUM_DASH )
        sprintf( szPen, "PEN(id:\"ogr-pen-2\"" );
    else if( psElement->style == DGNS_LONG_DASH )
        sprintf( szPen, "PEN(id:\"ogr-pen-4\"" );
    else if( psElement->style == DGNS_DOT_DASH )
        sprintf( szPen, "PEN(id:\"ogr-pen-6\"" );
    else if( psElement->style == DGNS_SHORT_DASH )
        sprintf( szPen, "PEN(id:\"ogr-pen-3\"" );
    else if( psElement->style == DGNS_DASH_DOUBLE_DOT )
        sprintf( szPen, "PEN(id:\"ogr-pen-7\"" );
    else if( psElement->style == DGNS_LONG_DASH_SHORT_DASH )
        sprintf( szPen, "PEN(p:\"10px 5px 4px 5px\"" );
    else
        sprintf( szPen, "PEN(id:\"ogr-pen-0\"" );

    if( strlen(szFSColor) > 0 )
        sprintf( szPen+strlen(szPen), ",%s", szFSColor );

    if( psElement->weight > 1 )
        sprintf( szPen+strlen(szPen), ",w:%dpx", psElement->weight );
        
    strcat( szPen, ")" );

    switch( psElement->stype )
    {
      case DGNST_MULTIPOINT:
        if( psElement->type == DGNT_SHAPE )
        {
            OGRLinearRing       *poLine = new OGRLinearRing();
            OGRPolygon          *poPolygon = new OGRPolygon();
            DGNElemMultiPoint *psEMP = (DGNElemMultiPoint *) psElement;
            
            poLine->setNumPoints( psEMP->num_vertices );
            for( int i = 0; i < psEMP->num_vertices; i++ )
            {
                poLine->setPoint( i, 
                                  psEMP->vertices[i].x,
                                  psEMP->vertices[i].y,
                                  psEMP->vertices[i].z );
            }

            poPolygon->addRingDirectly( poLine );

            poFeature->SetGeometryDirectly( poPolygon );

            ConsiderBrush( psElement, szPen, poFeature );
        }
        else if( psElement->type == DGNT_CURVE )
        {
            DGNElemMultiPoint *psEMP = (DGNElemMultiPoint *) psElement;
            OGRLineString       *poLine = new OGRLineString();
            DGNPoint            *pasPoints;
            int                 nPoints;

            nPoints = 5 * psEMP->num_vertices;
            pasPoints = (DGNPoint *) CPLMalloc(sizeof(DGNPoint) * nPoints);
            
            DGNStrokeCurve( hDGN, psEMP, nPoints, pasPoints );

            poLine->setNumPoints( nPoints );
            for( int i = 0; i < nPoints; i++ )
            {
                poLine->setPoint( i, 
                                  pasPoints[i].x,
                                  pasPoints[i].y,
                                  pasPoints[i].z );
            }

            poFeature->SetGeometryDirectly( poLine );
            CPLFree( pasPoints );

            poFeature->SetStyleString( szPen );
        }
        else
        {
            OGRLineString       *poLine = new OGRLineString();
            DGNElemMultiPoint *psEMP = (DGNElemMultiPoint *) psElement;
            
            if( psEMP->num_vertices > 0 )
            {
                poLine->setNumPoints( psEMP->num_vertices );
                for( int i = 0; i < psEMP->num_vertices; i++ )
                {
                    poLine->setPoint( i, 
                                      psEMP->vertices[i].x,
                                      psEMP->vertices[i].y,
                                      psEMP->vertices[i].z );
                }
                
                poFeature->SetGeometryDirectly( poLine );
            }

            poFeature->SetStyleString( szPen );
        }
        break;

      case DGNST_ARC:
      {
          OGRLineString *poLine = new OGRLineString();
          DGNElemArc    *psArc = (DGNElemArc *) psElement;
          DGNPoint      asPoints[90];
          int           nPoints;

          nPoints = (int) (MAX(1,ABS(psArc->sweepang) / 5) + 1);
          DGNStrokeArc( hDGN, psArc, nPoints, asPoints );

          poLine->setNumPoints( nPoints );
          for( int i = 0; i < nPoints; i++ )
          {
              poLine->setPoint( i, 
                                asPoints[i].x,
                                asPoints[i].y,
                                asPoints[i].z );
          }

          poFeature->SetGeometryDirectly( poLine );
          poFeature->SetStyleString( szPen );
      }
      break;

      case DGNST_TEXT:
      {
          OGRPoint      *poPoint = new OGRPoint();
          DGNElemText   *psText = (DGNElemText *) psElement;
          char          *pszOgrFS;

          poPoint->setX( psText->origin.x );
          poPoint->setY( psText->origin.y );
          poPoint->setZ( psText->origin.z );

          poFeature->SetGeometryDirectly( poPoint );

          pszOgrFS = (char *) CPLMalloc(strlen(psText->string) + 150);

          // setup the basic label.
          sprintf( pszOgrFS, "LABEL(t:\"%s\"",  psText->string );

          // set the color if we have it. 
          if( strlen(szFSColor) > 0 )
              sprintf( pszOgrFS+strlen(pszOgrFS), ",%s", szFSColor );

          // Add the size info in ground units.
          if( ABS(psText->height_mult) >= 6.0 )
              sprintf( pszOgrFS+strlen(pszOgrFS), ",s:%dg", 
                       (int) psText->height_mult );
          else if( ABS(psText->height_mult) > 0.1 )
              sprintf( pszOgrFS+strlen(pszOgrFS), ",s:%.3fg", 
                       psText->height_mult );
          else
              sprintf( pszOgrFS+strlen(pszOgrFS), ",s:%.12fg", 
                       psText->height_mult );

          // Add the angle, if not horizontal
          if( psText->rotation != 0.0 )
              sprintf( pszOgrFS+strlen(pszOgrFS), ",a:%d", 
                       (int) (psText->rotation+0.5) );

          strcat( pszOgrFS, ")" );

          poFeature->SetStyleString( pszOgrFS );
          CPLFree( pszOgrFS );

          poFeature->SetField( "Text", psText->string );
      }
      break;

      case DGNST_COMPLEX_HEADER:
      {
          DGNElemComplexHeader *psHdr = (DGNElemComplexHeader *) psElement;
          int           iChild;
          OGRMultiLineString  oChildren;

          /* collect subsequent child geometries. */
          // we should disable the spatial filter ... add later.
          for( iChild = 0; iChild < psHdr->numelems; iChild++ )
          {
              OGRFeature *poChildFeature = NULL;
              DGNElemCore *psChildElement;

              psChildElement = DGNReadElement( hDGN );
              // should verify complex bit set, not another header.

              if( psChildElement != NULL )
                  poChildFeature = ElementToFeature( psChildElement );

              if( poChildFeature != NULL
                  && poChildFeature->GetGeometryRef() != NULL )
              {
                  OGRGeometry *poGeom;

                  poGeom = poChildFeature->GetGeometryRef();
                  if( wkbFlatten(poGeom->getGeometryType()) == wkbLineString )
                      oChildren.addGeometry( poGeom );
              }

              if( poChildFeature != NULL )
                  delete poChildFeature;
          }

          // Try to assemble into polygon geometry.
          OGRGeometry *poGeom;

          if( psElement->type == DGNT_COMPLEX_SHAPE_HEADER )
              poGeom = (OGRPolygon *) 
                  OGRBuildPolygonFromEdges( (OGRGeometryH) &oChildren, 
                                            TRUE, TRUE, 100000, NULL );
          else
              poGeom = oChildren.clone();

          if( poGeom != NULL )
              poFeature->SetGeometryDirectly( poGeom );

          ConsiderBrush( psElement, szPen, poFeature );
      }
      break;

      default:
        break;
    }

    return poFeature;
}

/************************************************************************/
/*                           GetNextFeature()                           */
/************************************************************************/

OGRFeature *OGRDGNLayer::GetNextFeature()

{
    DGNElemCore *psElement;

    DGNGetElementIndex( hDGN, NULL );

    while( (psElement = DGNReadElement( hDGN )) != NULL )
    {
        OGRFeature      *poFeature;

        if( psElement->deleted )
        {
            DGNFreeElement( hDGN, psElement );
            continue;
        }

        poFeature = ElementToFeature( psElement );
        DGNFreeElement( hDGN, psElement );

        if( poFeature == NULL )
            continue;

        if( poFeature->GetGeometryRef() == NULL )
        {
            delete poFeature;
            continue;
        }

        if( m_poAttrQuery == NULL
            || m_poAttrQuery->Evaluate( poFeature ) )
            return poFeature;

        delete poFeature;
    }        

    return NULL;
}

/************************************************************************/
/*                           TestCapability()                           */
/************************************************************************/

int OGRDGNLayer::TestCapability( const char * pszCap )

{
    if( EQUAL(pszCap,OLCRandomRead) )
        return TRUE;

    else if( EQUAL(pszCap,OLCSequentialWrite) )
        return bUpdate;
    else if( EQUAL(pszCap,OLCRandomWrite) )
        return FALSE; /* maybe later? */

    else if( EQUAL(pszCap,OLCFastFeatureCount) )
        return poFilterGeom == NULL || m_poAttrQuery == NULL;

    else if( EQUAL(pszCap,OLCFastSpatialFilter) )
        return FALSE;

    else if( EQUAL(pszCap,OLCFastGetExtent) )
        return TRUE;

    else 
        return FALSE;
}

/************************************************************************/
/*                          GetFeatureCount()                           */
/************************************************************************/

int OGRDGNLayer::GetFeatureCount( int bForce )

{
/* -------------------------------------------------------------------- */
/*      If any odd conditions are in effect collect the information     */
/*      normally.                                                       */
/* -------------------------------------------------------------------- */
    if( poFilterGeom != NULL || m_poAttrQuery != NULL )
        return OGRLayer::GetFeatureCount( bForce );

/* -------------------------------------------------------------------- */
/*      Otherwise scan the index.                                       */
/* -------------------------------------------------------------------- */
    int nElementCount, i, nFeatureCount = 0;
    int bInComplexShape = FALSE;
    const DGNElementInfo *pasIndex = DGNGetElementIndex(hDGN,&nElementCount);

    for( i = 0; i < nElementCount; i++ )
    {
        if( pasIndex[i].flags & DGNEIF_DELETED )
            continue;

        switch( pasIndex[i].stype )
        {
          case DGNST_MULTIPOINT:
          case DGNST_ARC:
          case DGNST_TEXT:
            if( !(pasIndex[i].flags & DGNEIF_COMPLEX) || !bInComplexShape )
            {
                nFeatureCount++;
                bInComplexShape = FALSE;
            }
            break;

          case DGNST_COMPLEX_HEADER:
            nFeatureCount++;
            bInComplexShape = TRUE;
            break;

          default:
            break;
        }
    }

    return nFeatureCount;
}

/************************************************************************/
/*                             GetExtent()                              */
/************************************************************************/

OGRErr OGRDGNLayer::GetExtent( OGREnvelope *psExtent, int bForce )

{
    double      adfExtents[6];

    if( !DGNGetExtents( hDGN, adfExtents ) )
        return OGRERR_FAILURE;
    
    psExtent->MinX = adfExtents[0];
    psExtent->MinY = adfExtents[1];
    psExtent->MaxX = adfExtents[3];
    psExtent->MaxY = adfExtents[4];
    
    return OGRERR_NONE;
}

/************************************************************************/
/*                      LineStringToElementGroup()                      */
/*                                                                      */
/*      Convert an OGR line string to one or more DGN elements.  If     */
/*      the input is too long for a single element (more than 38        */
/*      points) we split it into multiple LINE_STRING elements, and     */
/*      prefix with a complex group header element.                     */
/*                                                                      */
/*      This method can create handle creating shapes, or line          */
/*      strings for the aggregate object, but the components of a       */
/*      complex shape group are always line strings.                    */
/************************************************************************/

#define MAX_ELEM_POINTS 38

DGNElemCore **OGRDGNLayer::LineStringToElementGroup( OGRLineString *poLS,
                                                     int nGroupType )

{
    int nTotalPoints = poLS->getNumPoints();
    int iNextPoint = 0, iGeom = 0;
    DGNElemCore **papsGroup;

    papsGroup = (DGNElemCore **) 
        CPLCalloc( sizeof(void*), (nTotalPoints/(MAX_ELEM_POINTS-1))+3 );

    for( iNextPoint = 0; iNextPoint < nTotalPoints;  )
    {
        DGNPoint asPoints[38];
        int nThisCount = 0;

        // we need to repeat end points of elements.
        if( iNextPoint != 0 )
            iNextPoint--;

        for( ; iNextPoint < nTotalPoints && nThisCount < MAX_ELEM_POINTS; 
             iNextPoint++, nThisCount++ )
        {
            asPoints[nThisCount].x = poLS->getX( iNextPoint );
            asPoints[nThisCount].y = poLS->getY( iNextPoint );
            asPoints[nThisCount].z = poLS->getZ( iNextPoint );
        }
        
        if( nTotalPoints <= MAX_ELEM_POINTS )
            papsGroup[0] = DGNCreateMultiPointElem( hDGN, nGroupType,
                                                 nThisCount, asPoints);
        else
            papsGroup[++iGeom] = 
                DGNCreateMultiPointElem( hDGN, DGNT_LINE_STRING,
                                         nThisCount, asPoints);
    }

/* -------------------------------------------------------------------- */
/*      We needed to make into a group.  Create the complex header      */
/*      from the rest of the group.                                     */
/* -------------------------------------------------------------------- */
    if( papsGroup[0] == NULL )
    {
        if( nGroupType == DGNT_SHAPE )
            nGroupType = DGNT_COMPLEX_SHAPE_HEADER;
        else
            nGroupType = DGNT_COMPLEX_CHAIN_HEADER;
        
        papsGroup[0] = 
            DGNCreateComplexHeaderFromGroup( hDGN, nGroupType, 
                                             iGeom, papsGroup + 1 );
    }

    return papsGroup;
}

/************************************************************************/
/*                           TranslateLabel()                           */
/*                                                                      */
/*      Translate LABEL feature.                                        */
/************************************************************************/

DGNElemCore **OGRDGNLayer::TranslateLabel( OGRFeature *poFeature )

{
    DGNElemCore **papsGroup;
    OGRPoint *poPoint = (OGRPoint *) poFeature->GetGeometryRef();
    OGRStyleMgr oMgr;
    OGRStyleLabel *poLabel;
    const char *pszText = poFeature->GetFieldAsString( "Text" );
    double dfRotation = 0.0;
    double dfCharHeight = 100.0;

    oMgr.InitFromFeature( poFeature );
    poLabel = (OGRStyleLabel *) oMgr.GetPart( 0 );
    if( poLabel != NULL && poLabel->GetType() != OGRSTCLabel )
    {
        delete poLabel;
        poLabel = NULL;
    }

    if( poLabel != NULL )
    {
        GBool bDefault;

        if( poLabel->TextString(bDefault) != NULL && !bDefault )
            pszText = poLabel->TextString(bDefault);
        dfRotation = poLabel->Angle(bDefault);

        poLabel->Size( bDefault );
        if( !bDefault && poLabel->GetUnit() == OGRSTUGround )
            dfCharHeight = poLabel->Size(bDefault);
        // this part is really kind of bogus.
        if( !bDefault && poLabel->GetUnit() == OGRSTUMM )
            dfCharHeight = poLabel->Size(bDefault)/1000.0;

        /* poLabel->ForeColor(); */

        /* get font id */
    }

    papsGroup = (DGNElemCore **) CPLCalloc(sizeof(void*),2);
    papsGroup[0] = 
        DGNCreateTextElem( hDGN, pszText, 0, DGNJ_LEFT_BOTTOM, 
                           dfCharHeight, dfCharHeight, dfRotation, NULL,
                           poPoint->getX(), 
                           poPoint->getY(), 
                           poPoint->getZ() );

    return papsGroup;
}

/************************************************************************/
/*                           CreateFeature()                            */
/*                                                                      */
/*      Create a new feature and write to file.                         */
/************************************************************************/

OGRErr OGRDGNLayer::CreateFeature( OGRFeature *poFeature )

{
    if( !bUpdate )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Attempt to create feature on read-only DGN file." );
        return OGRERR_FAILURE;
    }

/* -------------------------------------------------------------------- */
/*      Translate the geometry.                                         */
/* -------------------------------------------------------------------- */
    DGNElemCore **papsGroup = NULL;
    OGRGeometry *poGeom = poFeature->GetGeometryRef();
    int i;
    const char *pszStyle = poFeature->GetStyleString();

    if( poGeom == NULL )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Features with empty, geometry collection geometries not\n"
                  "supported in DGN format." );
        return OGRERR_FAILURE;
    }

    if( wkbFlatten(poGeom->getGeometryType()) == wkbPoint )
    {
        OGRPoint *poPoint = (OGRPoint *) poGeom;
        const char *pszText = poFeature->GetFieldAsString("Text");

        if( (pszText == NULL || strlen(pszText) == 0)
            && (pszStyle == NULL || strstr(pszStyle,"LABEL") == NULL) )
        {
            DGNPoint asPoints[2];

            papsGroup = (DGNElemCore **) CPLCalloc(sizeof(void*),2);

            // Treat a non text point as a degenerate line.
            asPoints[0].x = poPoint->getX();
            asPoints[0].y = poPoint->getY();
            asPoints[0].z = poPoint->getZ();
            asPoints[1] = asPoints[0];
            
            papsGroup[0] = DGNCreateMultiPointElem( hDGN, DGNT_LINE, 
                                                    2, asPoints );
        }
        else
        {
            papsGroup = TranslateLabel( poFeature );
        }
    }
    else if( wkbFlatten(poGeom->getGeometryType()) == wkbLineString )
    {
        papsGroup = LineStringToElementGroup( (OGRLineString *) poGeom, 
                                              DGNT_LINE_STRING );
    }
    else if( wkbFlatten(poGeom->getGeometryType()) == wkbPolygon )
    {
        OGRPolygon *poPoly = ((OGRPolygon *) poGeom);

        // Ignore all but the exterior ring. 
        papsGroup = LineStringToElementGroup( poPoly->getExteriorRing(),
                                              DGNT_SHAPE );
    }
    else if( wkbFlatten(poGeom->getGeometryType()) == wkbMultiPolygon )
    {
        OGRMultiPolygon *poMP = ((OGRMultiPolygon *) poGeom);
        OGRPolygon *poPoly = (OGRPolygon *) poMP->getGeometryRef(0);

        // Ignore everything but the outer ring of the first polygon.
        if( poPoly != NULL )
            papsGroup = LineStringToElementGroup( poPoly->getExteriorRing(),
                                                  DGNT_SHAPE );
    }
    else
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Unsupported geometry type (%s) for DGN.",
                  OGRGeometryTypeToName( poGeom->getGeometryType() ) );
        return OGRERR_FAILURE;
    }

/* -------------------------------------------------------------------- */
/*      Add other attributes.                                           */
/* -------------------------------------------------------------------- */
    int nLevel = poFeature->GetFieldAsInteger( "Level" );
    int nGraphicGroup = poFeature->GetFieldAsInteger( "GraphicGroup" );
    int nColor = poFeature->GetFieldAsInteger( "ColorIndex" );
    int nWeight = poFeature->GetFieldAsInteger( "Weight" );
    int nStyle = poFeature->GetFieldAsInteger( "Style" );

    nLevel = MAX(0,MIN(63,nLevel));
    nColor = MAX(0,MIN(255,nGraphicGroup));
    nWeight = MAX(0,MIN(31,nWeight));
    nStyle = MAX(0,MIN(7,nStyle));

    DGNUpdateElemCore( hDGN, papsGroup[0], nLevel, nGraphicGroup, nColor, 
                       nWeight, nStyle );
    
/* -------------------------------------------------------------------- */
/*      Write to file.                                                  */
/* -------------------------------------------------------------------- */
    for( i = 0; papsGroup[i] != NULL; i++ )
    {
        DGNWriteElement( hDGN, papsGroup[i] );

        if( i == 0 )
            poFeature->SetFID( papsGroup[i]->element_id );

        DGNFreeElement( hDGN, papsGroup[i] );
    }

    CPLFree( papsGroup );

    return OGRERR_NONE;
}
