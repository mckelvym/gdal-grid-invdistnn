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

//#include "cpl_conv.h"
//#include "cpl_string.h"
//#include "cpl_error.h"
#include <iostream>
//#include <sstream>
#include <kml/dom.h>
#include <kml/base/file.h>

using kmldom::KmlFactory;
using kmldom::DocumentPtr;
using kmldom::FolderPtr;
using kmldom::FeaturePtr;
using kmldom::NetworkLinkPtr;
using kmldom::StyleSelectorPtr;
using kmldom::LinkPtr;
using kmldom::SchemaPtr;
using kmlbase::File;
using kmldom::KmlPtr;

#include "ogr_libkml.h"
#include "ogrlibkmlstyle.h"

/******************************************************************************
 OGRLIBKMLDataSource()
******************************************************************************/

OGRLIBKMLDataSource::OGRLIBKMLDataSource (  )
{
    pszName = NULL;
    papoLayers = NULL;
    nLayers = 0;
    nAlloced = 0;

    bUpdated = FALSE;

    m_isKml = FALSE;
    m_poKmlDSKml = NULL;
    m_poKmlDSContainer = NULL;

    m_isKmz = FALSE;
    m_poKmlDocKml = NULL;

    m_poKmlFactory = KmlFactory::GetFactory (  );
    
    //m_poStyleTable = NULL;

}

/******************************************************************************
 method to write a single file ds .kml at ds destroy
******************************************************************************/

void OGRLIBKMLDataSource::WriteKml (
     )
{
    std::string oKmlFilename = pszName;
    if ( m_poKmlDSKml ) {
        std::string oKmlOut = kmldom::SerializePretty ( m_poKmlDSKml );

        if ( !kmlbase::File::WriteStringToFile ( oKmlOut, oKmlFilename ) )
            CPLError ( CE_Failure, CPLE_FileIO,
                       "ERROR writing %s", oKmlFilename.c_str (  ) );
    }
    else if ( m_poKmlDSContainer ) {
        std::string oKmlOut = kmldom::SerializePretty ( m_poKmlDSContainer );

        if ( !kmlbase::File::WriteStringToFile ( oKmlOut, oKmlFilename ) )
            CPLError ( CE_Failure, CPLE_FileIO,
                       "ERROR writing %s", oKmlFilename.c_str (  ) );

    }

    return;
}

/******************************************************************************
 method to write a ds .kmz at ds destroy
******************************************************************************/

void OGRLIBKMLDataSource::WriteKmz (
     )
{

    KmzFile *poKmlKmzfile = kmlengine::KmzFile::Create ( pszName );

    if ( !poKmlKmzfile ) {
        CPLError ( CE_Failure, CPLE_NoWriteAccess, "ERROR creating %s",
                   pszName );
        return;
    }

    /***** write out the doc.kml ****/

    const char *pszUseDocKml =
        CPLGetConfigOption ( "LIBKML_USE_DOC.KML", "yes" );

    if ( EQUAL ( pszUseDocKml, "yes" ) && m_poKmlDocKml ) {

        KmlPtr poKmlKml = m_poKmlFactory->CreateKml (  );

        poKmlKml->set_feature ( m_poKmlDocKml );
        std::string oKmlOut = kmldom::SerializePretty ( poKmlKml );

        if ( !poKmlKmzfile->AddFile ( oKmlOut, "doc.kml" ) )
            CPLError ( CE_Failure, CPLE_FileIO,
                       "ERROR adding %s to %s", "doc.kml", pszName );

    }

    /***** loop though the layers and write them *****/

    int iLayer;

    for ( iLayer = 0; iLayer < nLayers; iLayer++ ) {
        ContainerPtr poKlmContainer = papoLayers[iLayer]->GetKmlLayer (  );

        KmlPtr poKmlKml = m_poKmlFactory->CreateKml (  );

        poKmlKml->set_feature ( poKlmContainer );

        std::string oKmlOut = kmldom::SerializePretty ( poKmlKml );
        std::string oName = papoLayers[iLayer]->GetName (  );
        oName.append ( ".kml" );

        if ( !poKmlKmzfile->AddFile ( oKmlOut, oName.c_str (  ) ) )
            CPLError ( CE_Failure, CPLE_FileIO,
                       "ERROR adding %s to %s", oName.c_str (  ), pszName );

    }

   /***** write the style table *****/

    if ( m_poKmlStyleKml ) {
        
        KmlPtr poKmlKml = m_poKmlFactory->CreateKml (  );

        poKmlKml->set_feature ( m_poKmlStyleKml );
        std::string strKmlOut = kmldom::SerializePretty ( poKmlKml );

        if ( !poKmlKmzfile->AddFile ( strKmlOut, "style/style.kml" ) )
            CPLError ( CE_Failure, CPLE_FileIO,
                       "ERROR adding %s to %s", "style/style.kml", pszName );
    }

    delete poKmlKmzfile;

    return;
}

