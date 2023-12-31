/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Private definitions for OGR/ODBC driver.
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2003, Frank Warmerdam <warmerdam@pobox.com>
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
 * Revision 1.5  2005/02/22 12:53:56  fwarmerdam
 * use OGRLayer base spatial filter support
 *
 * Revision 1.4  2004/01/05 22:38:17  warmerda
 * stripped out some junk
 *
 * Revision 1.3  2004/01/05 22:23:40  warmerda
 * added ExecuteSQL implementation via OGRODBCSelectLayer
 *
 * Revision 1.2  2003/10/29 17:47:38  warmerda
 * Added FIDcolumn (based on primary key) support
 *
 * Revision 1.1  2003/09/25 17:08:37  warmerda
 * New
 *
 */

#ifndef _OGR_ODBC_H_INCLUDED
#define _OGR_ODBC_H_INLLUDED

#include "ogrsf_frmts.h"
#include "cpl_odbc.h"
#include "cpl_error.h"

/************************************************************************/
/*                            OGRODBCLayer                                */
/************************************************************************/

class OGRODBCDataSource;
    
class OGRODBCLayer : public OGRLayer
{
  protected:
    OGRFeatureDefn     *poFeatureDefn;

    CPLODBCStatement   *poStmt;

    // Layer spatial reference system, and srid.
    OGRSpatialReference *poSRS;
    int                 nSRSId;

    int                 iNextShapeId;

    OGRODBCDataSource    *poDS;

    char                *pszGeomColumn;
    char                *pszFIDColumn;

    int                *panFieldOrdinals;

    CPLErr              BuildFeatureDefn( const char *pszLayerName,
                                          CPLODBCStatement *poStmt );

    virtual CPLODBCStatement *  GetStatement() { return poStmt; }

  public:
                        OGRODBCLayer();
    virtual             ~OGRODBCLayer();

    virtual void        ResetReading();
    virtual OGRFeature *GetNextRawFeature();
    virtual OGRFeature *GetNextFeature();

    virtual OGRFeature *GetFeature( long nFeatureId );
    
    OGRFeatureDefn *    GetLayerDefn() { return poFeatureDefn; }

    virtual OGRSpatialReference *GetSpatialRef();

    virtual int         TestCapability( const char * );
};

/************************************************************************/
/*                           OGRODBCTableLayer                            */
/************************************************************************/

class OGRODBCTableLayer : public OGRODBCLayer
{
    int                 bUpdateAccess;

    char                *pszQuery;

    void		ClearStatement();
    OGRErr              ResetStatement();

    virtual CPLODBCStatement *  GetStatement();

  public:
                        OGRODBCTableLayer( OGRODBCDataSource * );
                        ~OGRODBCTableLayer();

    CPLErr              Initialize( const char *pszTableName, 
                                    const char *pszGeomCol );

    virtual void        ResetReading();
    virtual int         GetFeatureCount( int );

    virtual OGRErr      SetAttributeFilter( const char * );
#ifdef notdef
    virtual OGRErr      SetFeature( OGRFeature *poFeature );
    virtual OGRErr      CreateFeature( OGRFeature *poFeature );
    
    virtual OGRErr      CreateField( OGRFieldDefn *poField,
                                     int bApproxOK = TRUE );
#endif    
    virtual OGRFeature *GetFeature( long nFeatureId );
    
    virtual OGRSpatialReference *GetSpatialRef();

    virtual int         TestCapability( const char * );

#ifdef notdef
    // follow methods are not base class overrides
    void                SetLaunderFlag( int bFlag ) 
                                { bLaunderColumnNames = bFlag; }
    void                SetPrecisionFlag( int bFlag ) 
                                { bPreservePrecision = bFlag; }
#endif
};

/************************************************************************/
/*                          OGRODBCSelectLayer                          */
/************************************************************************/

class OGRODBCSelectLayer : public OGRODBCLayer
{
    char                *pszBaseStatement;

    void		ClearStatement();
    OGRErr              ResetStatement();

    virtual CPLODBCStatement *  GetStatement();

  public:
                        OGRODBCSelectLayer( OGRODBCDataSource *, 
                                           CPLODBCStatement * );
                        ~OGRODBCSelectLayer();

    virtual void        ResetReading();
    virtual int         GetFeatureCount( int );

    virtual OGRErr      SetAttributeFilter( const char * );
    virtual OGRFeature *GetFeature( long nFeatureId );
    
    virtual OGRErr      GetExtent(OGREnvelope *psExtent, int bForce = TRUE);

    virtual int         TestCapability( const char * );
};

/************************************************************************/
/*                           OGRODBCDataSource                            */
/************************************************************************/

class OGRODBCDataSource : public OGRDataSource
{
    OGRODBCLayer        **papoLayers;
    int                 nLayers;
    
    char               *pszName;

    int                 bDSUpdate;
    CPLODBCSession      oSession;

    // We maintain a list of known SRID to reduce the number of trips to
    // the database to get SRSes. 
    int                 nKnownSRID;
    int                *panSRID;
    OGRSpatialReference **papoSRS;
    
  public:
                        OGRODBCDataSource();
                        ~OGRODBCDataSource();

    int                 Open( const char *, int bUpdate, int bTestOpen );
    int                 OpenTable( const char *pszTableName, 
                                   const char *pszGeomCol,
                                   int bUpdate );

    const char          *GetName() { return pszName; }
    int                 GetLayerCount() { return nLayers; }
    OGRLayer            *GetLayer( int );

    int                 TestCapability( const char * );

    virtual OGRLayer *  ExecuteSQL( const char *pszSQLCommand,
                                    OGRGeometry *poSpatialFilter,
                                    const char *pszDialect );
    virtual void        ReleaseResultSet( OGRLayer * poLayer );

    // Internal use
    CPLODBCSession     *GetSession() { return &oSession; }

};

/************************************************************************/
/*                             OGRODBCDriver                            */
/************************************************************************/

class OGRODBCDriver : public OGRSFDriver
{
  public:
                ~OGRODBCDriver();
                
    const char *GetName();
    OGRDataSource *Open( const char *, int );

    virtual OGRDataSource *CreateDataSource( const char *pszName,
                                             char ** = NULL );
    
    int                 TestCapability( const char * );
};


#endif /* ndef _OGR_ODBC_H_INCLUDED */


