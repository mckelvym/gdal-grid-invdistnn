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

#include "ogrlibkmlfeature.h"

/******************************************************************************

******************************************************************************/

OGRLIBKMLLayer::OGRLIBKMLLayer(
  const char *pszLayerName, 
  OGRSpatialReference *poSpatialRef,
  OGRwkbGeometryType eGType,
  KmlFactory *poKmlMyFactory)
{

  poKmlFactory = poKmlMyFactory;

  
}

/******************************************************************************

******************************************************************************/

OGRLIBKMLLayer::~OGRLIBKMLLayer()
{


}

/******************************************************************************

******************************************************************************/

void OGRLIBKMLLayer::ResetReading()
{


}

/******************************************************************************

******************************************************************************/

OGRFeature * OGRLIBKMLLayer::GetNextFeature()
{


}

/******************************************************************************

******************************************************************************/

OGRFeatureDefn * OGRLIBKMLLayer::GetLayerDefn()
{

  return poFeatureDefn;
}

/******************************************************************************

******************************************************************************/

OGRErr OGRLIBKMLLayer::SetAttributeFilter(
  const char *)
{


}


/******************************************************************************

******************************************************************************/

OGRErr OGRLIBKMLLayer::SetNextByIndex(
  long nIndex)
{


}

/******************************************************************************

******************************************************************************/

OGRFeature *OGRLIBKMLLayer::GetFeature(
  long nFID)
{


}

/******************************************************************************

******************************************************************************/

OGRErr OGRLIBKMLLayer::SetFeature(
  OGRFeature *poFeature)
{


}

/******************************************************************************

******************************************************************************/

OGRErr OGRLIBKMLLayer::CreateFeature(
  OGRFeature *poOgrFeat)
{

  PlacemarkPtr poKmlPlacemark = feat2kml(this, poOgrFeat, poKmlFactory);

  if (isFolder)
    poKmlFolder->add_feature(poKmlPlacemark);
  

/******************************************************************************

******************************************************************************/

OGRErr OGRLIBKMLLayer::DeleteFeature(
  long nFID )
{


}

/******************************************************************************

******************************************************************************/

OGRSpatialReference * OGRLIBKMLLayer::GetSpatialRef()
{

  return NULL;
}

/******************************************************************************

******************************************************************************/

int OGRLIBKMLLayer::GetFeatureCount(
  int bForce)
{


}

/******************************************************************************

******************************************************************************/

OGRErr OGRLIBKMLLayer::GetExtent(
  OGREnvelope *psExtent,
  int bForce)
{


}

    
/******************************************************************************

******************************************************************************/

const char *OGRLIBKMLLayer::GetInfo(
  const char * )
{


}

/******************************************************************************

******************************************************************************/

OGRErr OGRLIBKMLLayer::CreateField(
  OGRFieldDefn *poField,
  int bApproxOK)
{


}


/******************************************************************************

******************************************************************************/

OGRErr OGRLIBKMLLayer::SyncToDisk()
{


}

/******************************************************************************

******************************************************************************/

OGRStyleTable *OGRLIBKMLLayer::GetStyleTable()
{

  return m_poStyleTable;
}

/******************************************************************************

******************************************************************************/

void OGRLIBKMLLayer::SetStyleTableDirectly(
  OGRStyleTable *poStyleTable )
{

  m_poStyleTable = poStyleTable;
  
  
}

/******************************************************************************

******************************************************************************/

void OGRLIBKMLLayer::SetStyleTable(
    OGRStyleTable *poStyleTable)
{
  
  
}

/******************************************************************************

******************************************************************************/

int OGRLIBKMLLayer::TestCapability(
  const char * pszCap)
{
  int result = FALSE;
  
  if( EQUAL(pszCap,OLCSequentialWrite))
    result = bUpdate;

  return result;
}