/******************************************************************************
 ~OGRLIBKMLDataSource()
******************************************************************************/

OGRLIBKMLDataSource::~OGRLIBKMLDataSource (  )
{

    /***** some types of output require the write to happen at the very end *****/

    /***** kml *****/

    if ( bUpdate && bUpdated && IsKml (  ) )
        WriteKml (  );

    /***** kmz *****/

    else if ( bUpdate && bUpdated && IsKmz (  ) ) {
        WriteKmz (  );
    }




    CPLFree ( pszName );


    for ( int i = 0; i < nLayers; i++ )
        delete papoLayers[i];

    CPLFree ( papoLayers );

    //delete m_poStyleTable;
    

}


/******************************************************************************
 method to parse a schemas out of a document
******************************************************************************/

void OGRLIBKMLDataSource::ParseSchemas (
    DocumentPtr poKmlDocument )
{


    size_t nKmlSchemas = poKmlDocument->get_schema_array_size (  );
    size_t iKmlSchema;
    
    papoKmlSchema = ( SchemaPtr * ) CPLMalloc ( sizeof ( SchemaPtr ) * nKmlSchemas);

    /***** loop over the Schemas and store them in the array *****/
        
    for ( iKmlSchema = 0; iKmlSchema < nKmlSchemas; iKmlSchema++ ) {

        papoKmlSchema[iKmlSchema] = poKmlDocument->get_schema_array_at ( iKmlSchema );
    }
    
    return;
}

/******************************************************************************
 method to parse multiple layers out of a container

 returns number of features in the container that are NOT another container
******************************************************************************/

int OGRLIBKMLDataSource::ParseLayers (
    ContainerPtr poKmlContainer,
    OGRSpatialReference * poOgrSRS )
{
    int nResult = 0;

    size_t nKmlFeatures = poKmlContainer->get_feature_array_size (  );

    /***** alocate memory for the layer array *****/

    papoLayers =
        ( OGRLIBKMLLayer ** ) CPLMalloc ( sizeof ( OGRLIBKMLLayer * ) *
                                          nKmlFeatures );

    nAlloced = nKmlFeatures;

    /***** loop over the container to seperate the style, layers, etc *****/

    size_t iKmlFeature;

    for ( iKmlFeature = 0; iKmlFeature < nKmlFeatures; iKmlFeature++ ) {
        FeaturePtr poKmlFeat =
            poKmlContainer->get_feature_array_at ( iKmlFeature );

        /***** container *****/

        if ( poKmlFeat->IsA ( kmldom::Type_Container ) ) {

            /***** see if the container has a name *****/

            std::string oKmlFeatName;
            if ( poKmlFeat->has_name (  ) ) {
                oKmlFeatName = poKmlFeat->get_name (  );
            }

            /***** use the feature index number as the name *****/
            /***** not sure i like this c++ ich *****/

            else {
                std::stringstream oOut;
                oOut << iKmlFeature;
                oKmlFeatName = oOut.str (  );
            }

            /***** create the layer *****/

#warning we need a way to pass schema data for a layerdefn
            OGRLIBKMLLayer *poOgrLayer =
                new OGRLIBKMLLayer ( oKmlFeatName.c_str (  ),
                                     poOgrSRS, wkbUnknown, this,
                                     poKmlFeat );

            papoLayers[nLayers++] = poOgrLayer;
        }

        else
            nResult++;
    }

    return nResult;
}

/******************************************************************************
 function to get the container from the kmlroot
******************************************************************************/

