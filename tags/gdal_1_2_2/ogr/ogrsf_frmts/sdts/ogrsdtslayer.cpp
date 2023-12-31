/******************************************************************************
 * $Id$
 *
 * Project:  SDTSReader
 * Purpose:  Implements OGRSDTSLayer class.
 * Author:   Frank Warmerdam, warmerda@home.com
 *
 ******************************************************************************
 * Copyright (c) 1999, Frank Warmerdam
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
 * Revision 1.10  2003/12/11 21:11:00  warmerda
 * fixed leak of ATIDRefs list
 *
 * Revision 1.9  2003/10/06 16:21:33  warmerda
 * ensure we get the attributes properly for primary and secondary records
 *
 * Revision 1.8  2001/07/18 04:55:16  warmerda
 * added CPL_CSVID
 *
 * Revision 1.7  2001/07/16 03:47:49  warmerda
 * added ENID and SNID to line features
 *
 * Revision 1.6  2001/06/19 15:50:23  warmerda
 * added feature attribute query support
 *
 * Revision 1.5  2001/01/19 21:14:22  warmerda
 * expanded tabs
 *
 * Revision 1.4  2000/02/20 21:17:56  warmerda
 * added projection support
 *
 * Revision 1.3  2000/02/15 13:59:30  warmerda
 * Actually lookup and copy attributes
 *
 * Revision 1.2  1999/09/29 16:48:02  warmerda
 * added (perhaps incomplete) attribute handling.
 *
 * Revision 1.1  1999/09/22 13:32:16  warmerda
 * New
 *
 */

#include "ogr_sdts.h"
#include "cpl_conv.h"
#include "cpl_string.h"

CPL_CVSID("$Id$");

/************************************************************************/
/*                            OGRSDTSLayer()                            */
/*                                                                      */
/*      Note that the OGRSDTSLayer assumes ownership of the passed      */
/*      OGRFeatureDefn object.                                          */
/************************************************************************/

OGRSDTSLayer::OGRSDTSLayer( SDTSTransfer * poTransferIn, int iLayerIn,
                            OGRSDTSDataSource * poDSIn )

