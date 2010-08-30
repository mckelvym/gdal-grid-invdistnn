/******************************************************************************
 *
 * Project:  NETCDF Translator
 * Purpose:  Implements OGRNETCDFDriver
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

#include "ogrnetcdf.h"
#include <ogr_feature.h>

#include "ogrnetcdffeat.h"
#include "ogrnetcdffield.h"

#include <netcdf.h>

/******************************************************************************
 Layer constructor
******************************************************************************/

OGRNETCDFLayer::OGRNETCDFLayer( const char *pszFilename, int Ncid )

{
    nNextFID = 0;
    nNcid = Ncid;
    
    poFeatureDefn = new OGRFeatureDefn( CPLGetBasename( pszFilename ) );
    poFeatureDefn->Reference();
    poFeatureDefn->SetGeomType( wkbPoint );
   
    int status = nc_inq(nNcid, &nDims, &nVars, &nGatts, &nUnlimdimid);

    /***** get the total number of features *****/

    status = nc_inq_dimlen(nNcid, nUnlimdimid, &nFeat);

    nc2FeatureDef (Ncid, nVars, nUnlimdimid, poFeatureDefn );

    OGRFieldDefn oFieldTemplate( "Name", OFTString );

    poFeatureDefn->AddFieldDefn( &oFieldTemplate );

    
}

/******************************************************************************
 Layer destructor
******************************************************************************/

OGRNETCDFLayer::~OGRNETCDFLayer()

{
    poFeatureDefn->Release();
}

/******************************************************************************
 Method to get the next feature on the layer

 Args:          none
 
 Returns:       The next feature, or NULL if there is no more
                
******************************************************************************/

OGRFeature *OGRNETCDFLayer::GetNextRawFeature (
     )
{
    OGRFeature *poFeat = NULL;

    if (nNextFID < (int) nFeat) {

        poFeat = GetFeature(nNextFID);
        nNextFID++;
        
    }

    return poFeat;
}

/******************************************************************************
 Method to get the next feature on the layer

 Args:          none
 
 Returns:       The next feature, or NULL if there is no more

 this function copyed from the sqlite driver
******************************************************************************/

OGRFeature *OGRNETCDFLayer::GetNextFeature()

{
    for( ; TRUE; )
    {
        OGRFeature      *poFeature;

        poFeature = GetNextRawFeature();
        if( poFeature == NULL )
            return NULL;

        if( (m_poFilterGeom == NULL
            || FilterGeometry( poFeature->GetGeometryRef() ) )
            && (m_poAttrQuery == NULL
                || m_poAttrQuery->Evaluate( poFeature )) )
            return poFeature;

        delete poFeature;
    }
}


/******************************************************************************
 Layer SetNextByIndex method
******************************************************************************/

OGRErr 	OGRNETCDFLayer::SetNextByIndex (long nIndex) {
    if (nIndex < (int)nFeat + 1)
        nNextFID = nIndex;
    else
        nNextFID = nFeat - 1;

    return OGRERR_NONE;
}

OGRFeature *OGRNETCDFLayer::GetFeature (long nFID) {
    
    OGRFeature *poFeat = nc2feat (nNcid, nVars, nUnlimdimid, nFID, poFeatureDefn, poSRS);

    poFeat->SetFID(nFID);
    

    nNextFID = nFID + 1;
    
    return poFeat;
}



/******************************************************************************
 Layer ResetReading method
******************************************************************************/

void OGRNETCDFLayer::ResetReading()

{
    nNextFID = 0;
}

/******************************************************************************
 Layer GetFeatureCount method
******************************************************************************/

int OGRNETCDFLayer::GetFeatureCount( int bForce ) {

    return (int) nFeat;
}

/******************************************************************************
 Test if capability is available.

 Args:          pszCap  layer capability name to test
 
 Returns:       True if the layer supports the capability, otherwise false

******************************************************************************/

int OGRNETCDFLayer::TestCapability (
    const char *pszCap )
{
    int result = FALSE;

    if ( EQUAL ( pszCap, OLCRandomRead ) )
        result = TRUE;
    if ( EQUAL ( pszCap, OLCSequentialWrite ) )
        result = FALSE;
    if ( EQUAL ( pszCap, OLCRandomWrite ) )
        result = FALSE;
    if ( EQUAL ( pszCap, OLCFastFeatureCount ) )
        result = TRUE;
    if ( EQUAL ( pszCap, OLCFastSetNextByIndex ) )
        result = TRUE;
    if ( EQUAL ( pszCap, OLCCreateField ) )
        result = FALSE;
    if ( EQUAL ( pszCap, OLCDeleteFeature ) )
        result = FALSE;

    return result;
}