ContainerPtr GetContainerFromRoot (
    ElementPtr poKmlRoot )
{
    ContainerPtr poKmlContainer = NULL;

    if ( poKmlRoot ) {

        /***** skip over the <kml> we want the container *****/

        if ( poKmlRoot->IsA ( kmldom::Type_kml ) ) {

            KmlPtr poKmlKml = AsKml ( poKmlRoot );

            if ( poKmlKml->has_feature (  ) ) {
                FeaturePtr poKmlFeat = poKmlKml->get_feature (  );

                if ( poKmlFeat->IsA ( kmldom::Type_Container ) )
                    poKmlContainer = AsContainer ( poKmlFeat );
            }

        }

        else if ( poKmlRoot->IsA ( kmldom::Type_Container ) )
            poKmlContainer = AsContainer ( poKmlRoot );
    }

    return poKmlContainer;
}

/******************************************************************************
 method to open a kml file
******************************************************************************/

int OGRLIBKMLDataSource::OpenKml (
    const char *pszFilename,
    int bUpdate )
{
    std::string oKmlKml;

    if ( !kmlbase::File::ReadFileToString ( pszFilename, &oKmlKml ) ) {
        CPLError ( CE_Failure, CPLE_OpenFailed,
                   "%s is not a valid kml file", pszFilename );
        return FALSE;
    }

    /***** create a SRS                                    *****/
    /***** this was shamelessly swiped from the kml driver *****/

    OGRSpatialReference *poOgrSRS =
        new OGRSpatialReference ( "GEOGCS[\"WGS 84\", "
                                  "   DATUM[\"WGS_1984\","
                                  "       SPHEROID[\"WGS 84\",6378137,298.257223563,"
                                  "           AUTHORITY[\"EPSG\",\"7030\"]],"
                                  "           AUTHORITY[\"EPSG\",\"6326\"]],"
                                  "       PRIMEM[\"Greenwich\",0,"
                                  "           AUTHORITY[\"EPSG\",\"8901\"]],"
                                  "       UNIT[\"degree\",0.01745329251994328,"
                                  "           AUTHORITY[\"EPSG\",\"9122\"]],"
                                  "           AUTHORITY[\"EPSG\",\"4326\"]]" );



    /***** parse the kml into the DOM *****/

    std::string oKmlErrors;

    ElementPtr poKmlRoot = kmldom::Parse ( oKmlKml, &oKmlErrors );

    if ( !poKmlRoot ) {
        CPLError ( CE_Failure, CPLE_OpenFailed,
                   "ERROR Parseing kml %s :%s",
                   pszFilename, oKmlErrors.c_str (  ) );
        delete poOgrSRS;

        return FALSE;
    }

    /***** get the container from root  *****/

    if ( !( m_poKmlDSContainer = GetContainerFromRoot ( poKmlRoot ) ) ) {
        CPLError ( CE_Failure, CPLE_OpenFailed,
                   "ERROR Parseing kml %s :%s %s",
                   pszFilename, "This file does not fit the OGR model,",
                   "there is no container element at the root." );
        delete poOgrSRS;

        return FALSE;
    }

    /***** get the styles *****/

    ParseStyles ( AsDocument ( m_poKmlDSContainer ), &m_poStyleTable );

    /***** parse for schemas *****/

    ParseSchemas( AsDocument ( m_poKmlDSContainer ) );

    /***** parse for layers *****/

    int nPlacemarks = ParseLayers ( m_poKmlDSContainer, poOgrSRS );

    /***** if there is placemarks in the root its a layer *****/

    if ( nPlacemarks && !nLayers ) {

#warning the layer name needs to be the basename of the file
#warning we need a way to pass schema data for a layerdefn
        OGRLIBKMLLayer *poOgrLayer = new OGRLIBKMLLayer ( pszFilename,
                                                          poOgrSRS, wkbUnknown,
                                                          this,
                                                          m_poKmlDSContainer );


        papoLayers[nLayers++] = poOgrLayer;
    }

    delete poOgrSRS;

    return TRUE;
}

/******************************************************************************
 method to open a kmz file
******************************************************************************/


