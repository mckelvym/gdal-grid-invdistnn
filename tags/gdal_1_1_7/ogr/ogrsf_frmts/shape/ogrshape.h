/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Private definitions within the Shapefile driver to implement
 *           integration with OGR.
 * Author:   Frank Warmerdam, warmerda@home.com
 *
 ******************************************************************************
 * Copyright (c) 1999,  Les Technologies SoftMap Inc.
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
 * Revision 1.9  2002/03/27 21:04:38  warmerda
 * Added support for reading, and creating lone .dbf files for wkbNone geometry
 * layers.  Added support for creating a single .shp file instead of a directory
 * if a path ending in .shp is passed to the data source create method.
 *
 * Revision 1.8  2001/09/04 15:35:14  warmerda
 * add support for deferring geometry type selection till first feature
 *
 * Revision 1.7  2001/03/16 22:16:10  warmerda
 * added support for ESRI .prj files
 *
 * Revision 1.6  2001/03/15 04:21:50  danmo
 * Added GetExtent()
 *
 * Revision 1.5  1999/11/04 21:17:25  warmerda
 * support layer/ds creation, one ds is now many shapefiles
 *
 * Revision 1.4  1999/07/27 00:52:17  warmerda
 * added random access, write and capability methods
 *
 * Revision 1.3  1999/07/26 13:59:25  warmerda
 * added feature writing api
 *
 * Revision 1.2  1999/07/08 20:05:45  warmerda
 * added GetFeatureCount()
 *
 * Revision 1.1  1999/07/05 18:58:07  warmerda
 * New
 *
 */

#ifndef _OGRSHAPE_H_INCLUDED
#define _OGRSHAPE_H_INLLUDED

#include "shapefil.h"
#include "ogrsf_frmts.h"

/* ==================================================================== */
/*      Functions from Shape2ogr.cpp.                                   */
/* ==================================================================== */
OGRFeature *SHPReadOGRFeature( SHPHandle hSHP, DBFHandle hDBF,
                               OGRFeatureDefn * poDefn, int iShape );
OGRGeometry *SHPReadOGRObject( SHPHandle hSHP, int iShape );
OGRFeatureDefn *SHPReadOGRFeatureDefn( const char * pszName,
                                       SHPHandle hSHP, DBFHandle hDBF );
OGRErr SHPWriteOGRFeature( SHPHandle hSHP, DBFHandle hDBF,
                           OGRFeatureDefn *poFeatureDefn,
                           OGRFeature *poFeature );

/************************************************************************/
/*                            OGRShapeLayer                             */
/************************************************************************/

class OGRShapeLayer : public OGRLayer
{
    OGRSpatialReference *poSRS;
    OGRFeatureDefn     *poFeatureDefn;
    OGRGeometry		*poFilterGeom;
    int			iNextShapeId;
    int			nTotalShapeCount;

    SHPHandle		hSHP;
    DBFHandle		hDBF;

    int			bUpdateAccess;

    OGRwkbGeometryType  eRequestedGeomType;
    int			ResetGeomType( int nNewType );

  public:
    			OGRShapeLayer( const char * pszName,
                                       SHPHandle hSHP, DBFHandle hDBF,
                                       OGRSpatialReference *poSRS,
                                       int bUpdate, 
                                       OGRwkbGeometryType eReqType=wkbUnknown);
    			~OGRShapeLayer();

    OGRGeometry *	GetSpatialFilter() { return poFilterGeom; }
    void		SetSpatialFilter( OGRGeometry * );

    void		ResetReading();
    OGRFeature *	GetNextFeature();

    OGRFeature         *GetFeature( long nFeatureId );
    OGRErr              SetFeature( OGRFeature *poFeature );
    OGRErr              CreateFeature( OGRFeature *poFeature );
    
    OGRFeatureDefn *	GetLayerDefn() { return poFeatureDefn; }

    int                 GetFeatureCount( int );
    OGRErr              GetExtent(OGREnvelope *psExtent, int bForce);

    virtual OGRErr      CreateField( OGRFieldDefn *poField,
                                     int bApproxOK = TRUE );

    virtual OGRSpatialReference *GetSpatialRef();
    
    int                 TestCapability( const char * );
};

/************************************************************************/
/*                          OGRShapeDataSource                          */
/************************************************************************/

class OGRShapeDataSource : public OGRDataSource
{
    OGRShapeLayer     **papoLayers;
    int			nLayers;
    
    char		*pszName;

    int			bDSUpdate;

    int                 bSingleNewFile;
    
  public:
    			OGRShapeDataSource();
    			~OGRShapeDataSource();

    int			Open( const char *, int bUpdate, int bTestOpen,
                              int bSingleNewFile = FALSE );
    int                 OpenFile( const char *, int bUpdate, int bTestOpen );

    const char	        *GetName() { return pszName; }
    int			GetLayerCount() { return nLayers; }
    OGRLayer		*GetLayer( int );

    virtual OGRLayer    *CreateLayer( const char *, 
                                      OGRSpatialReference * = NULL,
                                      OGRwkbGeometryType = wkbUnknown,
                                      char ** = NULL );

    int                 TestCapability( const char * );
};

/************************************************************************/
/*                            OGRShapeDriver                            */
/************************************************************************/

class OGRShapeDriver : public OGRSFDriver
{
  public:
    		~OGRShapeDriver();
                
    const char *GetName();
    OGRDataSource *Open( const char *, int );

    virtual OGRDataSource *CreateDataSource( const char *pszName,
                                             char ** = NULL );
    
    int                 TestCapability( const char * );
};


#endif /* ndef _OGRSHAPE_H_INCLUDED */
