/******************************************************************************
 *
 * Project:  KML Translator
 * Purpose:  Implements OGRLIBKMLDriver
 * Author:   Brian Case, rush at winkey dot org
 *
 ******************************************************************************
 * Copyright (c) 2010, Brian Case
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
 *****************************************************************************/

#include "ogr_libkml.h"
//#include "cpl_conv.h"
//#include "cpl_string.h"
#include "cpl_error.h"

#include <kml/dom.h>

using kmldom::KmlFactory;
using kmldom::PlacemarkPtr;
using kmldom::Placemark;
using kmldom::DocumentPtr;
using kmldom::FeaturePtr;
using kmldom::Feature;
using kmldom::KmlPtr;
using kmldom::Kml;
using kmlengine::KmzFile;
using kmlengine::KmlFile;
using kmlengine::Bbox;

#include "ogrlibkmlfeature.h"
#include "ogrlibkmllayerstyle.h"

/******************************************************************************




******************************************************************************/

OGRLIBKMLLayer::OGRLIBKMLLayer ( const char *pszLayerName,
                                 OGRSpatialReference * poSpatialRef,
                                 OGRwkbGeometryType eGType,
                                 OGRLIBKMLDataSource * poOgrDS,
                                 ElementPtr poKmlRoot)
{
//CPLGetBasename ( oKmlHref.get_path (  ).c_str (  ) )
    printf("createing a layer named %s\n", pszLayerName);
    m_pszLayerName = CPLStrdup ( pszLayerName );
    m_poOgrDS = poOgrDS;
    iFeature = 0;

    m_poOgrSRS = new OGRSpatialReference ( NULL );
    m_poOgrSRS->SetWellKnownGeogCS ( "WGS84" );

    m_poOgrFeatureDefn = new OGRFeatureDefn( pszLayerName );
    m_poOgrFeatureDefn->Reference();
    m_poOgrFeatureDefn->SetGeomType( eGType );

    OGRFieldDefn oOgrFieldName( "Name", OFTString );
    m_poOgrFeatureDefn->AddFieldDefn( &oOgrFieldName );
    
    OGRFieldDefn oOgrFieldDesc( "Description", OFTString );
    m_poOgrFeatureDefn->AddFieldDefn( &oOgrFieldDesc );

    /***** store the root element pointer *****/

    m_poKmlLayer = boost::static_pointer_cast <kmldom::Container>( poKmlRoot);
    
        
    
    nFeatures = m_poKmlLayer->get_feature_array_size() ;
    iFeature = 0;

    m_poStyleTable = NULL;
    
}

/******************************************************************************

******************************************************************************/

OGRLIBKMLLayer::~OGRLIBKMLLayer (  )
{

    CPLFree ( ( void * )m_pszLayerName );
    delete m_poOgrSRS;
    m_poOgrFeatureDefn->Release();

    
}

/******************************************************************************

******************************************************************************/

void OGRLIBKMLLayer::ResetReading (
     )
{

    iFeature = 0;


    return;
}

/******************************************************************************

******************************************************************************/

OGRFeature *OGRLIBKMLLayer::GetNextFeature (
     )
{
    FeaturePtr poKmlFeature;
    OGRFeature *poOgrFeature = NULL;

#warning perhaps we need an array of idexes that contain actual placemarks

    do {
        if ( iFeature >= nFeatures )
            break;

        poKmlFeature = m_poKmlLayer->get_feature_array_at ( iFeature );
        iFeature++;

    } while ( poKmlFeature->Type (  ) != kmldom::Type_Placemark );


    if ( iFeature < nFeatures
         && poKmlFeature->Type (  ) == kmldom::Type_Placemark ) {
        poOgrFeature =
             kml2feat ( boost::static_pointer_cast < Placemark >
                       ( poKmlFeature ), this, m_poOgrFeatureDefn );
             poOgrFeature->SetFID(iFeature);
    }

    return poOgrFeature;
}



/******************************************************************************

******************************************************************************/

OGRErr OGRLIBKMLLayer::SetNextByIndex (
    long nIndex )
{
    iFeature = nIndex;

    return OGRERR_NONE;
}

/******************************************************************************

******************************************************************************/

OGRFeature *OGRLIBKMLLayer::GetFeature (
    long nFID )
{

    SetNextByIndex ( nFID );

    OGRFeature *poOgrFeature = GetNextFeature (  );

    return poOgrFeature;
}

/******************************************************************************

******************************************************************************/

OGRErr OGRLIBKMLLayer::SetFeature (
    OGRFeature * poFeature )
{
#warning we need to figure this out

    return OGRERR_UNSUPPORTED_OPERATION;
}

