/* -------------------------------------------------------------------- */
/*      Geometry factory methods.                                       */
/* -------------------------------------------------------------------- */

#ifndef SWIGJAVA
%feature( "kwargs" ) CreateGeometryFromWkb;
%newobject CreateGeometryFromWkb;
#ifndef SWIGCSHARP
%apply (int nLen, char *pBuf ) { (int len, char *bin_string)};
#else
%apply (void *buffer_ptr) {char *bin_string};
#endif
%inline %{
  OGRGeometryShadow* CreateGeometryFromWkb( int len, char *bin_string, 
                                            OSRSpatialReferenceShadow *reference=NULL ) {
    void *geom;
    OGRErr err = OGR_G_CreateFromWkb( (unsigned char *) bin_string,
                                      reference,
                                      &geom,
                                      len );
    if (err != 0 ) {
       CPLError(CE_Failure, err, "%s", OGRErrMessages(err));
       return NULL;
    }
    return (OGRGeometryShadow*) geom;
  }
 
%}
#endif
#ifndef SWIGCSHARP
%clear (int len, char *bin_string);
#else
%clear (char *bin_string);
#endif

#ifdef SWIGJAVA
%newobject CreateGeometryFromWkb;
%feature("kwargs") CreateGeometryFromWkb;
%inline {
OGRGeometryShadow* CreateGeometryFromWkb(int nLen, unsigned char *pBuf, 
                                            OSRSpatialReferenceShadow *reference=NULL ) {
    void *geom;
    OGRErr err = OGR_G_CreateFromWkb((unsigned char*) pBuf, reference, &geom, nLen);
    if (err != 0 ) {
       CPLError(CE_Failure, err, "%s", OGRErrMessages(err));
       return NULL;
    }
    return (OGRGeometryShadow*) geom;
  }
}
#endif

%feature( "kwargs" ) CreateGeometryFromWkt;
%apply (char **ignorechange) { (char **) };
%newobject CreateGeometryFromWkt;
%inline %{
  OGRGeometryShadow* CreateGeometryFromWkt( char **val, 
                                      OSRSpatialReferenceShadow *reference=NULL ) {
    void *geom;
    OGRErr err = OGR_G_CreateFromWkt(val,
                                      reference,
                                      &geom);
    if (err != 0 ) {
       CPLError(CE_Failure, err, "%s", OGRErrMessages(err));
       return NULL;
    }
    return (OGRGeometryShadow*) geom;
  }
 
%}
%clear (char **);

%newobject CreateGeometryFromGML;
%inline %{
  OGRGeometryShadow *CreateGeometryFromGML( const char * input_string ) {
    OGRGeometryShadow* geom = (OGRGeometryShadow*)OGR_G_CreateFromGML(input_string);
    return geom;
  }
 
%}

%newobject CreateGeometryFromJson;
%inline %{
  OGRGeometryShadow *CreateGeometryFromJson( const char * input_string ) {
    OGRGeometryShadow* geom = (OGRGeometryShadow*)OGR_G_CreateGeometryFromJson(input_string);
    return geom;
  }
 
%}

/************************************************************************/
/*                        Other misc functions.                         */
/************************************************************************/

%{
char const *OGRDriverShadow_get_name( OGRDriverShadow *h ) {
  return OGR_Dr_GetName( h );
}

char const *OGRDataSourceShadow_get_name( OGRDataSourceShadow *h ) {
  return OGR_DS_GetName( h );
}

char const *OGRDriverShadow_name_get( OGRDriverShadow *h ) {
  return OGR_Dr_GetName( h );
}

char const *OGRDataSourceShadow_name_get( OGRDataSourceShadow *h ) {
  return OGR_DS_GetName( h );
}
%}

#ifndef GDAL_BINDINGS
int OGRGetDriverCount();
#endif

int OGRGetOpenDSCount();

OGRErr OGRSetGenerate_DB2_V72_BYTE_ORDER(int bGenerate_DB2_V72_BYTE_ORDER);

void OGRRegisterAll();

%inline %{
  OGRDataSourceShadow* GetOpenDS(int ds_number) {
    OGRDataSourceShadow* layer = (OGRDataSourceShadow*) OGRGetOpenDS(ds_number);
    return layer;
  }
%}

%newobject Open;
%feature( "kwargs" ) Open;
%inline %{
  OGRDataSourceShadow* Open( const char *filename, int update =0 ) {
    CPLErrorReset();
    OGRDataSourceShadow* ds = (OGRDataSourceShadow*)OGROpen(filename,update,NULL);
    if( CPLGetLastErrorType() == CE_Failure && ds != NULL )
    {
        OGRReleaseDataSource(ds);
        ds = NULL;
    }
	
    return ds;
  }
%}

%newobject OpenShared;
%feature( "kwargs" ) OpenShared;
%inline %{
  OGRDataSourceShadow* OpenShared( const char *filename, int update =0 ) {
    CPLErrorReset();
    OGRDataSourceShadow* ds = (OGRDataSourceShadow*)OGROpenShared(filename,update,NULL);
    if( CPLGetLastErrorType() == CE_Failure && ds != NULL )
    {
        OGRReleaseDataSource(ds);
        ds = NULL;
    }
	
    return ds;
  }
%}

#ifndef GDAL_BINDINGS
%inline %{
OGRDriverShadow* GetDriverByName( char const *name ) {
  return (OGRDriverShadow*) OGRGetDriverByName( name );
}

OGRDriverShadow* GetDriver(int driver_number) {
  return (OGRDriverShadow*) OGRGetDriver(driver_number);
}
%}
#endif
