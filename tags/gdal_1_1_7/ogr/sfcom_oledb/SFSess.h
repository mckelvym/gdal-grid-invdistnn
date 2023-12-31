/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Declaration of the CSFSession
 * Author:   Ken Shih, kshih@home.com
 *
 ******************************************************************************
 * Copyright (c) 1999, Les Technologies SoftMap Inc.
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
 * Revision 1.19  2002/04/10 20:07:59  warmerda
 * Added ADSK_GEOM_EXTENT support
 *
 * Revision 1.18  2002/01/13 01:41:46  warmerda
 * add proper support for restrictions
 *
 * Revision 1.17  2001/11/09 20:48:19  warmerda
 * use SFGetLayerWKT() to cleanup WKT
 *
 * Revision 1.16  2001/11/09 19:07:33  warmerda
 * added debugging
 *
 * Revision 1.15  2001/10/22 21:29:24  warmerda
 * added some debugging statements
 *
 * Revision 1.14  2001/10/15 15:36:30  warmerda
 * don't default to EPSG authority
 *
 * Revision 1.13  2001/10/15 15:20:28  warmerda
 * allow nulling of SRS fields
 *
 * Revision 1.12  2001/05/28 19:41:58  warmerda
 * lots of changes
 *
 * Revision 1.11  2001/04/30 18:57:57  warmerda
 * Added debug statement and cpl_error.h
 *
 * Revision 1.10  1999/11/22 18:25:58  warmerda
 * spatial reference table generation is now working fairly well.  I
 * should still try and boil out duplicate rows.
 *
 * Revision 1.9  1999/11/22 17:15:12  warmerda
 * reformat
 *
 * Revision 1.8  1999/07/20 17:11:11  kshih
 * Use OGR code
 *
 * Revision 1.7  1999/06/21 20:52:38  warmerda
 * Added default lat/long SRS WKT value.
 *
 * Revision 1.6  1999/06/21 17:29:51  warmerda
 * Set the SRS id to 2 to match the value dummy value in the SRS table.
 *
 * Revision 1.5  1999/06/15 03:03:16  kshih
 * Provider Type schema Rowset
 *
 * Revision 1.4  1999/06/13 17:49:46  warmerda
 * Added copyright header, and some comments.
 *
 */

#ifndef __CSFSession_H_
#define __CSFSession_H_
#include "resource.h"       // main symbols
#include "SFRS.h"
#include "oledbgis.h"
#include "cpl_error.h"
#include "ICRRowsetImpl.h"
#include "CCRRowsetImpl.h"

class CSFSessionTRSchemaRowset;
class CSFSessionColSchemaRowset;
class CSFSessionPTSchemaRowset;
class CSFSessionSchemaOGISTables;
class CSFSessionSchemaOGISGeoColumns;
class CSFSessionSchemaSpatRef;


class ATL_NO_VTABLE CSFSessionSupportErrorInfoImpl : public ISupportErrorInfo
{
public:
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid)
	{
		if (IID_IOpenRowset == riid)
			return S_OK;

		return S_FALSE;
	}
};

