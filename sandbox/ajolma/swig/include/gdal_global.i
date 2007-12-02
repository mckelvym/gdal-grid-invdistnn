const char *GDALVersionInfo( const char *request );

void GDALAllRegister();

int GDALGetCacheMax();

void GDALSetCacheMax( int nBytes );
    
int GDALGetCacheUsed();
    
int GDALGetDataTypeSize( GDALDataType );

int GDALDataTypeIsComplex( GDALDataType );

const char *GDALGetDataTypeName( GDALDataType );

GDALDataType GDALGetDataTypeByName( const char * );

const char *GDALGetColorInterpretationName( GDALColorInterp );

const char *GDALGetPaletteInterpretationName( GDALPaletteInterp );

const char *GDALDecToDMS( double, const char *, int );

double GDALPackedDMSToDec( double );

double GDALDecToPackedDMS( double );

CPLXMLNode *CPLParseXMLString( char * );

char *CPLSerializeXMLTree( CPLXMLNode *xmlnode );

//************************************************************************
//
// Define the factory functions for Drivers and Datasets
//
//************************************************************************

// Missing
// GetDriverList

%inline %{
int GetDriverCount() {
  return GDALGetDriverCount();
}
%}

%inline %{
GDALDriverShadow* GetDriverByName( char const *name ) {
  return (GDALDriverShadow*) GDALGetDriverByName( name );
}
%}

%inline %{
GDALDriverShadow* GetDriver( int i ) {
  return (GDALDriverShadow*) GDALGetDriver( i );
}
%}

%newobject Open;
%inline %{
GDALDatasetShadow* Open( char const* name, GDALAccess eAccess = GA_ReadOnly ) {
  CPLErrorReset();
  GDALDatasetShadow *ds = GDALOpen( name, eAccess );
  if( ds != NULL && CPLGetLastErrorType() == CE_Failure )
  {
      if ( GDALDereferenceDataset( ds ) <= 0 )
          GDALClose(ds);
      ds = NULL;
  }
  return (GDALDatasetShadow*) ds;
}
%}

%newobject OpenShared;
%inline %{
GDALDatasetShadow* OpenShared( char const* name, GDALAccess eAccess = GA_ReadOnly ) {
  CPLErrorReset();
  GDALDatasetShadow *ds = GDALOpenShared( name, eAccess );
  if( ds != NULL && CPLGetLastErrorType() == CE_Failure )
  {
      if ( GDALDereferenceDataset( ds ) <= 0 )
          GDALClose(ds);
      ds = NULL;
  }
  return (GDALDatasetShadow*) ds;
}
%}

%apply (char **options) {char **papszSiblings};
%inline %{
GDALDriverShadow *IdentifyDriver( const char *pszDatasource, 
				  char **papszSiblings = NULL ) {
    return (GDALDriverShadow *) GDALIdentifyDriver( pszDatasource, 
	                                            papszSiblings );
}
%}
%clear char **papszSiblings;
