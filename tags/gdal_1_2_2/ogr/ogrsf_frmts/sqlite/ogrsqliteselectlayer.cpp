/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Implements OGRSQLiteSelectLayer class, layer access to the results
 *           of a SELECT statement executed via ExecuteSQL().
 * Author:   Frank Warmerdam, warmerdam@pobox.com
 *
 ******************************************************************************
 * Copyright (c) 2004, Frank Warmerdam
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
 * Revision 1.3  2004/08/20 21:43:12  warmerda
 * avoid doing alot of work in GetExtent() if we have no geometry
 *
 * Revision 1.2  2004/07/11 19:23:51  warmerda
 * read implementation working well
 *
 * Revision 1.1  2004/07/09 06:25:05  warmerda
 * New
 */

#include "cpl_conv.h"
#include "ogr_sqlite.h"

CPL_CVSID("$Id$");
/************************************************************************/
/*                        OGRSQLiteSelectLayer()                        */
/************************************************************************/

OGRSQLiteSelectLayer::OGRSQLiteSelectLayer( OGRSQLiteDataSource *poDSIn,
                                            sqlite3_stmt *hStmtIn )

{
    poDS = poDSIn;

    iNextShapeId = 0;
    nSRSId = -1;
    poFeatureDefn = NULL;

    hStmt = hStmtIn;

    BuildFeatureDefn( "SELECT", hStmt );
    
    // Reset so the next _step() will get the first record.
    sqlite3_reset( hStmt );
}

/************************************************************************/
/*                       ~OGRSQLiteSelectLayer()                        */
/************************************************************************/

OGRSQLiteSelectLayer::~OGRSQLiteSelectLayer()

{
    sqlite3_finalize( hStmt );
    hStmt = NULL;
}

/************************************************************************/
/*                          SetSpatialFilter()                          */
/************************************************************************/

void OGRSQLiteSelectLayer::SetSpatialFilter( OGRGeometry * poGeomIn )

{
    if( poFilterGeom != NULL )
    {
        delete poFilterGeom;
        poFilterGeom = NULL;
    }

    if( poGeomIn != NULL )
        poFilterGeom = poGeomIn->clone();

    ResetReading();
}

/************************************************************************/
/*                           ClearStatement()                           */
/*                                                                      */
/*      Called when GetNextRawFeature() runs out of rows.               */
/************************************************************************/

void OGRSQLiteSelectLayer::ClearStatement()

{
}

/************************************************************************/
/*                            ResetReading()                            */
/************************************************************************/

void OGRSQLiteSelectLayer::ResetReading()

{
    sqlite3_reset( hStmt );
    OGRSQLiteLayer::ResetReading();
}

/************************************************************************/
/*                             GetFeature()                             */
/************************************************************************/

OGRFeature *OGRSQLiteSelectLayer::GetFeature( long nFeatureId )

{
    return OGRSQLiteLayer::GetFeature( nFeatureId );
}

/************************************************************************/
/*                         SetAttributeFilter()                         */
/************************************************************************/

OGRErr OGRSQLiteSelectLayer::SetAttributeFilter( const char *pszQuery )

{
    CPLError( CE_Failure, CPLE_AppDefined, 
              "SetAttributeFilter() not supported on ExecuteSQL() results." );

    return OGRERR_UNSUPPORTED_OPERATION;
}

/************************************************************************/
/*                           TestCapability()                           */
/************************************************************************/

int OGRSQLiteSelectLayer::TestCapability( const char * pszCap )

{
    return OGRSQLiteLayer::TestCapability( pszCap );
}

/************************************************************************/
/*                          GetFeatureCount()                           */
/*                                                                      */
/*      If a spatial filter is in effect, we turn control over to       */
/*      the generic counter.  Otherwise we return the total count.      */
/*      Eventually we should consider implementing a more efficient     */
/*      way of counting features matching a spatial query.              */
/************************************************************************/

int OGRSQLiteSelectLayer::GetFeatureCount( int bForce )

{
    return OGRSQLiteLayer::GetFeatureCount( bForce );
}