{
    poFilterGeom = NULL;
    poDS = poDSIn;

    poTransfer = poTransferIn;
    iLayer = iLayerIn;

    poReader = poTransfer->GetLayerIndexedReader( iLayer );

/* -------------------------------------------------------------------- */
/*      Define the feature.                                             */
/* -------------------------------------------------------------------- */
    int         iCATDEntry = poTransfer->GetLayerCATDEntry( iLayer );
    
    poFeatureDefn =
        new OGRFeatureDefn(poTransfer->GetCATD()->GetEntryModule(iCATDEntry));

    OGRFieldDefn oRecId( "RCID", OFTInteger );
    poFeatureDefn->AddFieldDefn( &oRecId );

    if( poTransfer->GetLayerType(iLayer) == SLTPoint )
    {
        poFeatureDefn->SetGeomType( wkbPoint );
    }
    else if( poTransfer->GetLayerType(iLayer) == SLTLine )
    {
        poFeatureDefn->SetGeomType( wkbLineString );

        oRecId.SetName( "SNID" );
        poFeatureDefn->AddFieldDefn( &oRecId );

        oRecId.SetName( "ENID" );
        poFeatureDefn->AddFieldDefn( &oRecId );
    }
    else if( poTransfer->GetLayerType(iLayer) == SLTPoly )
    {
        poFeatureDefn->SetGeomType( wkbPolygon );
    }
    else if( poTransfer->GetLayerType(iLayer) == SLTAttr )
    {
        poFeatureDefn->SetGeomType( wkbNone );
    }

/* -------------------------------------------------------------------- */
/*      Add schema from referenced attribute records.                   */
/* -------------------------------------------------------------------- */
    char        **papszATIDRefs = NULL;
    
    if( poTransfer->GetLayerType(iLayer) != SLTAttr )
        papszATIDRefs = poReader->ScanModuleReferences();
    else
        papszATIDRefs = CSLAddString( papszATIDRefs, 
                                      poTransfer->GetCATD()->GetEntryModule(iCATDEntry) );

    for( int iTable = 0;
         papszATIDRefs != NULL && papszATIDRefs[iTable] != NULL;
         iTable++ )
    {
        SDTSAttrReader  *poAttrReader;
        DDFFieldDefn    *poFDefn;
        
/* -------------------------------------------------------------------- */
/*      Get the attribute table reader, and the associated user         */
/*      attribute field.                                                */
/* -------------------------------------------------------------------- */
        poAttrReader = (SDTSAttrReader *)
            poTransfer->GetLayerIndexedReader(
                poTransfer->FindLayer( papszATIDRefs[iTable] ) );

        if( poAttrReader == NULL )
            continue;

        poFDefn = poAttrReader->GetModule()->FindFieldDefn( "ATTP" );
        if( poFDefn == NULL )
            poFDefn = poAttrReader->GetModule()->FindFieldDefn( "ATTS" );
        if( poFDefn == NULL )
            continue;
        
/* -------------------------------------------------------------------- */
/*      Process each user subfield on the attribute table into an       */
/*      OGR field definition.                                           */
/* -------------------------------------------------------------------- */
        for( int iSF=0; iSF < poFDefn->GetSubfieldCount(); iSF++ )
        {
            DDFSubfieldDefn     *poSFDefn = poFDefn->GetSubfield( iSF );
            int                 nWidth = poSFDefn->GetWidth();
            char                *pszFieldName;

            if( poFeatureDefn->GetFieldIndex( poSFDefn->GetName() ) != -1 )
                pszFieldName = CPLStrdup( CPLSPrintf( "%s_%s",
                                                      papszATIDRefs[iTable],
                                                      poSFDefn->GetName() ) );
            else
                pszFieldName = CPLStrdup( poSFDefn->GetName() );
            
            switch( poSFDefn->GetType() )
            {
              case DDFString:
              {
                  OGRFieldDefn  oStrField( pszFieldName, OFTString );

                  if( nWidth != 0 )
                      oStrField.SetWidth( nWidth );

                  poFeatureDefn->AddFieldDefn( &oStrField );
              }
              break;

              case DDFInt:
              {
                  OGRFieldDefn  oIntField( pszFieldName, OFTInteger );

                  if( nWidth != 0 )
                      oIntField.SetWidth( nWidth );

                  poFeatureDefn->AddFieldDefn( &oIntField );
              }
              break;

              case DDFFloat:
              {
                  OGRFieldDefn  oRealField( pszFieldName, OFTReal );

                  // We don't have a precision in DDF files, so we never even
                  // use the width.  Otherwise with a precision of zero the
                  // result would look like an integer.

                  poFeatureDefn->AddFieldDefn( &oRealField );
              }
              break;

              default:
                break;
            }

            CPLFree( pszFieldName );
            
        } /* next iSF (subfield) */
    } /* next iTable */
    CSLDestroy( papszATIDRefs );
}

/************************************************************************/
/*                           ~OGRSDTSLayer()                           */
/************************************************************************/

OGRSDTSLayer::~OGRSDTSLayer()

{
    delete poFeatureDefn;

    if( poFilterGeom != NULL )
        delete poFilterGeom;
}

/************************************************************************/
/*                          SetSpatialFilter()                          */
/************************************************************************/

void OGRSDTSLayer::SetSpatialFilter( OGRGeometry * poGeomIn )

{
    if( poFilterGeom != NULL )
    {
        delete poFilterGeom;
        poFilterGeom = NULL;
    }

    if( poGeomIn != NULL )
        poFilterGeom = poGeomIn->clone();
}

/************************************************************************/
/*                            ResetReading()                            */
/************************************************************************/

void OGRSDTSLayer::ResetReading()

{
    poReader->Rewind();
}

/************************************************************************/
/*                     AssignAttrRecordToFeature()                      */
/************************************************************************/

static void
AssignAttrRecordToFeature( OGRFeature * poFeature, SDTSTransfer * poTransfer, 
                           DDFField * poSR )

