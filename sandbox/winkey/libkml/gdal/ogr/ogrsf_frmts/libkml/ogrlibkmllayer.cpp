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

OGRLIBKMLLayer::OGRLIBKMLLayer(const char *pszLayerName, 
                               OGRSpatialReference *poSpatialRef,
                               OGRwkbGeometryType eGType)
    : poFeatureDefn(0)
    , fp(0)
    , bUpdate(0)
    , nNextFID(0)
    , isFolder(0)
    , kmlfile(0)
    , poKmlFactory(kmldom::KmlFactory::GetFactory())
{
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
    // TODO: unfinished? mloskot
}

/******************************************************************************

******************************************************************************/

OGRFeature * OGRLIBKMLLayer::GetNextFeature()
{
    // TODO: unfinished? mloskot
    return NULL;
}

/******************************************************************************

******************************************************************************/

OGRFeatureDefn * OGRLIBKMLLayer::GetLayerDefn()
{
    // TODO: unfinished? mloskot
    return poFeatureDefn;
}

/******************************************************************************

******************************************************************************/

OGRErr OGRLIBKMLLayer::SetAttributeFilter(const char * pszFilter)
{
    // TODO: unfinished? mloskot
    return OGRERR_NONE;
}


/******************************************************************************

******************************************************************************/

OGRErr OGRLIBKMLLayer::SetNextByIndex(long nIndex)
{
    // TODO: unfinished? mloskot
    return OGRERR_NONE;
}

/******************************************************************************

******************************************************************************/

OGRFeature *OGRLIBKMLLayer::GetFeature(long nFID)
{
    // TODO: unfinished? mloskot
    return NULL;
}

/******************************************************************************

******************************************************************************/

OGRErr OGRLIBKMLLayer::SetFeature(OGRFeature *poFeature)
{
    CPLAssert(0 != poFeature);

    // TODO: unfinished? mloskot
    return OGRERR_NONE;
}

/******************************************************************************

******************************************************************************/

OGRErr OGRLIBKMLLayer::CreateFeature(OGRFeature *poFeature)
{
    CPLAssert(0 != poFeature);

    PlacemarkPtr poKmlPlacemark = feat2kml(this, poFeature, poKmlFactory);

    if (isFolder)
    {
        poKmlFolder->add_feature(poKmlPlacemark);
    }

    return OGRERR_NONE;
}  

/******************************************************************************

******************************************************************************/

OGRErr OGRLIBKMLLayer::DeleteFeature(long nFID)
{
    // TODO: unfinished? mloskot
    return OGRERR_NONE;
}

/******************************************************************************

******************************************************************************/

OGRSpatialReference * OGRLIBKMLLayer::GetSpatialRef()
{
    return NULL;
}

/******************************************************************************

******************************************************************************/

int OGRLIBKMLLayer::GetFeatureCount(int bForce)
{
    // TODO: unfinished? mloskot
    return 0;
}

/******************************************************************************

******************************************************************************/

OGRErr OGRLIBKMLLayer::GetExtent(OGREnvelope *poExtent, int bForce)
{
    CPLAssert(0 != poExtent);

    // TODO: unfinished? mloskot
    return OGRERR_NONE;
}
    
/******************************************************************************

******************************************************************************/

const char *OGRLIBKMLLayer::GetInfo(const char* pszInfo)
{
    CPLAssert(0 != pszInfo);

    return 0;
}

/******************************************************************************

******************************************************************************/

OGRErr OGRLIBKMLLayer::CreateField(OGRFieldDefn *poField, int bApproxOK)
{
    CPLAssert(0 != poField);

    // TODO: unfinished? mloskot
    return OGRERR_NONE;
}

/******************************************************************************

******************************************************************************/

OGRErr OGRLIBKMLLayer::SyncToDisk()
{
    // TODO: unfinished? mloskot
    return OGRERR_NONE;
}

/******************************************************************************

******************************************************************************/

OGRStyleTable *OGRLIBKMLLayer::GetStyleTable()
{
    return m_poStyleTable;
}

/******************************************************************************

******************************************************************************/

void OGRLIBKMLLayer::SetStyleTableDirectly(OGRStyleTable *poStyleTable)
{
    CPLAssert(0 != poStyleTable);
    
    m_poStyleTable = poStyleTable;
}

/******************************************************************************

******************************************************************************/

void OGRLIBKMLLayer::SetStyleTable(OGRStyleTable *poStyleTable)
{
    CPLAssert(0 != poStyleTable);
}

/******************************************************************************

******************************************************************************/

int OGRLIBKMLLayer::TestCapability(const char * pszCap)
{
    CPLAssert(0 != pszCap);

    int result = FALSE;
    
    if( EQUAL(pszCap,OLCSequentialWrite) )
        result = bUpdate;
    
    return result;
}
