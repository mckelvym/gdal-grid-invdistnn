%rename (DataSource) OGRDataSourceShadow;

class OGRDataSourceShadow {
  OGRDataSourceShadow() {
  }
public:
%extend {

%immutable;
  char const *name;
%mutable;

  ~OGRDataSourceShadow() {
    OGRReleaseDataSource(self);
  }

  int GetRefCount() {
    return OGR_DS_GetRefCount(self);
  }
  
  int GetSummaryRefCount() {
    return OGR_DS_GetSummaryRefCount(self);
  }
  
  int GetLayerCount() {
    return OGR_DS_GetLayerCount(self);
  }
  
  OGRDriverShadow * GetDriver() {
    return (OGRLayerShadow *) OGR_DS_GetDriver( self );
  }

  const char * GetName() {
    return OGR_DS_GetName(self);
  }
  
  OGRErr DeleteLayer(int index){
    return OGR_DS_DeleteLayer(self, index);
  }

  /* Note that datasources own their layers */
  %feature( "kwargs" ) CreateLayer;
  OGRLayerShadow *CreateLayer(const char* name, 
              OSRSpatialReferenceShadow* srs=NULL,
              OGRwkbGeometryType geom_type=wkbUnknown,
              char** options=0) {
    OGRLayerShadow* layer = (OGRLayerShadow*) OGR_DS_CreateLayer( self,
								  name,
								  srs,
								  geom_type,
								  options);
    return layer;
  }

  %feature( "kwargs" ) CopyLayer;
  OGRLayerShadow *CopyLayer(OGRLayerShadow *src_layer,
            const char* new_name,
            char** options=0) {
    OGRLayerShadow* layer = (OGRLayerShadow*) OGR_DS_CopyLayer( self,
                                                      src_layer,
                                                      new_name,
                                                      options);
    return layer;
  }
  
  %feature( "kwargs" ) GetLayerByIndex;
  OGRLayerShadow *GetLayerByIndex( int index=0) {
    OGRLayerShadow* layer = (OGRLayerShadow*) OGR_DS_GetLayer(self, index);
    return layer;
  }

  OGRLayerShadow *GetLayerByName( const char* layer_name) {
    OGRLayerShadow* layer = (OGRLayerShadow*) OGR_DS_GetLayerByName(self, layer_name);
    return layer;
  }

  bool TestCapability(const char * cap) {
    return OGR_DS_TestCapability(self, cap);
  }

  %newobject ExecuteSQL;
  %feature( "kwargs" ) ExecuteSQL;
  OGRLayerShadow *ExecuteSQL(const char* statement,
                        OGRGeometryShadow* spatialFilter=NULL,
                        const char* dialect="") {
    OGRLayerShadow* layer = (OGRLayerShadow*) OGR_DS_ExecuteSQL((OGRDataSourceShadow*)self,
                                                      statement,
                                                      spatialFilter,
                                                      dialect);
    return layer;
  }
  
%apply SWIGTYPE *DISOWN {OGRLayerShadow *layer};
  void ReleaseResultSet(OGRLayerShadow *layer){
    OGR_DS_ReleaseResultSet(self, layer);
  }
%clear OGRLayerShadow *layer;

} /* %extend */


}; /* class OGRDataSourceShadow */