{
/* -------------------------------------------------------------------- */
/*      Process each subfield in the record.                            */
/* -------------------------------------------------------------------- */
    DDFFieldDefn        *poFDefn = poSR->GetFieldDefn();
        
    for( int iSF=0; iSF < poFDefn->GetSubfieldCount(); iSF++ )
    {
        DDFSubfieldDefn *poSFDefn = poFDefn->GetSubfield( iSF );
        int                     iField;
        int                     nMaxBytes;
        const char *    pachData = poSR->GetSubfieldData(poSFDefn,
                                                         &nMaxBytes);
/* -------------------------------------------------------------------- */
/*      Indentify this field on the feature.                            */
/* -------------------------------------------------------------------- */
        iField = poFeature->GetFieldIndex( poSFDefn->GetName() );

/* -------------------------------------------------------------------- */
/*      Handle each of the types.                                       */
/* -------------------------------------------------------------------- */
        switch( poSFDefn->GetType() )
        {
          case DDFString:
            const char  *pszValue;

            pszValue = poSFDefn->ExtractStringData(pachData, nMaxBytes,
                                                   NULL);

            if( iField != -1 )
                poFeature->SetField( iField, pszValue );
            break;

          case DDFFloat:
            double      dfValue;

            dfValue = poSFDefn->ExtractFloatData(pachData, nMaxBytes,
                                                 NULL);

            if( iField != -1 )
                poFeature->SetField( iField, dfValue );
            break;

          case DDFInt:
            int         nValue;

            nValue = poSFDefn->ExtractIntData(pachData, nMaxBytes, NULL);

            if( iField != -1 )
                poFeature->SetField( iField, nValue );
            break;

          default:
            break;
        }
    } /* next subfield */
}

/************************************************************************/
/*                      GetNextUnfilteredFeature()                      */
/************************************************************************/

OGRFeature * OGRSDTSLayer::GetNextUnfilteredFeature()

