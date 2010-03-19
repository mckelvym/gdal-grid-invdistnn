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

#include  <ogrsf_frmts.h>
#include <ogr_feature.h>

#include <kml/dom.h>
#include <iostream>

using kmldom::KmlFactory;;
using kmldom::PlacemarkPtr;
using kmldom::ExtendedDataPtr;
using kmldom::SchemaPtr;
using kmldom::SchemaDataPtr;
using kmldom::TimeSpanPtr;
using kmldom::SimpleDataPtr;
using kmldom::TimeStampPtr;
using kmldom::SimpleFieldPtr;


#include "ogrlibkmlfield.h"

void KML_time (
    char *buf,
    int bufsize,
    int *piYear,
    int *piMonth,
    int *piDay,
    int *piHour,
    int *piMin,
    int *piSec,
    int *pitz );

/******************************************************************************
 function to output ogr fields in kml

 args:
        poOgrFeat       pointer to the feature the field is in
        poKmlFactory    pointer to the libkml dom factory
        poKmlPlacemark  pointer to the placemark to add to
 
 returns:
        nothing

 env vars:
  LIBKML_TIMESTAMP_FIELD         default: OFTDate or OFTDateTime named timestamp
  LIBKML_TIMESPAN_BEGIN_FIELD    default: OFTDate or OFTDateTime named begin
  LIBKML_TIMESPAN_END_FIELD      default: OFTDate or OFTDateTime named end
  LIBKML_DESCRIPTION_FIELD       default: none
  LIBKML_NAME_FIELD              default: OFTString field named name


******************************************************************************/