/////////////////////////////////////////////////////////////////////////////
// CSFSession
class ATL_NO_VTABLE CSFSession : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IGetDataSourceImpl<CSFSession>,
	public IOpenRowsetImpl<CSFSession>,
	public ISessionPropertiesImpl<CSFSession>,
	public IObjectWithSiteSessionImpl<CSFSession>,
	public IDBSchemaRowsetImpl<CSFSession>,
	public IDBCreateCommandImpl<CSFSession, CSFCommand>,
	public CSFSessionSupportErrorInfoImpl
{
public:
	CSFSession()
	{
            CPLDebug( "OGR_OLEDB", "CSFSession() constructor" );
	}
	virtual ~CSFSession()
	{
            CPLDebug( "OGR_OLEDB", "~CSFSession()" );
	}
	HRESULT FinalConstruct()
	{
		return FInit();
	}
	STDMETHOD(OpenRowset)(IUnknown *pUnk, DBID *pTID, DBID *pInID, REFIID riid,
					   ULONG cSets, DBPROPSET rgSets[], IUnknown **ppRowset)
	{
		CSFRowset* pRowset;
		return CreateRowset(pUnk, pTID, pInID, riid, cSets, rgSets, ppRowset, pRowset);
	}
BEGIN_PROPSET_MAP(CSFSession)
	BEGIN_PROPERTY_SET(DBPROPSET_SESSION)
		PROPERTY_INFO_ENTRY(SESS_AUTOCOMMITISOLEVELS)
	END_PROPERTY_SET(DBPROPSET_SESSION)
	BEGIN_PROPERTY_SET(DBPROPSET_ROWSET)
		PROPERTY_INFO_ENTRY(CANHOLDROWS)
	END_PROPERTY_SET(DBPROPSET_ROWSET)
END_PROPSET_MAP()
BEGIN_COM_MAP(CSFSession)
	COM_INTERFACE_ENTRY(IGetDataSource)
	COM_INTERFACE_ENTRY(IOpenRowset)
	COM_INTERFACE_ENTRY(ISessionProperties)
	COM_INTERFACE_ENTRY(IObjectWithSite)
	COM_INTERFACE_ENTRY(IDBCreateCommand)
	COM_INTERFACE_ENTRY(IDBSchemaRowset)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
BEGIN_SCHEMA_MAP(CSFSession)
	SCHEMA_ENTRY(DBSCHEMA_TABLES, CSFSessionTRSchemaRowset)
	SCHEMA_ENTRY(DBSCHEMA_COLUMNS, CSFSessionColSchemaRowset)
	SCHEMA_ENTRY(DBSCHEMA_PROVIDER_TYPES, CSFSessionPTSchemaRowset)
	SCHEMA_ENTRY(DBSCHEMA_OGIS_FEATURE_TABLES, CSFSessionSchemaOGISTables)
	SCHEMA_ENTRY(DBSCHEMA_OGIS_GEOMETRY_COLUMNS,CSFSessionSchemaOGISGeoColumns)
	SCHEMA_ENTRY(DBSCHEMA_OGIS_SPATIAL_REF_SYSTEMS,CSFSessionSchemaSpatRef);
END_SCHEMA_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CSFSessionTRSchemaRowset
class CSFSessionTRSchemaRowset : 
	public CRowsetImpl< CSFSessionTRSchemaRowset, CTABLESRow, CSFSession>
{
  public:
    HRESULT Execute(LONG* pcRowsAffected,
                    ULONG cRestrictions,
                    const VARIANT*rgRestrictions)
    {
        USES_CONVERSION;
        
        CTABLESRow		trData;
        int				iLayer;
        IUnknown		*pIU;
        OGRDataSource	*poDS;
        OGRLayer		*pLayer;
        OGRFeatureDefn	*poDefn;
        const char      *pszTableRestriction = NULL;
        
        CPLDebug( "OGR_OLEDB",
                  "CSFSessionTRSchemaRowset::Execute()." );
                
        if( cRestrictions >= 3
            && rgRestrictions[2].vt == VT_BSTR )
        {
            pszTableRestriction = OLE2A(rgRestrictions[2].bstrVal);
            if( strlen(pszTableRestriction) == 0 )
                pszTableRestriction = NULL;
            else
                CPLDebug( "OGR_OLEDB", "TABLE_NAME restriction = %s",
                          pszTableRestriction );
        }

        QueryInterface(IID_IUnknown,(void **) &pIU);
        poDS = SFGetOGRDataSource(pIU);

        if (!poDS)
        {
            // Prep errors as well
            return S_FALSE;
        }
        
        for (iLayer = 0; iLayer < poDS->GetLayerCount(); iLayer++)
        {
            pLayer = poDS->GetLayer(iLayer);
            poDefn = pLayer->GetLayerDefn();

            if( pszTableRestriction != NULL
                && !EQUAL(pszTableRestriction,poDefn->GetName()) )
                continue;
            
            lstrcpyW(trData.m_szType, OLESTR("TABLE"));
            lstrcpyW(trData.m_szTable,A2OLE(poDefn->GetName()));
            if (!m_rgRowData.Add(trData))
                return E_OUTOFMEMORY;
        }
        
        *pcRowsAffected = poDS->GetLayerCount();
        
        return S_OK;
    }
};
 
	
/////////////////////////////////////////////////////////////////////////////
// CSFSessionColSchemaRowset
class CSFSessionColSchemaRowset : 
	public CRowsetImpl< CSFSessionColSchemaRowset, CCOLUMNSRow, CSFSession>
{
  public:
    HRESULT Execute(LONG* pcRowsAffected,
                    ULONG cRestrictions,
                    const VARIANT*rgRestrictions)
    {
        USES_CONVERSION;
        int				i;
        int				iLayer;
        IUnknown		*pIU;
        OGRDataSource	*poDS;
        OGRLayer		*pLayer;
        OGRFeatureDefn	*poDefn;
        OGRFieldDefn	*poField;
        const char      *pszTableRestriction = NULL;
        const char      *pszColumnRestriction = NULL;

        CPLDebug( "OGR_OLEDB",
                  "CSFSessionColSchemaRowset::Execute(%p).",
                  pcRowsAffected );

        if( cRestrictions >= 3
            && rgRestrictions[2].vt == VT_BSTR )
        {
            pszTableRestriction = OLE2A(rgRestrictions[2].bstrVal);
            if( strlen(pszTableRestriction) == 0 )
                pszTableRestriction = NULL;
            else
                CPLDebug( "OGR_OLEDB", "TABLE_NAME restriction = %s",
                          pszTableRestriction );
        }
        if( cRestrictions >= 4
            && rgRestrictions[3].vt == VT_BSTR )
        {
            pszColumnRestriction = OLE2A(rgRestrictions[3].bstrVal);
            if( strlen(pszColumnRestriction) == 0 )
                pszColumnRestriction = NULL;
            else
                CPLDebug( "OGR_OLEDB", "COLUMN_NAME restriction = %s",
                          pszColumnRestriction );
        }

        *pcRowsAffected = 0;
        
        QueryInterface(IID_IUnknown,(void **) &pIU);
        poDS = SFGetOGRDataSource(pIU);

        if (!poDS)
        {
            // Prep errors as well
            return S_FALSE;
        }

        for (iLayer = 0; iLayer < poDS->GetLayerCount(); iLayer++)
        {
            CCOLUMNSRow trData;
            
            pLayer = poDS->GetLayer(iLayer);
            poDefn = pLayer->GetLayerDefn();

            if( pszTableRestriction != NULL
                && !EQUAL(pszTableRestriction,poDefn->GetName()) )
                continue;
            
            LPOLESTR	pszLayerName = A2OLE(poDefn->GetName());

            memset( &trData, 0, sizeof(trData) );
            trData.m_nDataType = DBTYPE_I4;
            lstrcpyW(trData.m_szTableName,pszLayerName);
            lstrcpyW(trData.m_szColumnName,A2OLE("FID"));
            trData.m_ulOrdinalPosition = 1;
            
            if (!m_rgRowData.Add(trData))
                return E_OUTOFMEMORY;
            
            // Add all the regular feature attributes.
            for (i=0; i < poDefn->GetFieldCount(); i++)
            {
                poField = poDefn->GetFieldDefn(i);

                if( pszColumnRestriction != NULL
                    && !EQUAL(pszColumnRestriction,poField->GetNameRef()) )
                    continue;

                memset( &trData, 0, sizeof(trData) );
                switch(poField->GetType())
                {
                    case OFTInteger:
                        trData.m_nDataType = DBTYPE_I4;
                        break;
                    case OFTReal:
                        trData.m_nDataType = DBTYPE_R8;
                        trData.m_nNumericPrecision = poField->GetPrecision();

                        break;
                    case OFTString:
                        int nLength;
                        nLength = poField->GetWidth();
                        if (nLength == 0 || nLength > 4096)
                        {
                            nLength = 4096;
                        }
                        trData.m_nDataType = DBTYPE_STR;
                        trData.m_ulCharMaxLength = nLength;
                        trData.m_ulCharOctetLength = nLength;
                        break;
                    default:
                        return S_FALSE;
                }

                lstrcpyW(trData.m_szTableName,pszLayerName);
                lstrcpyW(trData.m_szColumnName,A2OLE(poField->GetNameRef()));
                trData.m_ulOrdinalPosition = i+2;
				
                if (!m_rgRowData.Add(trData))
                    return E_OUTOFMEMORY;
            }

            // Add the geometry column.
            memset( &trData, 0, sizeof(trData) );
            lstrcpyW(trData.m_szTableName,pszLayerName);
            lstrcpyW(trData.m_szColumnName,A2OLE("OGIS_GEOMETRY"));
            trData.m_ulOrdinalPosition = i+2;
            trData.m_nDataType = DBTYPE_IUNKNOWN;
            if (!m_rgRowData.Add(trData))
                return E_OUTOFMEMORY;

            *pcRowsAffected += poDefn->GetFieldCount() + 2;
        }

        return S_OK;
    }
};

/////////////////////////////////////////////////////////////////////////////
// CSFSessionPTSchemaRowset
class CSFSessionPTSchemaRowset : 
	public CRowsetImpl< CSFSessionPTSchemaRowset, CPROVIDER_TYPERow, CSFSession>
{
public:
	HRESULT Execute(LONG* pcRowsAffected, ULONG, const VARIANT*)
	{
		USES_CONVERSION;

		CPROVIDER_TYPERow trDataI,trDataR,trDataS,trDataBlob;
		
                CPLDebug( "OGR_OLEDB",
                          "CSFSessionPTSchemaRowset::Execute()." );
                
		lstrcpyW(trDataI.m_szName,A2OLE("Integer"));
		trDataI.m_nType  = DBTYPE_I4;
		m_rgRowData.Add(trDataI);

		lstrcpyW(trDataR.m_szName,A2OLE("Real"));
		trDataR.m_nType = DBTYPE_R8;
		m_rgRowData.Add(trDataR);

		lstrcpyW(trDataS.m_szName,A2OLE("String"));
		trDataS.m_nType = DBTYPE_STR;
		trDataS.m_ulSize = 256;
		trDataS.m_bUnsignedAttribute = NULL;
		m_rgRowData.Add(trDataS);

		lstrcpyW(trDataBlob.m_szName,A2OLE("Geometry"));
		trDataS.m_nType = DBTYPE_IUNKNOWN;
		m_rgRowData.Add(trDataBlob);

		*pcRowsAffected = 4;
		return S_OK;
	}
};

/////////////////////////////////////////////////////////////////////////////
// CSFSessionSchemaOGISTables 
class OGISTables_Row
{
public:
	WCHAR	m_szAlias[4];
	WCHAR	m_szCatalog[4];
	WCHAR	m_szSchema[4];
	WCHAR	m_szTableName[129];
	WCHAR	m_szColumnName[129];
	WCHAR	m_szDGName[129];

	OGISTables_Row()
	{
		m_szAlias[0] = NULL;
		m_szCatalog[0] = NULL;
		m_szSchema[0] = NULL;
		m_szTableName[0] = NULL;
		m_szColumnName[0] = NULL;
		m_szDGName[0] = NULL;
	}

BEGIN_PROVIDER_COLUMN_MAP(OGISTables_Row)
	PROVIDER_COLUMN_ENTRY("FEATURE_TABLE_ALIAS",1,m_szAlias)
	PROVIDER_COLUMN_ENTRY("TABLE_CATALOG",2,m_szCatalog)
	PROVIDER_COLUMN_ENTRY("TABLE_SCHEMA",3,m_szSchema)
	PROVIDER_COLUMN_ENTRY("TABLE_NAME",4,m_szTableName)
	PROVIDER_COLUMN_ENTRY("ID_COLUMN_NAME",5,m_szColumnName)
	PROVIDER_COLUMN_ENTRY("DG_COLUMN_NAME",6,m_szDGName)
END_PROVIDER_COLUMN_MAP()
};

class CSFSessionSchemaOGISTables:
public CRowsetImpl <CSFSessionSchemaOGISTables,OGISTables_Row, CSFSession>
{
  public:
    HRESULT Execute(LONG* pcRowsAffected, ULONG, const VARIANT*)
    {
        USES_CONVERSION;

        int				iLayer;
        IUnknown		*pIU;
        OGRDataSource	*poDS;
        OGRLayer		*pLayer;
        OGRFeatureDefn	*poDefn;

        CPLDebug( "OGR_OLEDB",
                  "CSFSessionSchemaOGISTables::Execute()." );
                
        QueryInterface(IID_IUnknown,(void **) &pIU);
        poDS = SFGetOGRDataSource(pIU);

        if (!poDS)
        {
            // Prep errors as well
            return S_FALSE;
        }

        for (iLayer = 0; iLayer < poDS->GetLayerCount(); iLayer++)
        {
            OGISTables_Row trData;

            pLayer = poDS->GetLayer(iLayer);
            poDefn = pLayer->GetLayerDefn();

            lstrcpyW(trData.m_szTableName,A2OLE(poDefn->GetName()));
            lstrcpyW(trData.m_szDGName,A2OLE("OGIS_GEOMETRY"));
            lstrcpyW(trData.m_szColumnName,A2OLE("FID"));

            m_rgRowData.Add(trData);
        }

        *pcRowsAffected = poDS->GetLayerCount();
        return S_OK;
    }
};

/////////////////////////////////////////////////////////////////////////////
// CSFSessionSchemaOGISGeoColumns

class OGISGeometry_Row
{
public:
	WCHAR	m_szCatalog[4];
	WCHAR	m_szSchema[4];
	WCHAR	m_szTableName[129];
	WCHAR	m_szColumnName[129];
	unsigned int m_nGeomType;
	int		m_nSpatialRefId;
#ifdef SUPPORT_ADSK_GEOM_EXTENT    
	BYTE    m_abyADSK_GEOM_EXTENT[93];
#endif        
	
	OGISGeometry_Row()
	{
		m_szCatalog[0] = NULL;
		m_szSchema[0] = NULL;
		m_szTableName[0] = NULL;
		m_szColumnName[0] = NULL;
		m_nGeomType = 0;
		m_nSpatialRefId = 0;
#ifdef SUPPORT_ADSK_GEOM_EXTENT    
                memset( m_abyADSK_GEOM_EXTENT, 0,
                        sizeof(m_abyADSK_GEOM_EXTENT) );
#endif                
	}

BEGIN_PROVIDER_COLUMN_MAP(OGISGeometry_Row)
	PROVIDER_COLUMN_ENTRY("TABLE_CATALOG",1,m_szCatalog)
	PROVIDER_COLUMN_ENTRY("TABLE_SCHEMA",2,m_szSchema)
	PROVIDER_COLUMN_ENTRY("TABLE_NAME",3,m_szTableName)
	PROVIDER_COLUMN_ENTRY("COLUMN_NAME",4,m_szColumnName)
	PROVIDER_COLUMN_ENTRY("GEOM_TYPE",5,m_nGeomType)
	PROVIDER_COLUMN_ENTRY("SPATIAL_REF_SYSTEM_ID",6,m_nSpatialRefId)
#ifdef SUPPORT_ADSK_GEOM_EXTENT    
	PROVIDER_COLUMN_ENTRY("ADSK_GEOM_EXTENT",7,m_abyADSK_GEOM_EXTENT)
#endif
END_PROVIDER_COLUMN_MAP()
};

class CSFSessionSchemaOGISGeoColumns:
public CCRRowsetImpl<CSFSessionSchemaOGISGeoColumns,OGISGeometry_Row,CSFSession>
{
  public:
    DBSTATUS GetRCDBStatus(CSimpleRow* poRC,
                           ATLCOLUMNINFO*poColInfo,
                           void *pSrcData)
        {
#ifdef SUPPORT_ADSK_GEOM_EXTENT            
            OGISGeometry_Row      *poRow = (OGISGeometry_Row *) pSrcData;

            if( lstrcmpW(poColInfo->pwszName,L"ADSK_GEOM_EXTENT") == 0 )
            {
                if( poRow->m_abyADSK_GEOM_EXTENT[0] == 0 )
                    return DBSTATUS_S_ISNULL;
            }
#endif            
            return DBSTATUS_S_OK;
        }

    HRESULT Execute(LONG* pcRowsAffected, ULONG, const VARIANT*)
	{
            USES_CONVERSION;

            int				iLayer;
            IUnknown		*pIU;
            OGRDataSource	*poDS;
            OGRLayer		*pLayer;
            OGRFeatureDefn	*poDefn;

            CPLDebug( "OGR_OLEDB",
                      "CSFSessionSchemaOGISGeoColumns::Execute()." );
                
            QueryInterface(IID_IUnknown,(void **) &pIU);
            poDS = SFGetOGRDataSource(pIU);

            if (!poDS)
            {
                // Prep errors as well
                return S_FALSE;
            }

            for (iLayer = 0; iLayer < poDS->GetLayerCount(); iLayer++)
            {
                OGISGeometry_Row trData;
                char             *pszWKT = NULL;

                pLayer = poDS->GetLayer(iLayer);
                poDefn = pLayer->GetLayerDefn();

                lstrcpyW(trData.m_szTableName,A2OLE(poDefn->GetName()));
                lstrcpyW(trData.m_szColumnName,A2OLE("OGIS_GEOMETRY"));
                trData.m_nGeomType = SFWkbGeomTypeToDBGEOM(poDefn->GetGeomType());
                QueryInterface(IID_IUnknown,(void **) &pIU);
                pszWKT = SFGetLayerWKT( pLayer, pIU );
                
                if( pszWKT != NULL && strlen(pszWKT) > 0 )
                {
                    OGRFree( pszWKT );
                    trData.m_nSpatialRefId = iLayer+1;
                }
                else
                    trData.m_nSpatialRefId = poDS->GetLayerCount() + 1;

#ifdef SUPPORT_ADSK_GEOM_EXTENT    
                OGREnvelope sExtent;
                
                if( pLayer->GetExtent( &sExtent, FALSE ) == OGRERR_NONE )
                {
                    OGRPolygon oExtentPoly;
                    OGRLinearRing oExtentRing;

                    oExtentRing.addPoint( sExtent.MinX, sExtent.MinY );
                    oExtentRing.addPoint( sExtent.MinX, sExtent.MaxY );
                    oExtentRing.addPoint( sExtent.MaxX, sExtent.MaxY );
                    oExtentRing.addPoint( sExtent.MaxX, sExtent.MinY );
                    oExtentRing.addPoint( sExtent.MinX, sExtent.MinY );

                    oExtentPoly.addRing( &oExtentRing );

                    CPLDebug( "FME_OLEDB",
                              "ADSK_GEOM_EXTENT(%f,%f,%f,%f) -> %d bytes",
                              sExtent.MinX, sExtent.MaxX,
                              sExtent.MinY, sExtent.MaxY,
                              oExtentPoly.WkbSize() );
                    
                    oExtentPoly.exportToWkb( wkbNDR,
                                             trData.m_abyADSK_GEOM_EXTENT+0 );
                }
#endif

                m_rgRowData.Add(trData);
            }

            *pcRowsAffected = poDS->GetLayerCount();

            return S_OK;
	}
};



/////////////////////////////////////////////////////////////////////////////
// CSFSessionSchemaSpatRef
// Note quite sure what to do with this yet!
class OGISSpat_Row
{
  public:
    int		m_nSpatialRefId;
    WCHAR	m_szAuthorityName[129];
    WCHAR	m_nAuthorityId;
    WCHAR	m_pszSpatialRefSystem[10240];

    OGISSpat_Row()
	{
            m_nSpatialRefId = 0;
            m_szAuthorityName[0] = NULL;
            m_nAuthorityId = 0;
            lstrcpyW(m_pszSpatialRefSystem,L"" );
	}

    BEGIN_PROVIDER_COLUMN_MAP(OGISSpat_Row)
	PROVIDER_COLUMN_ENTRY("SPATIAL_REF_SYSTEM_ID",1,m_nSpatialRefId)
	PROVIDER_COLUMN_ENTRY("AUTHORITY_NAME",2,m_szAuthorityName)
	PROVIDER_COLUMN_ENTRY("AUTHORITY_ID",3,m_nAuthorityId)
	PROVIDER_COLUMN_ENTRY_WSTR("SPATIAL_REF_SYSTEM_WKT",4,m_pszSpatialRefSystem)
        END_PROVIDER_COLUMN_MAP()
};


class CSFSessionSchemaSpatRef:
public CCRRowsetImpl<CSFSessionSchemaSpatRef,OGISSpat_Row,CSFSession>
{
  public:
    DBSTATUS GetRCDBStatus(CSimpleRow* poRC,
                           ATLCOLUMNINFO*poColInfo,
                           void *pSrcData)
        {
            OGISSpat_Row      *poRow = (OGISSpat_Row *) pSrcData;

            if( lstrcmpW(poColInfo->pwszName,L"AUTHORITY_NAME") == 0
                ||lstrcmpW(poColInfo->pwszName,L"AUTHORITY_ID") == 0 )
            {
                if( lstrcmpW(poRow->m_szAuthorityName,L"") == 0 )
                    return DBSTATUS_S_ISNULL;
            }
            if( lstrcmpW(poColInfo->pwszName,L"SPATIAL_REF_SYSTEM_WKT") == 0 )
            {
                if( lstrcmpW(poRow->m_pszSpatialRefSystem,L"") == 0 )
                    return DBSTATUS_S_ISNULL;
            }
            
            return DBSTATUS_S_OK;
        }

    HRESULT Execute(LONG* pcRowsAffected, ULONG, const VARIANT*)
	{
            USES_CONVERSION;
            bool	bAddDefault = false;

            // See if we can get the Spatial reference system for each layer.
            // It is unclear at the current time what the valid authority id and spatial
            // ref ids are.  
            int				iLayer;
            IUnknown		*pIU;
            OGRDataSource	*poDS;
            OGRLayer		*pLayer;

            CPLDebug( "OGR_OLEDB",
                      "CSFSessionSchemaSpatRef::Execute()." );
                
            QueryInterface(IID_IUnknown,(void **) &pIU);
            poDS = SFGetOGRDataSource(pIU);

            if (!poDS)
            {
                // Prep errors as well
                return S_FALSE;
            }

            for (iLayer = 0; iLayer < poDS->GetLayerCount(); iLayer++)
            {
                OGISSpat_Row trData;
                char *pszWKT = NULL;
				
                pLayer = poDS->GetLayer(iLayer);

                QueryInterface(IID_IUnknown,(void **) &pIU);
                pszWKT = SFGetLayerWKT( pLayer, pIU );

                if( pszWKT != NULL && strlen(pszWKT) > 0 )
                {
                    lstrcpyW(trData.m_szAuthorityName,A2OLE(""));
                    trData.m_nAuthorityId = 0;
                    trData.m_nSpatialRefId = iLayer+1;
                    lstrcpyW(trData.m_pszSpatialRefSystem,A2OLE(pszWKT));
                    
                    m_rgRowData.Add(trData);
                }
                else
                    bAddDefault = TRUE;

                OGRFree( pszWKT );
            }

            if (bAddDefault)
            {
                OGISSpat_Row trData;

                trData.m_nAuthorityId = 0;
                trData.m_nSpatialRefId = poDS->GetLayerCount() + 1;
                lstrcpyW(trData.m_szAuthorityName,A2OLE(""));
		lstrcpyW(trData.m_pszSpatialRefSystem,L"" );
			
                m_rgRowData.Add(trData);	
            }

            *pcRowsAffected = poDS->GetLayerCount();

            return S_OK;
	}
};

#endif //__CSFSession_H_
