#ifndef _netCDFVarS_H_INCLUDED_
#define _netCDFVarS_H_INCLUDED_

CPLErr NCDFGetAttr( int nCdfId, int nVarId, const char *pszAttrName, 
                    char **pszValue );

//*
//* Simple structure to hold all variables and corresponding dimensions.
//*
struct netCDFDim
{
    char      szName[NC_MAX_NAME];
    size_t    nLen;
};

struct netCDFVar
{
    int          nCDF;
    int          nId;
    char         szName[NC_MAX_NAME];
    nc_type      nType;
    int          nDimCount;
    int*         panDimIds;
    netCDFDim*   paoDims;
    void*        paData;
    char         szType[NC_MAX_NAME];
    CPLString    osStdName;
    CPLString    osLongName;

    inline netCDFVar()
    {
        nCDF = -1;
        nId = -1;
        szName[0] = '\0';
        nType = NC_NAT;
        nDimCount = 0;
        panDimIds = 0;
        paoDims = 0;
        paData = 0;
        szType[0] = '\0';
    }
    
    inline netCDFVar( int nCDFID, int nVarId )
    {
        nCDF = nCDFID;
        nId = nVarId;
        szName[0] = '\0';
        nType = NC_NAT;
        nDimCount = 0;
        panDimIds = 0;
        paoDims = 0;
        paData = 0;
        szType[0] = '\0';

        nc_inq_varname( nCDF, nId, szName);
        nc_inq_vartype( nCDF, nId, &nType );
        switch( nType ) {

            case NC_BYTE:
                strcpy( szType, "8-bit integer" );
                break;
            case NC_CHAR:
                strcpy( szType, "8-bit character" );
                break;
            case NC_SHORT: 
                strcpy( szType, "16-bit integer" );
                break;
            case NC_INT:
                strcpy( szType, "32-bit integer" );
                break;
            case NC_FLOAT:
                strcpy( szType, "32-bit floating-point" );
                break;
            case NC_DOUBLE:
                strcpy( szType, "64-bit floating-point" );
                break;
#ifdef NETCDF_HAS_NC4
            case NC_UBYTE:
                strcpy( szType, "8-bit unsigned integer" );
                break;
            case NC_USHORT: 
                strcpy( szType, "16-bit unsigned integer" );
                break;
            case NC_UINT:
                strcpy( szType, "32-bit unsigned integer" );
                break;
            case NC_INT64:
                strcpy( szType, "64-bit integer" );
                break;
            case NC_UINT64:
                strcpy( szType, "64-bit unsigned integer" );
                break;
#endif    
            default:
                strcpy( szType, "unknown" );
                break;
        }
        nc_inq_varndims ( nCDF, nId, &nDimCount );
        if ( !nDimCount )
            return;
        panDimIds = (int *) CPLCalloc( nDimCount, sizeof( int ) );
        if ( !panDimIds )
            return;
        nc_inq_vardimid ( nCDF, nId, panDimIds );
        paoDims = (netCDFDim*) CPLCalloc( nDimCount, sizeof( netCDFDim ) );
        if ( !paoDims )
            return;
        for ( int i = 0; i < nDimCount; ++i )
        {
            nc_inq_dimlen ( nCDF, panDimIds[i], &paoDims[i].nLen );
            nc_inq_dimname( nCDF, panDimIds[i], paoDims[i].szName );
        }
        
        ReadBasicAttributes();
        Read1DVariable();
    }

    inline virtual ~netCDFVar()
    {
        if ( panDimIds )
            CPLFree( panDimIds );
        if ( paoDims )
            CPLFree( paoDims );
        if ( paData )
            CPLFree( paData );
    }
    
    int Read1DVariable()
    {
        int     status = NC_NOERR;
        size_t start[1], count[1];

        if ( nDimCount != 1 )
            return false;
        start[0] = 0;
        count[0] = paoDims[0].nLen;

        switch ( nType ) {
            case NC_CHAR:
                paData = CPLCalloc( paoDims[0].nLen + 1, sizeof( char ) );
                status = nc_get_vara_text( nCDF, nId, start, count, (char *)paData );
                ((char*)paData)[paoDims[0].nLen]='\0';
                break;
            case NC_BYTE:
                paData = CPLCalloc( paoDims[0].nLen, sizeof( signed char ) );
                status = nc_get_vara_schar( nCDF, nId, start, count, (signed char *)paData );
                break;
            case NC_SHORT:
                paData = CPLCalloc( paoDims[0].nLen, sizeof( short ) );
                status = nc_get_vara_short( nCDF, nId, start, count, (short *)paData );
                break;
            case NC_USHORT:
                paData = CPLCalloc( paoDims[0].nLen, sizeof( unsigned short ) );
                status = nc_get_vara_ushort( nCDF, nId, start, count, (unsigned short *)paData );
            break;
            case NC_INT:
                paData = CPLCalloc( paoDims[0].nLen, sizeof( int ) );
                status = nc_get_vara_int( nCDF, nId, start, count, (int *)paData );
                break;
            case NC_FLOAT:
                paData = CPLCalloc( paoDims[0].nLen, sizeof( float ) );
                status = nc_get_vara_float( nCDF, nId, start, count, (float *)paData );
                break;
            case NC_DOUBLE:
                paData = CPLCalloc(paoDims[0].nLen, sizeof(double) );
                status = nc_get_vara_double( nCDF, nId, start, count, (double *)paData );
                break;
            default:
                CPLDebug( "GDAL_netCDF", "NCDFGetVar1D unsupported type %d", nType );
                return NC_NOERR;
                break;
        }

        return status;
    }
    
    int ReadBasicAttributes()
    {
        char *pszTemp = 0;
        if ( NCDFGetAttr( nCDF, nId, "standard_name", &pszTemp ) == CE_None )
        {
            osStdName = pszTemp;
            CPLFree(pszTemp);
            pszTemp = 0;
        }
        if ( NCDFGetAttr( nCDF, nId, "long_name", &pszTemp ) == CE_None )
        {
            osLongName = pszTemp;
            CPLFree(pszTemp);
            pszTemp = 0;
        }
        return CE_None;
    }
    
};

typedef std::map< int,  netCDFVar* > netCDFVariableMap;

inline int findVariable( const netCDFVariableMap& oVars, const char* pszName )
{
    netCDFVariableMap::const_iterator oIr = oVars.begin();
    while ( oIr != oVars.end() ) {
        if ( EQUAL( (*oIr).second->szName, pszName ) )
            return (*oIr).first;
        ++oIr;
    }
    return -1;
}

inline void freeVariables( netCDFVariableMap& oVars )
{
    if ( oVars.empty() )
        return;
    
    netCDFVariableMap::iterator oIr = oVars.begin();
    while ( oIr != oVars.end() ) {
        if ( (*oIr).second )
        {
            delete (*oIr).second;
            (*oIr).second = 0;
        }
        ++oIr;
    }
    oVars.clear();
}

inline bool readVariables( int nCDFID, netCDFVariableMap& oVars )
{
    int nVarCount;
    
    nc_inq_nvars ( nCDFID, &nVarCount );
    for ( int nVar = 0; nVar < nVarCount; nVar++ )
    {
        netCDFVar* poCDFVar = new netCDFVar( nCDFID, nVar );
        if ( !poCDFVar )
        {
            CPLError( CE_Failure, CPLE_AppDefined,
                      "Failed to parse NETCDF: error reading variables into cache." );
            return false;
        }
        oVars[nVar] = poCDFVar;
    }    
    return true;
}

#endif
