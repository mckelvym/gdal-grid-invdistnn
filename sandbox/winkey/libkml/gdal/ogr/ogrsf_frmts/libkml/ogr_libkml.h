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

using kmldom::KmlFactory;
using kmldom::KmlPtr;
using kmldom::DocumentPtr;
using kmldom::ContainerPtr;
using kmldom::ElementPtr;
using kmlengine::KmzFile;
using kmlengine::KmzFilePtr;

using kmlengine::KmlFile;
using kmlengine::KmlFilePtr;

class OGRLIBKMLDataSource;

/******************************************************************************
  layer class
******************************************************************************/

class OGRLIBKMLLayer:public OGRLayer
{
    OGRFeatureDefn *poFeatureDefn;

    FILE                      fp;
    int                       bUpdate;
    int                       nFeatures;
    int                       iFeature;
    const char                *m_pszLayerName;
    ContainerPtr              m_poKmlLayer;
    ElementPtr                m_poKmlLayerRoot;
    KmlFile                  *m_poKmlKmlfile;

    DocumentPtr               m_poKmlDocument;
    OGRStyleTable            *m_poStyleTable;
    OGRLIBKMLDataSource      *m_poOgrDS;
    OGRFeatureDefn           *m_poOgrFeatureDefn;
    OGRSpatialReference      *m_poOgrSRS;
  public:
    OGRLIBKMLLayer            ( const char *pszLayerName,
                                OGRSpatialReference * poSpatialRef,
                                OGRwkbGeometryType eGType,
                                OGRLIBKMLDataSource *poOgrDS,
                                ElementPtr poKmlRoot);
    ~OGRLIBKMLLayer           (  );

    void                      ResetReading  (  );
    OGRFeature               *GetNextFeature (  );
    OGRFeatureDefn           *GetLayerDefn (  ) { return m_poOgrFeatureDefn; };
    //OGRErr                    SetAttributeFilter (const char * );
    OGRErr                    SetNextByIndex ( long nIndex );
    OGRFeature               *GetFeature ( long nFID );
    OGRErr                    SetFeature ( OGRFeature * poFeature );
    OGRErr                    CreateFeature ( OGRFeature * poOgrFeat );
    OGRErr                    DeleteFeature ( long nFID );

    OGRSpatialReference      *GetSpatialRef (  ) { return m_poOgrSRS; };

    int                       GetFeatureCount ( int bForce = TRUE );
    OGRErr                    GetExtent ( OGREnvelope * psExtent,
                                          int bForce = TRUE );


    //const char               *GetInfo ( const char * );

    OGRErr                    CreateField ( OGRFieldDefn * poField,
                                            int bApproxOK = TRUE );

    OGRErr                    SyncToDisk (  );

    OGRStyleTable            *GetStyleTable (  );
    void                      SetStyleTableDirectly ( OGRStyleTable * poStyleTable );
    void                      SetStyleTable ( OGRStyleTable * poStyleTable );
    const char               *GetName();
    int                       TestCapability ( const char * );
    ContainerPtr              GetKmlLayer () {return m_poKmlLayer;};
};

/******************************************************************************
  datasource class
******************************************************************************/

class OGRLIBKMLDataSource:public OGRDataSource
{
    char                     *pszName;

    /***** layers *****/
    
    OGRLIBKMLLayer          **papoLayers;
    int                       nLayers;
    int                       nAlloced;
    

    int                       bUpdate;
    int                       bUpdated;
    
    /***** for kml files *****/
    int                       m_isKml;
    KmlPtr                    m_poKmlDSKml;
    ContainerPtr              m_poKmlDSContainer;

    /***** for kmz files *****/

    int                       m_isKmz;
    ContainerPtr              m_poKmlDocKml;
    
    /***** the kml factory *****/
    
    KmlFactory               *m_poKmlFactory;
    
    /***** style table pointer *****/
    
    OGRStyleTable            *m_poStyleTable;

  public:
    OGRLIBKMLDataSource       (  );
    ~OGRLIBKMLDataSource      (  );

    const char               *GetName (  ) { return pszName; };

    int                       GetLayerCount (  ) { return nLayers; }
    OGRLayer                 *GetLayer ( int );
    OGRLayer                 *GetLayerByName ( const char * );
    OGRErr                    DeleteLayer ( int );


    OGRLayer                 *CreateLayer ( const char *pszName,
                                            OGRSpatialReference * poSpatialRef = NULL,
                                            OGRwkbGeometryType eGType = wkbUnknown,
                                            char **papszOptions = NULL );

    OGRStyleTable            *GetStyleTable (  );
    void                      SetStyleTableDirectly ( OGRStyleTable * poStyleTable );
    void                      SetStyleTable ( OGRStyleTable * poStyleTable );

    int                       Open ( const char *pszFilename,
                                     int bUpdate );
    int                       Create ( const char *pszFilename,
                                       char **papszOptions );

    int                       TestCapability (const char * );
    KmlFactory               *GetKmlFactory() { return m_poKmlFactory; };
    //KmzFile                  *GetKmz() { return m_poKmlKmzfile; };
    int                       IsKml() {return m_isKml;};
    int                       IsKmz() {return m_isKmz;};
  private:

    /***** methods to write out various datasource types at destroy *****/

    void                      WriteKml();
    void                      WriteKmz();
    
    /***** methods to open various datasource types *****/
        
    int                       OpenKmz ( const char *pszFilename,
                                        int bUpdate );
    int                       OpenKml ( const char *pszFilename,
                                        int bUpdate );
    int                       OpenDir ( const char *pszFilename,
                                        int bUpdate );

    /***** methods to create various datasource types *****/
    
    int                       CreateKml ( const char *pszFilename,
                                          char **papszOptions );
    int                       CreateKmz ( const char *pszFilename,
                                          char **papszOptions );
    int                       CreateDir ( const char *pszFilename,
                                          char **papszOptions );

    /***** methods to create layers on various datasource types *****/
    
    OGRLayer                 *CreateLayerKml ( const char *pszLayerName,
                                               OGRSpatialReference * poOgrSRS,
                                               OGRwkbGeometryType eGType,
                                               char **papszOptions );
        
    /***** methods to write a styletable to various datasource types *****/

    void                      SetStyleTable2Kml ( OGRStyleTable * poStyleTable );
    
     
};


/******************************************************************************
  driver class
******************************************************************************/

class OGRLIBKMLDriver:public OGRSFDriver
{
    int bUpdate;
  public:
    ~OGRLIBKMLDriver          (  );

    const char               *GetName (  );
    OGRDataSource            *Open ( const char *pszFilename,
                                     int bUpdate );
    OGRDataSource            *CreateDataSource ( const char *pszFilename,
                                                 char **papszOptions );

    OGRErr                    DeleteDataSource ( const char *pszName );

    int                       TestCapability ( const char * );
};