int OGRLIBKMLDataSource::OpenKmz (
    const char *pszFilename,
    int bUpdate )
{

    KmzFile *poKmlKmzfile = KmzFile::OpenFromFile ( pszFilename );

    if ( !poKmlKmzfile ) {
        CPLError ( CE_Failure, CPLE_OpenFailed,
                   "%s is not a valid kmz file", pszFilename );
        return FALSE;
    }

    /***** read the doc.kml *****/

    std::string oKmlKml;
    std::string oKmlKmlPath;
    if ( !poKmlKmzfile->ReadKmlAndGetPath ( &oKmlKml, &oKmlKmlPath ) ) {

        return FALSE;
    }

    /***** create a SRS                                    *****/
    /***** this was shamelessly swiped from the kml driver *****/

    OGRSpatialReference *poOgrSRS =
        new OGRSpatialReference ( "GEOGCS[\"WGS 84\", "
                                  "   DATUM[\"WGS_1984\","
                                  "       SPHEROID[\"WGS 84\",6378137,298.257223563,"
                                  "           AUTHORITY[\"EPSG\",\"7030\"]],"
                                  "           AUTHORITY[\"EPSG\",\"6326\"]],"
                                  "       PRIMEM[\"Greenwich\",0,"
                                  "           AUTHORITY[\"EPSG\",\"8901\"]],"
                                  "       UNIT[\"degree\",0.01745329251994328,"
                                  "           AUTHORITY[\"EPSG\",\"9122\"]],"
                                  "           AUTHORITY[\"EPSG\",\"4326\"]]" );


    /***** parse the kml into the DOM *****/

    std::string oKmlErrors;
    ElementPtr poKmlRoot = kmldom::Parse ( oKmlKml, &oKmlErrors );

    if ( !poKmlRoot ) {
        CPLError ( CE_Failure, CPLE_OpenFailed,
                   "ERROR Parseing kml layer %s from %s :%s",
                   oKmlKmlPath.c_str (  ),
                   pszFilename, oKmlErrors.c_str (  ) );
        delete poOgrSRS;

        return FALSE;
    }

    /***** get the child contianer from root *****/
    
    ContainerPtr poKmlContainer = GetContainerFromRoot (poKmlRoot);

    /***** loop over the container looking for network links *****/

    size_t nKmlFeatures = poKmlContainer->get_feature_array_size (  );
    size_t iKmlFeature;
    int nLinks = 0;
    for ( iKmlFeature = 0; iKmlFeature < nKmlFeatures; iKmlFeature++ ) {
        FeaturePtr poKmlFeat =
            poKmlContainer->get_feature_array_at ( iKmlFeature );

        /***** is it a network link? *****/
        
        if (!poKmlFeat->IsA (kmldom::Type_NetworkLink))
            continue;

        NetworkLinkPtr poKmlNetworkLink = AsNetworkLink ( poKmlFeat );

        /***** does it have a link? *****/

        if ( !poKmlNetworkLink->has_link() )
            continue;

        LinkPtr poKmlLink = poKmlNetworkLink->get_link ();

        /***** does the link have a href? *****/

        if ( !poKmlLink->has_href ())
            continue;

        kmlengine::Href *poKmlHref = new kmlengine::Href(poKmlLink->get_href());

        /***** is the link relative? *****/

        if (poKmlHref->IsRelativePath (  ) ) {

            nLinks++;

            /***** alocate memory for the layers if we have not allready *****/
            
            if (!papoLayers) {
                papoLayers = ( OGRLIBKMLLayer ** ) CPLMalloc ( sizeof ( OGRLIBKMLLayer * ) * nKmlFeatures );

                nAlloced = nKmlFeatures;
            }

            std::string oKml;
            if ( poKmlKmzfile-> ReadFile ( poKmlHref->get_path (  ).c_str (  ), &oKml ) ) {

                /***** parse the kml into the DOM *****/

                std::string oKmlErrors;
                ElementPtr poKmlLyrRoot = kmldom::Parse ( oKml, &oKmlErrors );

                if ( !poKmlLyrRoot ) {
                    CPLError ( CE_Failure, CPLE_OpenFailed,
                               "ERROR Parseing kml layer %s from %s :%s",
                               poKmlHref->get_path (  ).c_str (  ),
                               pszFilename, oKmlErrors.c_str (  ) );
                        delete poKmlHref;
                        
                        continue;
                    }

                /***** get the container from root  *****/

                ContainerPtr poKmlLyrContainer = GetContainerFromRoot ( poKmlLyrRoot );
  
                /***** create the layer *****/

                OGRLIBKMLLayer *poOgrLayer =
                    new OGRLIBKMLLayer ( poKmlHref->get_path (  ).c_str (  ),
                                         poOgrSRS, wkbUnknown, this,
                                         poKmlLyrContainer );


                papoLayers[nLayers++] = poOgrLayer;

                /***** cleanup *****/

                delete poKmlHref;

            }
        }
    }

    /***** if the doc.kml has no links treat it as a normal kml file *****/

    if (!nLinks) {

#warning there could still be a seperate styles file in the kmz
#warning if there is this would be a layer style table IF its only a single layer
        
        /***** get the styles *****/

        ParseStyles ( AsDocument ( poKmlContainer ), &m_poStyleTable );

        /***** parse for schemas *****/

        ParseSchemas( AsDocument ( poKmlContainer ) );

        /***** parse for layers *****/

        int nPlacemarks = ParseLayers ( poKmlContainer, poOgrSRS );

        /***** if there is placemarks in the root its a layer *****/

        if ( nPlacemarks && !nLayers ) {

#warning the layer name needs to be the basename of the file
#warning we need a way to pass schema data for a layerdefn

            /***** alocate memory for the layer array *****/


            papoLayers =
                ( OGRLIBKMLLayer ** ) CPLMalloc ( sizeof ( OGRLIBKMLLayer * ) );
            
            OGRLIBKMLLayer *poOgrLayer = new OGRLIBKMLLayer ( pszFilename,
                                                              poOgrSRS, wkbUnknown,
                                                              this,
                                                              poKmlContainer );


            papoLayers[nLayers++] = poOgrLayer;
        }
    }

#warning i think we should do this sooner
    
    /***** read the style table if it has one *****/

    std::string oKmlStyleKml;
    if ( poKmlKmzfile->ReadFile ( "style/style.kml", &oKmlStyleKml ) ) {

        /***** parse the kml into the dom *****/

        std::string oKmlErrors;
        ElementPtr poKmlRoot = kmldom::Parse ( oKmlStyleKml, &oKmlErrors );

        if ( poKmlRoot ) {

            ContainerPtr poKmlContainer;

            if ( ( poKmlContainer = GetContainerFromRoot ( poKmlRoot ) ) )
                ParseStyles ( AsDocument ( poKmlContainer ), &m_poStyleTable );
        }
    }

#warning can schemas be stored in a seperate file?

    /***** cleanup *****/
    delete poOgrSRS;
    
    delete poKmlKmzfile;
    
    return TRUE;
}

