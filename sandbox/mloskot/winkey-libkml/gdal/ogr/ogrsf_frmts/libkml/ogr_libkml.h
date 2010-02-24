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

#include "ogrsf_frmts.h"

#include <kml/engine.h>
#include <kml/dom.h>

class OGRLIBKMLDataSource;
/******************************************************************************
  layer class
******************************************************************************/

class OGRLIBKMLLayer : public OGRLayer
{
    OGRFeatureDefn      *poFeatureDefn;

    FILE                *fp;
    int                  bUpdate;
    int                  nNextFID;
    int                  isFolder;
    //kmlengine::KmlFolderPtr poKmlFolder;
    kmlengine::KmlFile  *kmlfile;
    kmldom::KmlFactory  *poKmlFactory;
    //kmldom::DocumentPtr poKmlDoc_kmlDocument
   
  public:
    OGRLIBKMLLayer       ( const char *pszFilename );
   ~OGRLIBKMLLayer();

    void                 ResetReading();
    OGRFeature *         GetNextFeature();

    OGRFeatureDefn *     GetLayerDefn();
    OGRErr               SetAttributeFilter( const char * );


    OGRErr               SetNextByIndex( long nIndex );
    OGRFeature          *GetFeature( long nFID );
    OGRErr               SetFeature( OGRFeature *poFeature );
    OGRErr               CreateFeature( OGRFeature *poOgrFeat );
    OGRErr               DeleteFeature( long nFID );
 
    OGRSpatialReference *GetSpatialRef();

    int                  GetFeatureCount( int bForce = TRUE );
    OGRErr               GetExtent( OGREnvelope *psExtent,
                                    int bForce = TRUE);

    
    const char          *GetInfo( const char * );

    OGRErr               CreateField( OGRFieldDefn *poField,
                                      int bApproxOK = TRUE );

    OGRErr               SyncToDisk();

    OGRStyleTable       *GetStyleTable();
    void                 SetStyleTableDirectly( OGRStyleTable *poStyleTable );
    void                 SetStyleTable(OGRStyleTable *poStyleTable);

    int                  TestCapability( const char * );
};

/******************************************************************************
  datasource class
******************************************************************************/

class OGRLIBKMLDataSource : public OGRDataSource {
    char            *pszName;
    
    OGRLIBKMLLayer **papoLayers;
    int              nLayers;
    int              bUpdate;
    
    kmlengine::KmzFile *kmzfile;
    kmlengine::KmlFile *kmlfile;
    kmldom::KmlFactory *poKmlFactory;
    //kmldom::DocumentPtr poKmlDoc_kmlDocument
    
  public:
                     OGRLIBKMLDataSource();
                    ~OGRLIBKMLDataSource();

    const char      *GetName() { return pszName; };
    
    int              GetLayerCount() { return nLayers; }
    OGRLayer        *GetLayer(int);
    OGRLayer        *GetLayerByName(const char *);
    OGRErr           DeleteLayer(int);

    
    OGRLayer        *CreateLayer( const char *pszName, 
                                  OGRSpatialReference *poSpatialRef = NULL,
                                  OGRwkbGeometryType eGType = wkbUnknown,
                                  char ** papszOptions = NULL );

    OGRStyleTable   *GetStyleTable();
    void             SetStyleTableDirectly( OGRStyleTable *poStyleTable );
    void             SetStyleTable(OGRStyleTable *poStyleTable);

    int              Open( const char *pszFilename,
                           int bUpdate);
    int              Create ( const char *pszFilename,
                              char **papszOptions );

    int              TestCapability( const char * );
};

/******************************************************************************
  driver class
******************************************************************************/

class OGRLIBKMLDriver : public OGRSFDriver {
    int            bUpdate;
  public:
                  ~OGRLIBKMLDriver();
                
    const char    *GetName();
    OGRDataSource *Open( const char *pszFilename,
                         int bUpdate );
    OGRDataSource *CreateDataSource( const char *pszFilename,
                                     char **papszOptions );

    OGRErr         DeleteDataSource( const char *pszName );

    int            TestCapability( const char * );
};