{
/* -------------------------------------------------------------------- */
/*      If not done before we need to assemble the geometry for a       */
/*      polygon layer.                                                  */
/* -------------------------------------------------------------------- */
    if( poTransfer->GetLayerType(iLayer) == SLTPoly )
    {
        ((SDTSPolygonReader *) poReader)->AssembleRings(poTransfer);
    }

/* -------------------------------------------------------------------- */
/*      Fetch the next sdts style feature object from the reader.       */
/* -------------------------------------------------------------------- */
    SDTSFeature *poSDTSFeature = poReader->GetNextFeature();
    OGRFeature  *poFeature;

    if( poSDTSFeature == NULL )
        return NULL;

/* -------------------------------------------------------------------- */
/*      Create the OGR feature.                                         */
/* -------------------------------------------------------------------- */
    poFeature = new OGRFeature( poFeatureDefn );

    switch( poTransfer->GetLayerType(iLayer) )
    {
/* -------------------------------------------------------------------- */
/*      Translate point feature specific information and geometry.      */
/* -------------------------------------------------------------------- */
      case SLTPoint:
      {
          SDTSRawPoint  *poPoint = (SDTSRawPoint *) poSDTSFeature;
          
          poFeature->SetGeometryDirectly( new OGRPoint( poPoint->dfX,
                                                        poPoint->dfY,
                                                        poPoint->dfZ ) );
      }
      break;

/* -------------------------------------------------------------------- */
/*      Translate line feature specific information and geometry.       */
/* -------------------------------------------------------------------- */
      case SLTLine:
      {
          SDTSRawLine   *poLine = (SDTSRawLine *) poSDTSFeature;
          OGRLineString *poOGRLine = new OGRLineString();

          poOGRLine->setPoints( poLine->nVertices,
                                poLine->padfX, poLine->padfY, poLine->padfZ );
          poFeature->SetGeometryDirectly( poOGRLine );
          poFeature->SetField( "SNID", (int) poLine->oStartNode.nRecord );
          poFeature->SetField( "ENID", (int) poLine->oEndNode.nRecord );
      }
      break;

/* -------------------------------------------------------------------- */
/*      Translate polygon feature specific information and geometry.    */
/* -------------------------------------------------------------------- */
      case SLTPoly:
      {
          SDTSRawPolygon *poPoly = (SDTSRawPolygon *) poSDTSFeature;
          OGRPolygon *poOGRPoly = new OGRPolygon();

          for( int iRing = 0; iRing < poPoly->nRings; iRing++ )
          {
              OGRLinearRing *poRing = new OGRLinearRing();
              int           nVertices;

              if( iRing == poPoly->nRings - 1 )
                  nVertices = poPoly->nVertices - poPoly->panRingStart[iRing];
              else
                  nVertices = poPoly->panRingStart[iRing+1]
                            - poPoly->panRingStart[iRing];

              poRing->setPoints( nVertices,
                                 poPoly->padfX + poPoly->panRingStart[iRing],
                                 poPoly->padfY + poPoly->panRingStart[iRing],
                                 poPoly->padfZ + poPoly->panRingStart[iRing] );

              poOGRPoly->addRingDirectly( poRing );
          }

          poFeature->SetGeometryDirectly( poOGRPoly );
      }
      break;

      default:
        break;
    }

/* -------------------------------------------------------------------- */
/*      Set attributes for any indicated attribute records.             */
/* -------------------------------------------------------------------- */
    int         iAttrRecord;
    
    for( iAttrRecord = 0; 
         iAttrRecord < poSDTSFeature->nAttributes; 
         iAttrRecord++)
    {
        DDFField        *poSR;

        poSR = poTransfer->GetAttr( poSDTSFeature->paoATID+iAttrRecord );
          
        AssignAttrRecordToFeature( poFeature, poTransfer, poSR );
    }

/* -------------------------------------------------------------------- */
/*      If this record is an attribute record, attach the local         */
/*      attributes.                                                     */
/* -------------------------------------------------------------------- */
    if( poTransfer->GetLayerType(iLayer) == SLTAttr )
    {
        AssignAttrRecordToFeature( poFeature, poTransfer, 
                                   ((SDTSAttrRecord *) poSDTSFeature)->poATTR);
    }
    
/* -------------------------------------------------------------------- */
/*      Translate the record id.                                        */
/* -------------------------------------------------------------------- */
    poFeature->SetFID( poSDTSFeature->oModId.nRecord );
    poFeature->SetField( 0, (int) poSDTSFeature->oModId.nRecord );
    if( poFeature->GetGeometryRef() != NULL )
        poFeature->GetGeometryRef()->assignSpatialReference(
            poDS->GetSpatialRef() );

    if( !poReader->IsIndexed() )
        delete poSDTSFeature;

    return poFeature;
}


/************************************************************************/
/*                           GetNextFeature()                           */
/************************************************************************/

OGRFeature *OGRSDTSLayer::GetNextFeature()

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

        if( (poFilterGeom == NULL
             || poFeature->GetGeometryRef() == NULL 
             || poFilterGeom->Intersect( poFeature->GetGeometryRef() ) )
            && (m_poAttrQuery == NULL
                || m_poAttrQuery->Evaluate( poFeature )) )
            break;

        delete poFeature;
    }

    return poFeature;
}

/************************************************************************/
/*                           TestCapability()                           */
/************************************************************************/

int OGRSDTSLayer::TestCapability( const char * pszCap )

{
    if( EQUAL(pszCap,OLCRandomRead) )
        return FALSE;

    else if( EQUAL(pszCap,OLCSequentialWrite) 
             || EQUAL(pszCap,OLCRandomWrite) )
        return FALSE;

    else if( EQUAL(pszCap,OLCFastFeatureCount) )
        return FALSE;

    else if( EQUAL(pszCap,OLCFastSpatialFilter) )
        return FALSE;

    else 
        return FALSE;
}


/************************************************************************/
/*                           GetSpatialRef()                            */
/************************************************************************/

OGRSpatialReference * OGRSDTSLayer::GetSpatialRef()

{
    return poDS->GetSpatialRef();
}