/******************************************************************************
 method to open a dir
******************************************************************************/

int OGRLIBKMLDataSource::OpenDir (
    const char *pszFilename,
    int bUpdate )
{

    char **papszDirList = NULL;

    if ( !( papszDirList = VSIReadDir ( pszFilename ) ) )
        return FALSE;

   /***** create a SRS                                    *****/
   /***** this was shamelessly swiped from the kml driver *****/

    OGRSpatialReference *poOgrSRS =
        new OGRSpatialReference ( "GEOGCS[\"WGS 84\", "
                                  "   DATUM[\"WGS_1984\","
                                  "       SPHEROID[\"WGS 84\",6378137,298.257223563,"
                                  "           AUTHORITY[\"EPSG\",\"7030\"]],"
                                  "           AUTHORITY[\"EPSG\",\"6326\"]],"
                                  "       PRIMEM[\"Greenwich\",0,"
                                  "           AUTHORITY[\"EPSG\",\"8901\"]],"
                                  "       UNIT[\"degree\",0.01745329251994328,"
                                  "           AUTHORITY[\"EPSG\",\"9122\"]],"
                                  "           AUTHORITY[\"EPSG\",\"4326\"]]" );



    int nFiles = CSLCount ( papszDirList );

    /***** alocate memory for the layer array *****/

    papoLayers =
        ( OGRLIBKMLLayer ** ) CPLMalloc ( sizeof ( OGRLIBKMLLayer * ) *
                                          nFiles );

    int iFile;

    for ( iFile = 0; iFile < nFiles; iFile++ ) {

        if ( !EQUAL ( CPLGetExtension ( papszDirList[iFile] ), "kml" ) )
            continue;

        /***** read the file *****/
        std::string oKmlKml;
        const char *pszFilePath =
            CPLFormFilename ( pszFilename, papszDirList[iFile], NULL );

        if ( !kmlbase::File::ReadFileToString ( pszFilePath, &oKmlKml ) ) {
            CPLError ( CE_Failure, CPLE_OpenFailed,
                       "%s is not a valid kml file", pszFilename );
            continue;
        }

        /***** parse the kml into the DOM *****/

        std::string oKmlErrors;
        ElementPtr poKmlRoot = kmldom::Parse ( oKmlKml, &oKmlErrors );

        if ( !poKmlRoot ) {
            CPLError ( CE_Failure, CPLE_OpenFailed,
                       "ERROR Parseing kml layer %s from %s :%s",
                       pszFilePath, pszFilename, oKmlErrors.c_str (  ) );

            continue;
        }

        /***** get the cintainer from the root *****/

        ContainerPtr poKmlContainer;

        if ( !( poKmlContainer = GetContainerFromRoot ( poKmlRoot ) ) ) {
            CPLError ( CE_Failure, CPLE_OpenFailed,
                       "ERROR Parseing kml %s :%s %s",
                       pszFilename,
                       "This file does not fit the OGR model,",
                       "there is no container element at the root." );
            continue;
        }

        /***** create the layer *****/

        OGRLIBKMLLayer *poOgrLayer = new OGRLIBKMLLayer ( pszFilePath,
                                                          poOgrSRS, wkbUnknown,
                                                          this,
                                                          poKmlContainer );


        papoLayers[nLayers++] = poOgrLayer;
    }

    delete poOgrSRS;

    return TRUE;
}