void field2kml (
    OGRFeature * poOgrFeat,
    KmlFactory * poKmlFactory,
    PlacemarkPtr poKmlPlacemark )
{
    int i;

    ExtendedDataPtr poKmlExtendedData = poKmlFactory->CreateExtendedData (  );
    SchemaDataPtr poKmlSchemaData = poKmlFactory->CreateSchemaData (  );
    TimeSpanPtr poKmlTimeSpan = NULL;

    int nFields = poOgrFeat->GetFieldCount (  );

    for ( i = 0; i < nFields; i++ ) {

      /***** if the field isn't set just bail now *****/

        if ( !poOgrFeat->IsFieldSet ( i ) )
            continue;

        OGRFieldDefn *poOgrFieldDef = poOgrFeat->GetFieldDefnRef ( i );
        OGRFieldType type = poOgrFieldDef->GetType (  );
        const char *name = poOgrFieldDef->GetNameRef (  );

        const char *namefield =
            CPLGetConfigOption ( "LIBKML_NAME_FIELD", "Name" );
        const char *descfield =
            CPLGetConfigOption ( "LIBKML_DESCRIPTION_FIELD", "Description" );
        const char *tsfield =
            CPLGetConfigOption ( "LIBKML_TIMESTAMP_FIELD", "timestamp" );
        const char *beginfield =
            CPLGetConfigOption ( "LIBKML_BEGIN_FIELD", "begin" );
        const char *endfield =
            CPLGetConfigOption ( "LIBKML_END_FIELD", "end" );
        SimpleDataPtr poKmlSimpleData = NULL;

        switch ( type ) {

        case OFTString:        //     String of ASCII chars
            {

              /***** name *****/

                if ( !strcasecmp ( name, namefield ) ) {
                    poKmlPlacemark->set_name ( poOgrFeat->
                                               GetFieldAsString ( i ) );
                    continue;
                }

              /***** description *****/

                else if ( !strcasecmp ( name, descfield ) ) {
                    poKmlPlacemark->set_description ( poOgrFeat->
                                                      GetFieldAsString ( i ) );
                    continue;
                }

        /***** other *****/

                poKmlSimpleData = poKmlFactory->CreateSimpleData (  );
                poKmlSimpleData->set_name ( name );
                poKmlSimpleData->set_text ( poOgrFeat->
                                            GetFieldAsString ( i ) );

                break;
            }
#warning we need some kind of look ahead to get a time field if its a date field


        case OFTDate:          //   Date
        case OFTDateTime:      //  Date and Time
            {
              /***** timestamp *****/

                if ( !strcasecmp ( name, tsfield ) ) {

                    int year,
                        month,
                        day,
                        hour,
                        min,
                        sec,
                        tz;

                    poOgrFeat->GetFieldAsDateTime ( i,
                                                    &year, &month, &day,
                                                    &hour, &min, &sec, &tz );


                    char timebuf[30] = { };

                    if ( type == OFTDate )
                        KML_time ( timebuf, sizeof ( timebuf ), &year, &month,
                                   &day, &hour, &min, &sec, &tz );
                    else
                        KML_time ( timebuf, sizeof ( timebuf ), &year, &month,
                                   &day, &hour, &min, &sec, &tz );

                    TimeStampPtr poKmlTimeStamp =
                        poKmlFactory->CreateTimeStamp (  );
                    poKmlTimeStamp->set_when ( timebuf );
                    poKmlPlacemark->set_timeprimitive ( poKmlTimeStamp );

                    continue;
                }

              /***** begin *****/

                if ( !strcasecmp ( name, beginfield ) ) {

                    int year,
                        month,
                        day,
                        hour,
                        min,
                        sec,
                        tz;

                    poOgrFeat->GetFieldAsDateTime ( i,
                                                    &year, &month, &day,
                                                    &hour, &min, &sec, &tz );


                    char timebuf[30] = { };

                    poKmlFactory->CreateTimeSpan (  );

                    if ( type == OFTDate )
                        KML_time ( timebuf, sizeof ( timebuf ), &year, &month,
                                   &day, &hour, &min, &sec, &tz );
                    else
                        KML_time ( timebuf, sizeof ( timebuf ), &year, &month,
                                   &day, &hour, &min, &sec, &tz );

                    if ( !poKmlTimeSpan )
                        poKmlTimeSpan = poKmlFactory->CreateTimeSpan (  );
                    poKmlTimeSpan->set_begin ( timebuf );
                    if ( poKmlTimeSpan->has_end (  ) )
                        poKmlPlacemark->set_timeprimitive ( poKmlTimeSpan );

                    continue;

                }

              /***** end *****/

                else if ( !strcasecmp ( name, endfield ) ) {

                    int year,
                        month,
                        day,
                        hour,
                        min,
                        sec,
                        tz;

                    poOgrFeat->GetFieldAsDateTime ( i,
                                                    &year, &month, &day,
                                                    &hour, &min, &sec, &tz );


                    char timebuf[30] = { };

                    if ( type == OFTDate )
                        KML_time ( timebuf, sizeof ( timebuf ), &year, &month,
                                   &day, &hour, &min, &sec, &tz );
                    else
                        KML_time ( timebuf, sizeof ( timebuf ), &year, &month,
                                   &day, &hour, &min, &sec, &tz );


                    if ( !poKmlTimeSpan )
                        poKmlTimeSpan = poKmlFactory->CreateTimeSpan (  );
                    poKmlTimeSpan->set_end ( timebuf );
                    if ( poKmlTimeSpan->has_begin (  ) )
                        poKmlPlacemark->set_timeprimitive ( poKmlTimeSpan );

                    continue;
                }

              /***** other *****/

                poKmlSimpleData = poKmlFactory->CreateSimpleData (  );
                poKmlSimpleData->set_name ( name );
                poKmlSimpleData->set_text ( poOgrFeat->
                                            GetFieldAsString ( i ) );

                break;
            }

        case OFTTime:          //   Time
        case OFTStringList:    //     Array of strings
        case OFTBinary:        //     Raw Binary data

        /***** other *****/

            poKmlSimpleData = poKmlFactory->CreateSimpleData (  );
            poKmlSimpleData->set_name ( name );
            poKmlSimpleData->set_text ( poOgrFeat->GetFieldAsString ( i ) );

            break;
        case OFTInteger:       //    Simple 32bit integer

        /***** other *****/

            poKmlSimpleData = poKmlFactory->CreateSimpleData (  );
            poKmlSimpleData->set_name ( name );
            poKmlSimpleData->set_text ( poOgrFeat->GetFieldAsString ( i ) );

            break;
        case OFTIntegerList:   //    List of 32bit integers
        case OFTReal:          //   Double Precision floating point

        /***** other *****/

            poKmlSimpleData = poKmlFactory->CreateSimpleData (  );
            poKmlSimpleData->set_name ( name );
            poKmlSimpleData->set_text ( poOgrFeat->GetFieldAsString ( i ) );

            break;
        case OFTRealList:      //   List of doubles

        case OFTWideStringList:    //     deprecated
        default:

        /***** other *****/

            poKmlSimpleData = poKmlFactory->CreateSimpleData (  );
            poKmlSimpleData->set_name ( name );
            poKmlSimpleData->set_text ( poOgrFeat->GetFieldAsString ( i ) );

            break;
        }
        poKmlSchemaData->add_simpledata ( poKmlSimpleData );
    }
    poKmlExtendedData->add_schemadata ( poKmlSchemaData );
    poKmlPlacemark->set_extendeddata ( poKmlExtendedData );

    return;
}

/******************************************************************************
 function to read kml into ogr fields
******************************************************************************/

