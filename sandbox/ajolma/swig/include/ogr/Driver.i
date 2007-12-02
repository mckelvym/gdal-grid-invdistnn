%rename (Driver) OGRDriverShadow;

class OGRDriverShadow {
  OGRDriverShadow();
  ~OGRDriverShadow();
public:
%extend {

%immutable;
  char const *name;
%mutable;

%newobject CreateDataSource;
%feature( "kwargs" ) CreateDataSource;
  OGRDataSourceShadow *CreateDataSource( const char *name, 
                                    char **options = 0 ) {
    OGRDataSourceShadow *ds = (OGRDataSourceShadow*) OGR_Dr_CreateDataSource( self, name, options);
    return ds;
  }
  
%newobject CopyDataSource;
%feature( "kwargs" ) CopyDataSource;
  OGRDataSourceShadow *CopyDataSource( OGRDataSourceShadow* copy_ds, 
                                  const char* name, 
                                  char **options = 0 ) {
    OGRDataSourceShadow *ds = (OGRDataSourceShadow*) OGR_Dr_CopyDataSource(self, copy_ds, name, options);
    return ds;
  }
  
%newobject Open;
%feature( "kwargs" ) Open;
  OGRDataSourceShadow *Open( const char* name, 
                        int update=0 ) {
    OGRDataSourceShadow* ds = (OGRDataSourceShadow*) OGR_Dr_Open(self, name, update);
    return ds;
  }

  int DeleteDataSource( const char *name ) {
    return OGR_Dr_DeleteDataSource( self, name );
  }

  bool TestCapability (const char *cap) {
    return OGR_Dr_TestCapability(self, cap);
  }
  
  const char * GetName() {
    return OGR_Dr_GetName( self );
  }


} /* %extend */
}; /* class OGRDriverShadow */