/******************************************************************************

******************************************************************************/

OGRErr OGRLIBKMLLayer::CreateFeature (
    OGRFeature * poOgrFeat )
{

    PlacemarkPtr poKmlPlacemark =
        feat2kml ( this, poOgrFeat, m_poOgrDS->GetKmlFactory (  ) );

    m_poKmlLayer->add_feature ( poKmlPlacemark );

    return OGRERR_NONE;
}

/******************************************************************************

******************************************************************************/

OGRErr OGRLIBKMLLayer::DeleteFeature (
    long nFID )
{

#warning we need to figure this out

    return OGRERR_UNSUPPORTED_OPERATION;
}

/******************************************************************************

******************************************************************************/



/******************************************************************************

******************************************************************************/

int OGRLIBKMLLayer::GetFeatureCount (
    int bForce )
{

#warning this return value can be wrong, kml features could be the schema, style, or a folder

    return nFeatures;
}

/******************************************************************************
 GetExtent()
******************************************************************************/

OGRErr OGRLIBKMLLayer::GetExtent (
    OGREnvelope * psExtent,
    int bForce )
{
    Bbox oKmlBbox;

    if ( kmlengine::
         GetFeatureBounds ( boost::static_pointer_cast < Feature >
                            ( m_poKmlLayer ), &oKmlBbox ) ) {
        psExtent->MinX = oKmlBbox.get_west (  );
        psExtent->MinY = oKmlBbox.get_south (  );
        psExtent->MaxX = oKmlBbox.get_east (  );
        psExtent->MaxY = oKmlBbox.get_north (  );

        return OGRERR_NONE;
    }

    return OGRERR_FAILURE;
}




/******************************************************************************

******************************************************************************/

OGRErr OGRLIBKMLLayer::CreateField (
    OGRFieldDefn * poField,
    int bApproxOK )
{


}


/******************************************************************************

******************************************************************************/

OGRErr OGRLIBKMLLayer::SyncToDisk (
     )
{

    KmlFactory *poKmlFactory = m_poOgrDS->GetKmlFactory (  );
    KmzFile *poKmlKmzfile ;//= m_poOgrDS->GetKmz (  );

    KmlPtr poKmlKml = poKmlFactory->CreateKml (  );

    poKmlKml->set_feature ( m_poKmlDocument );

    std::string strKmlOut = kmldom::SerializePretty ( poKmlKml );
#warning this file name still might not be correct

    if ( poKmlKmzfile->
         AddFile ( strKmlOut,
                   CPLString (  ).Printf ( "%s.kml", m_pszLayerName ) ) )
        return OGRERR_NONE;

    return OGRERR_FAILURE;
}

/******************************************************************************

******************************************************************************/

OGRStyleTable *OGRLIBKMLLayer::GetStyleTable (
     )
{

    return m_poStyleTable;
}

/******************************************************************************

******************************************************************************/

void OGRLIBKMLLayer::SetStyleTableDirectly (
    OGRStyleTable * poStyleTable )
{

    m_poStyleTable = poStyleTable;

    styletable2kml ( m_poStyleTable, m_poOgrDS->GetKmlFactory (  ),
                     m_poKmlDocument );

    return;
}

/******************************************************************************

******************************************************************************/

void OGRLIBKMLLayer::SetStyleTable (
    OGRStyleTable * poStyleTable )
{

    m_poStyleTable = poStyleTable->Clone (  );

    styletable2kml ( m_poStyleTable, m_poOgrDS->GetKmlFactory (  ),
                     m_poKmlDocument );

    return;
}

/******************************************************************************

******************************************************************************/

const char *OGRLIBKMLLayer::GetName (
     )
{

    return m_pszLayerName;
}

/******************************************************************************

******************************************************************************/

int OGRLIBKMLLayer::TestCapability (
    const char *pszCap )
{
    int result = FALSE;

    if ( EQUAL ( pszCap, OLCRandomRead ) )
        result = TRUE;
    if ( EQUAL ( pszCap, OLCSequentialWrite ) )
        result = bUpdate;
#warning todo random write
    if ( EQUAL ( pszCap, OLCRandomWrite ) )
        result = FALSE;
    if ( EQUAL ( pszCap, OLCFastFeatureCount ) && !GetSpatialFilter (  ) )
        result = TRUE;
    if ( EQUAL ( pszCap, OLCFastSetNextByIndex ) )
        result = TRUE;
#warning todo CreateField
    if ( EQUAL ( pszCap, OLCCreateField ) )
        result = FALSE;
#warning todo DeleteFeature
    if ( EQUAL ( pszCap, OLCDeleteFeature ) )
        result = FALSE;

    return result;
}