/******************************************************************************
 Open()
******************************************************************************/

int OGRLIBKMLDataSource::Open (
    const char *pszFilename,
    int bUpdate )
{

    this->bUpdate = bUpdate;
    pszName = CPLStrdup ( pszFilename );

    /***** dir *****/

    VSIStatBufL sStatBuf = { };
    if ( !VSIStatL ( pszFilename, &sStatBuf ) &&
         VSI_ISDIR ( sStatBuf.st_mode ) )
        return OpenDir ( pszFilename, bUpdate );

   /***** kml *****/

    else if ( EQUAL ( CPLGetExtension ( pszFilename ), "kml" ) )
        return OpenKml ( pszFilename, bUpdate );

    /***** kmz *****/

    else if ( EQUAL ( CPLGetExtension ( pszFilename ), "kmz" ) )
        return OpenKmz ( pszFilename, bUpdate );

    else
        return FALSE;

    return TRUE;
}

/******************************************************************************
 method to create a single file .kml ds
******************************************************************************/

int OGRLIBKMLDataSource::CreateKml (
    const char *pszFilename,
    char **papszOptions )
{
    
    m_poKmlDSKml = m_poKmlFactory->CreateKml (  );
    DocumentPtr poKmlDocument = m_poKmlFactory->CreateDocument (  );

    m_poKmlDSKml->set_feature ( poKmlDocument );
    m_poKmlDSContainer = poKmlDocument;
    m_isKml = TRUE;
    bUpdated = TRUE;
    
    return true;
}

/******************************************************************************
 method to create a .kmz ds
******************************************************************************/

int OGRLIBKMLDataSource::CreateKmz (
    const char *pszFilename,
    char **papszOptions )
{


    /***** create the doc.kml  *****/

    const char *namefield = CPLGetConfigOption ( "LIBKML_USE_DOC.KML", "yes" );

    if ( !strcmp ( namefield, "yes" ) ) {
        m_poKmlDocKml = m_poKmlFactory->CreateDocument (  );
    }

    m_isKmz = TRUE;
    bUpdated = TRUE;
    
    return TRUE;
}

/******************************************************************************
 Create()


 env vars:
  LIBKML_USE_DOC.KML         default: yes
 
******************************************************************************/

int OGRLIBKMLDataSource::Create (
    const char *pszFilename,
    char **papszOptions )
{

    int bResult = FALSE;

    pszName = CPLStrdup ( pszFilename );
    bUpdate = TRUE;

    /***** kml *****/

    if ( EQUAL ( CPLGetExtension ( pszFilename ), "kml" ) ) {
        bResult = CreateKml ( pszFilename, papszOptions );
    }

    /***** kmz *****/

    if ( EQUAL ( CPLGetExtension ( pszFilename ), "kmz" ) ) {
        bResult = CreateKmz ( pszFilename, papszOptions );
    }

    /***** dir *****/

    else {


    }

    return bResult;
}

