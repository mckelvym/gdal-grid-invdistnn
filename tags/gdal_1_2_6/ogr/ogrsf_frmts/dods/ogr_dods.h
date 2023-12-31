/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Private definitions for OGR/DODS driver.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2004, Frank Warmerdam <warmerdam@pobox.com>
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
 * Revision 1.9  2005/02/22 12:57:39  fwarmerdam
 * use OGRLayer base spatial filter support
 *
 * Revision 1.8  2004/03/17 18:15:57  warmerda
 * added floating point test prototypes
 *
 * Revision 1.7  2004/03/12 22:13:07  warmerda
 * major upgrade with normalized sequen nested sequence support
 *
 * Revision 1.6  2004/02/19 13:58:26  warmerda
 * complete extra_containers support for grids
 *
 * Revision 1.5  2004/02/17 18:47:05  warmerda
 * added geometry support
 *
 * Revision 1.4  2004/02/17 05:46:43  warmerda
 * Added grid/array support
 *
 * Revision 1.3  2004/01/29 21:01:03  warmerda
 * added sequences within sequences support
 *
 * Revision 1.2  2004/01/22 21:15:36  warmerda
 * parse url into components
 *
 * Revision 1.1  2004/01/21 20:08:29  warmerda
 * New
 *
 */

#ifndef _OGR_DODS_H_INCLUDED
#define _OGR_DODS_H_INLLUDED

#include "ogrsf_frmts.h"
#include "cpl_error.h"
#include "cpl_conv.h"

// Lots of DODS related definitions

#include <string>
#include <sstream>
#include <algorithm>
#include <exception>

// #define DODS_DEBUG 1
#include <debug.h>

#include <BaseType.h>		// DODS
#include <Byte.h>
#include <Int16.h>
#include <UInt16.h>
#include <Int32.h>
#include <UInt32.h>
#include <Float32.h>
#include <Float64.h>
#include <Str.h>
#include <Url.h>
#include <Array.h>
#include <Structure.h>
#include <Sequence.h>
#include <Grid.h>

#include <AISConnect.h>		
#include <DDS.h>
#include <DAS.h>
#include <Error.h>
#include <escaping.h>

/************************************************************************/
/*                           OGRDODSFieldDefn                           */
/************************************************************************/
class OGRDODSFieldDefn {
public:
    OGRDODSFieldDefn();
    ~OGRDODSFieldDefn();
    
    int Initialize( AttrTable *, 
                    BaseType *poTarget = NULL, BaseType *poSuperSeq = NULL );
    int Initialize( const char *, const char * = "das",
                    BaseType *poTarget = NULL, BaseType *poSuperSeq = NULL );

    int  bValid;
    char *pszFieldName;
    char *pszFieldScope;
    int  iFieldIndex;
    char *pszFieldValue;
    char *pszPathToSequence;

    int  bRelativeToSuperSequence;
    int  bRelativeToSequence;
};

/************************************************************************/
/*                             OGRDODSLayer                             */
/************************************************************************/

class OGRDODSDataSource;
    
class OGRDODSLayer : public OGRLayer
{
  protected:
    OGRFeatureDefn     *poFeatureDefn;

    OGRSpatialReference *poSRS;

    int                 iNextShapeId;

    OGRDODSDataSource  *poDS;

    char               *pszQuery;

    char               *pszFIDColumn;

    char               *pszTarget;

    OGRDODSFieldDefn  **papoFields;

    virtual int         ProvideDataDDS();
    int                 bDataLoaded;

    AISConnect         *poConnection;
    DataDDS             oDataDDS;

    BaseType           *poTargetVar;
    
    AttrTable          *poOGRLayerInfo;

    int                 bKnowExtent;
    OGREnvelope         sExtent;

  public:
                        OGRDODSLayer( OGRDODSDataSource *poDS, 
                                      const char *pszTarget,
                                      AttrTable *poAttrInfo );
    virtual             ~OGRDODSLayer();

    virtual void        ResetReading();

    virtual OGRFeature *GetNextFeature();

    OGRFeatureDefn *    GetLayerDefn() { return poFeatureDefn; }

    virtual OGRSpatialReference *GetSpatialRef();

    virtual int         TestCapability( const char * );

    virtual OGRErr      GetExtent(OGREnvelope *psExtent, int bForce = TRUE);
};

/************************************************************************/
/*                         OGRDODSSequenceLayer                         */
/************************************************************************/

class OGRDODSSequenceLayer : public OGRDODSLayer
{
private:
    OGRDODSFieldDefn    oXField;
    OGRDODSFieldDefn    oYField;
    OGRDODSFieldDefn    oZField;

    char               *pszSubSeqPath;