void kml2field (
    OGRFeature * poOgrFeat,
    PlacemarkPtr poKmlPlacemark )
{

    const char *namefield =
        CPLGetConfigOption ( "LIBKML_NAME_FIELD", "Name" );
    const char *descfield =
        CPLGetConfigOption ( "LIBKML_DESCRIPTION_FIELD", "Description" );
    
    /***** read the name tag *****/

    if (poKmlPlacemark->has_name()) {
        const std::string oKmlName = poKmlPlacemark->get_name();
        int iField = poOgrFeat->GetFieldIndex ( namefield );
        
        if (iField > -1)
            poOgrFeat->SetField ( iField, oKmlName.c_str (  ));
    }

    /***** read the desc tag *****/

    if (poKmlPlacemark->has_description()) {
        const std::string oKmlDesc = poKmlPlacemark->get_description();
        int iField = poOgrFeat->GetFieldIndex ( descfield );
        
        if (iField > -1)
            poOgrFeat->SetField ( iField, oKmlDesc.c_str (  ));
    }

                                 
    ExtendedDataPtr poKmlExtendedData = NULL;

    if ( poKmlPlacemark->has_extendeddata (  ) ) {
        poKmlExtendedData = poKmlPlacemark->get_extendeddata (  );

        /***** loop over the schemadata_arrays *****/
#warning can ogr even do this?
        size_t nSchemaData = poKmlExtendedData->get_schemadata_array_size (  );

        size_t iSchemaData;

        for ( iSchemaData = 0; iSchemaData < nSchemaData; iSchemaData++ ) {
            SchemaDataPtr poKmlSchemaData =
                poKmlExtendedData->get_schemadata_array_at ( iSchemaData );

            /***** loop over the simpledata array *****/

            size_t nSimpleData =
                poKmlSchemaData->get_simpledata_array_size (  );

            size_t iSimpleData;

            for ( iSimpleData = 0; iSimpleData < nSimpleData; iSimpleData++ ) {
                SimpleDataPtr poKmlSimpleData =
                    poKmlSchemaData->get_simpledata_array_at ( iSimpleData );

                /***** find the field index *****/

                int iField = -1;

                if ( poKmlSimpleData->has_name (  ) ) {
                    const string oName = poKmlSimpleData->get_name (  );
                    const char *pszName = oName.c_str (  );

                    iField = poOgrFeat->GetFieldIndex ( pszName );
                }

                /***** if it has trxt set the field *****/

                if ( iField > -1 && poKmlSimpleData->has_text (  ) ) {
                    const string oText = poKmlSimpleData->get_text (  );

                    poOgrFeat->SetField ( iField, oText.c_str (  ) );
                }
            }
        }
    }

#warning we need to pull the date and time types in somehow
#warning we need to parse timestamp and timespan

}


/*****************************************************************************
 buf should be atleast 26 bytes
 proccessing of the args stops when a NULL is encountered
*****************************************************************************/

void KML_time (
    char *buf,
    int bufsize,
    int *piYear,
    int *piMonth,
    int *piDay,
    int *piHour,
    int *piMin,
    int *piSec,
    int *pitz )
{

    if ( piYear )
        bufsize -= snprintf ( buf, bufsize, "%i", *piYear );
    else
        goto end;

    if ( piMonth )
        bufsize -= snprintf ( buf, bufsize, "-%02i", *piMonth );
    else
        goto end;

    if ( piDay )
        bufsize -= snprintf ( buf, bufsize, "-%02i", *piDay );
    else
        goto end;

    if ( piHour )
        bufsize -= snprintf ( buf, bufsize, "T%02i", *piHour );
    else
        goto end;

    if ( piMin )
        bufsize -= snprintf ( buf, bufsize, ":%02i", *piMin );
    else
        goto end;

    if ( piSec )
        bufsize -= snprintf ( buf, bufsize, ":%02i", *piSec );
    else
        goto end;

    if ( pitz ) {

    /***** utc *****/

        if ( *pitz == 100 ) {
            bufsize -= snprintf ( buf, bufsize, "Z" );
        }

    /***** local *****/

        else if ( *pitz == 1 ) {
        }

    /***** tz specified *****/

        else {

            bufsize -= snprintf ( buf, bufsize, "%2i%2i",
                                  ( *pitz - 100 ) / 4,
                                  ( ( *pitz - 100 ) % 4 ) * 15 );
        }
    }

  end:

    return;
}

SchemaPtr LayerDefn2kml (
    OGRLayer * poOgrLayer,
    KmlFactory * poKmlFactory )
{

    SimpleFieldPtr poKmlSimpleField;

    SchemaPtr poKmlSchema = poKmlFactory->CreateSchema (  );

    OGRFeatureDefn *poOgrFeatureDef = poOgrLayer->GetLayerDefn (  );

    poKmlSchema->set_name ( poOgrFeatureDef->GetName (  ) );

    int i;

    for ( i = 0; i < poOgrFeatureDef->GetFieldCount (  ); i++ ) {
        OGRFieldDefn *poOgrFieldDef = poOgrFeatureDef->GetFieldDefn ( i );

        poKmlSimpleField = poKmlFactory->CreateSimpleField (  );

        poKmlSimpleField->set_name ( poOgrFieldDef->GetNameRef (  ) );

        switch ( poOgrFieldDef->GetType (  ) ) {

        case OFTInteger:
        case OFTIntegerList:
            poKmlSimpleField->set_type ( "int" );
            break;
        case OFTReal:
        case OFTRealList:
            poKmlSimpleField->set_type ( "float" );
            break;
        case OFTBinary:
            poKmlSimpleField->set_type ( "bool" );
            break;
        case OFTString:
        case OFTStringList:
            poKmlSimpleField->set_type ( "string" );
            break;
            //TODO: KML doesn't handle these data types yet...
        case OFTDate:
        case OFTTime:
        case OFTDateTime:
        default:
            poKmlSimpleField->set_type ( "string" );
            break;
        }
        poKmlSchema->add_simplefield ( poKmlSimpleField );

    }

    return poKmlSchema;
}