/******************************************************************************
 GetLayer()
******************************************************************************/

OGRLayer *OGRLIBKMLDataSource::GetLayer (
    int iLayer )
{
    if ( iLayer < 0 || iLayer >= nLayers )
        return NULL;
    else
        return papoLayers[iLayer];

}

/******************************************************************************
 GetLayerByName()
******************************************************************************/

OGRLayer *OGRLIBKMLDataSource::GetLayerByName (
    const char *pszname )
{
    int iLayer = 0;

    for ( iLayer = 0; iLayer < nLayers; iLayer++ ) {
        if ( EQUAL ( pszname, papoLayers[iLayer]->GetName (  ) ) )
            return papoLayers[iLayer];
    }

    return NULL;
}


/******************************************************************************
 DeleteLayer()
******************************************************************************/
OGRErr OGRLIBKMLDataSource::DeleteLayer (
    int )
{

#warning we need to figure this out

    return OGRERR_UNSUPPORTED_OPERATION;
}

/******************************************************************************
 method to create a layer in a single file .kml
******************************************************************************/

OGRLayer *OGRLIBKMLDataSource::CreateLayerKml (
    const char *pszLayerName,
    OGRSpatialReference * poOgrSRS,
    OGRwkbGeometryType eGType,
    char **papszOptions )
{

    OGRLIBKMLLayer *poOgrLayer = NULL;

    FolderPtr poKmlFolder = m_poKmlFactory->CreateFolder (  );

    m_poKmlDSContainer->add_feature ( poKmlFolder );

    /***** create the layer *****/

    poOgrLayer = new OGRLIBKMLLayer ( pszLayerName, poOgrSRS, eGType, this,
                                      poKmlFolder );

    /***** check to see if we have enough space to store the layer *****/

    if ( nLayers == nAlloced ) {
        void *tmp = CPLRealloc ( papoLayers,
                                 sizeof ( OGRLIBKMLLayer * ) * ++nAlloced );

        papoLayers = ( OGRLIBKMLLayer ** ) tmp;
    }

    /***** add the layer to the array of layers *****/

    papoLayers[nLayers++] = poOgrLayer;

    return ( OGRLayer * ) poOgrLayer;
}

/******************************************************************************
 method to create a layer in a .kmz
******************************************************************************/

OGRLayer *OGRLIBKMLDataSource::CreateLayerKmz (
    const char *pszLayerName,
    OGRSpatialReference * poOgrSRS,
    OGRwkbGeometryType eGType,
    char **papszOptions )
{

    /***** add a network link to doc.kml *****/

    const char *pszUseDocKml =
        CPLGetConfigOption ( "LIBKML_USE_DOC.KML", "yes" );

    if ( EQUAL ( pszUseDocKml, "yes" ) && m_poKmlDocKml ) {

        DocumentPtr poKmlDocument =
            boost::static_pointer_cast < kmldom::Document > ( m_poKmlDocKml );

        NetworkLinkPtr poKmlNetLink = m_poKmlFactory->CreateNetworkLink (  );
        LinkPtr poKmlLink = m_poKmlFactory->CreateLink (  );

        std::string oHref;
        oHref.append ( pszLayerName );
        oHref.append ( ".kml" );
        poKmlLink->set_href ( oHref );

        poKmlNetLink->set_link ( poKmlLink );
        poKmlDocument->add_feature ( poKmlNetLink );

    }

    /***** create the layer *****/

    OGRLIBKMLLayer *poOgrLayer = NULL;

    DocumentPtr poKmlDocument = m_poKmlFactory->CreateDocument (  );

    /***** create the layer *****/

    poOgrLayer = new OGRLIBKMLLayer ( pszLayerName, poOgrSRS, eGType, this,
                                      poKmlDocument );

    /***** check to see if we have enough space to store the layer *****/

    if ( nLayers == nAlloced ) {
        void *tmp = CPLRealloc ( papoLayers,
                                 sizeof ( OGRLIBKMLLayer * ) * ++nAlloced );

        papoLayers = ( OGRLIBKMLLayer ** ) tmp;
    }

    /***** add the layer to the array of layers *****/

    papoLayers[nLayers++] = poOgrLayer;

    return ( OGRLayer * ) poOgrLayer;
}