    Sequence           *poSuperSeq;

    int                 iLastSuperSeq;

    int                 nRecordCount; /* -1 if not yet known */
    int                 nSuperSeqCount; 
    int                *panSubSeqSize;

    double              GetFieldValueAsDouble( OGRDODSFieldDefn *, int );
    BaseType           *GetFieldValue( OGRDODSFieldDefn *, int,
                                       Sequence * );

    double              BaseTypeToDouble( BaseType * );
    
    int                 BuildFields( BaseType *, const char *, 
                                     const char * );

    Sequence           *FindSuperSequence( BaseType * );

protected:
    virtual int         ProvideDataDDS();

public:
                        OGRDODSSequenceLayer( OGRDODSDataSource *poDS, 
                                              const char *pszTarget,
                                              AttrTable *poAttrInfo );
    virtual             ~OGRDODSSequenceLayer();

    virtual OGRFeature *GetFeature( long nFeatureId );
    
    virtual int         GetFeatureCount( int );
};

/************************************************************************/
/*                           OGRDODSGridLayer                           */
/************************************************************************/

class OGRDODSDim
{
public:
    OGRDODSDim() { 
        pszDimName = NULL;
        nDimStart = 0;
        nDimEnd = 0;
        nDimStride = 0;
        nDimEntries = 0;
        poMap = NULL;
        pRawData = NULL;
        iLastValue = 0;
    }
    ~OGRDODSDim() {
        CPLFree( pszDimName );
        CPLFree( pRawData );
    }

    char *pszDimName;
    int  nDimStart;
    int  nDimEnd;
    int  nDimStride;
    int  nDimEntries;
    Array *poMap;
    void *pRawData;
    int  iLastValue;
};

class OGRDODSArrayRef
{
public:
    OGRDODSArrayRef() { 
        pszName = NULL;
        iFieldIndex = -1;
        poArray = NULL;
        pRawData = NULL;
            }
    ~OGRDODSArrayRef() {
        CPLFree( pszName );
        CPLFree( pRawData );
    }

    char *pszName;
    int   iFieldIndex;
    Array *poArray;
    void  *pRawData;
};

class OGRDODSGridLayer : public OGRDODSLayer
{
    Grid               *poTargetGrid; // NULL if simple array used.
    Array              *poTargetArray;

    int                 nArrayRefCount;
    OGRDODSArrayRef    *paoArrayRefs;  // includes poTargetArray.

    OGRDODSFieldDefn    oXField;
    OGRDODSFieldDefn    oYField;
    OGRDODSFieldDefn    oZField;

    int                 nDimCount;
    OGRDODSDim         *paoDimensions;
    int                 nMaxRawIndex;

    void               *pRawData;

    int                 ArrayEntryToField( Array *poArray, void *pRawData, 
                                           int iArrayIndex,
                                           OGRFeature *poFeature, int iField);
								       
protected:
    virtual int         ProvideDataDDS();

public:
                        OGRDODSGridLayer( OGRDODSDataSource *poDS, 
                                         const char *pszTarget,
                                         AttrTable *poAttrInfo );
    virtual             ~OGRDODSGridLayer();

    virtual OGRFeature *GetFeature( long nFeatureId );
    
    virtual int         GetFeatureCount( int );

};

/************************************************************************/
/*                          OGRDODSDataSource                           */
/************************************************************************/

class OGRDODSDataSource : public OGRDataSource
{
    OGRDODSLayer        **papoLayers;
    int                 nLayers;
    
    char               *pszName;

    void                AddLayer( OGRDODSLayer * );

  public: // Just intended for read access by layer classes.
    AISConnect         *poConnection;

    DAS                 oDAS;
    DDS                 oDDS;

    string              oBaseURL;
    string              oProjection;
    string              oConstraints;

  public:

                        OGRDODSDataSource();
                        ~OGRDODSDataSource();

    int                 Open( const char * );

    const char          *GetName() { return pszName; }
    int                 GetLayerCount() { return nLayers; }
    OGRLayer            *GetLayer( int );

    int                 TestCapability( const char * );
};

/************************************************************************/
/*                             OGRDODSDriver                            */
/************************************************************************/

class OGRDODSDriver : public OGRSFDriver
{
  public:
                ~OGRDODSDriver();
    const char *GetName();
    OGRDataSource *Open( const char *, int );
    int                 TestCapability( const char * );
};

string OGRDODSGetVarPath( BaseType * );
int  OGRDODSGetVarIndex( Sequence *poParent, string oVarName );

int  OGRDODSIsFloatInvalid( const float * );
int  OGRDODSIsDoubleInvalid( const double * );

#endif /* ndef _OGR_DODS_H_INCLUDED */