/******************************************************************************
 CreateLayer()
******************************************************************************/

OGRLayer *OGRLIBKMLDataSource::CreateLayer (
    const char *pszLayerName,
    OGRSpatialReference * poOgrSRS,
    OGRwkbGeometryType eGType,
    char **papszOptions )
{

    OGRLayer *poOgrLayer = NULL;

    /***** kml DS *****/

    if ( IsKml (  ) ) {
        poOgrLayer = CreateLayerKml ( pszLayerName, poOgrSRS,
                                      eGType, papszOptions );

    }

    else if ( IsKmz (  ) ) {
        poOgrLayer = CreateLayerKmz ( pszLayerName, poOgrSRS,
                                      eGType, papszOptions );

    }




    /***** mark the dataset as updated *****/

    if ( poOgrLayer )
        bUpdated = TRUE;

    return poOgrLayer;
}

/******************************************************************************

******************************************************************************/

OGRStyleTable *OGRLIBKMLDataSource::GetStyleTable (
     )
{

    return m_poStyleTable;
}

/******************************************************************************
    method to write a style table to a single file .kml ds
******************************************************************************/

void OGRLIBKMLDataSource::SetStyleTable2Kml (
    OGRStyleTable * poStyleTable )
{

#warning this does not work, style seems to be in a seperate array in a documentptr

    /***** go though the root features looking for styles *****/

    size_t nKmlFeatures = m_poKmlDSContainer->get_feature_array_size (  );
    size_t iKmlFeature;

    for ( iKmlFeature = 0; iKmlFeature < nKmlFeatures; iKmlFeature++ ) {
        FeaturePtr poKmlFeat =
            m_poKmlDSContainer->get_feature_array_at ( iKmlFeature );

            /***** is it a style *****/

        if ( poKmlFeat->IsA ( kmldom::Type_Style ) ) {

                /***** see if it has an id *****/

            if ( poKmlFeat->has_id (  ) ) {
                const std::string oKmlID = poKmlFeat->get_id (  );

                    /***** delete the style *****/

                m_poKmlDSContainer->DeleteFeatureById ( oKmlID );
            }
        }
    }

        /***** add the new style table to the container *****/

    styletable2kml ( poStyleTable, m_poKmlFactory, m_poKmlDSContainer );

    return;
}

/******************************************************************************
    method to write a style table to a kmz ds
******************************************************************************/

void OGRLIBKMLDataSource::SetStyleTable2Kmz (
    OGRStyleTable * poStyleTable )
{

    /***** replace the style document with a new one *****/

    m_poKmlStyleKml = m_poKmlFactory->CreateDocument (  );

    styletable2kml ( poStyleTable, m_poKmlFactory, m_poKmlStyleKml );

    return;
}

/******************************************************************************

******************************************************************************/

void OGRLIBKMLDataSource::SetStyleTableDirectly (
    OGRStyleTable * poStyleTable )
{

    if ( m_poStyleTable )
        delete m_poStyleTable;

    m_poStyleTable = poStyleTable;

    /***** a kml datasource? *****/

    if ( IsKml (  ) )
        SetStyleTable2Kml ( m_poStyleTable );

    else if ( IsKmz (  ) )
        SetStyleTable2Kmz ( m_poStyleTable );






    return;
}

/******************************************************************************

******************************************************************************/

void OGRLIBKMLDataSource::SetStyleTable (
    OGRStyleTable * poStyleTable )
{

    if ( poStyleTable )
        SetStyleTableDirectly ( poStyleTable->Clone (  ) );
    else
        SetStyleTableDirectly ( NULL );
    return;
}


/******************************************************************************
 TestCapability()
******************************************************************************/

int OGRLIBKMLDataSource::TestCapability (
    const char *pszCap )
{

    if ( EQUAL ( pszCap, ODsCCreateLayer ) )
        return bUpdate;
    else if ( EQUAL ( pszCap, ODsCDeleteLayer ) )
        return bUpdate;
    else
        return FALSE;

}
