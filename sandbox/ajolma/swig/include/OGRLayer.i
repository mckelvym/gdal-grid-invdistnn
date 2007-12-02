%rename (Layer) OGRLayerShadow;
class OGRLayerShadow {
  OGRLayerShadow();
  ~OGRLayerShadow();
public:
%extend {

  int GetRefCount() {
    return OGR_L_GetRefCount(self);
  }
  
  void SetSpatialFilter(OGRGeometryShadow* filter) {
    OGR_L_SetSpatialFilter (self, filter);
  }
  
  void SetSpatialFilterRect( double minx, double miny,
                             double maxx, double maxy) {
    OGR_L_SetSpatialFilterRect(self, minx, miny, maxx, maxy);                          
  }
  
  OGRGeometryShadow *GetSpatialFilter() {
    return (OGRGeometryShadow *) OGR_L_GetSpatialFilter(self);
  }

  OGRErr SetAttributeFilter(char* filter_string) {
    return OGR_L_SetAttributeFilter((OGRLayerShadow*)self, filter_string);
  }
  
  void ResetReading() {
    OGR_L_ResetReading(self);
  }
  
  const char * GetName() {
    return OGR_FD_GetName(OGR_L_GetLayerDefn(self));
  }
 
  const char * GetGeometryColumn() {
    return OGR_L_GetGeometryColumn(self);
  }
  
  const char * GetFIDColumn() {
    return OGR_L_GetFIDColumn(self);
  }

%newobject GetFeature;
  OGRFeatureShadow *GetFeature(long fid) {
    return (OGRFeatureShadow*) OGR_L_GetFeature(self, fid);
  }
  
%newobject GetNextFeature;
  OGRFeatureShadow *GetNextFeature() {
    return (OGRFeatureShadow*) OGR_L_GetNextFeature(self);
  }
  
  OGRErr SetNextByIndex(long new_index) {
    return OGR_L_SetNextByIndex(self, new_index);
  }
  
  OGRErr SetFeature(OGRFeatureShadow *feature) {
    return OGR_L_SetFeature(self, feature);
  }
  

  OGRErr CreateFeature(OGRFeatureShadow *feature) {
    return OGR_L_CreateFeature(self, feature);
  }
  
  OGRErr DeleteFeature(long fid) {
    return OGR_L_DeleteFeature(self, fid);
  }
  
  OGRErr SyncToDisk() {
    return OGR_L_SyncToDisk(self);
  }
  
  OGRFeatureDefnShadow *GetLayerDefn() {
    return (OGRFeatureDefnShadow*) OGR_L_GetLayerDefn(self);
  }

  %feature( "kwargs" ) GetFeatureCount;  
  int GetFeatureCount(int force=1) {
    return OGR_L_GetFeatureCount(self, force);
  }
  
#if defined(SWIGCSHARP)
  %feature( "kwargs" ) GetExtent;
  OGRErr GetExtent(OGREnvelope* extent, int force=1) {
    return OGR_L_GetExtent(self, extent, force);
  }
#else
  %feature( "kwargs" ) GetExtent;
  OGRErr GetExtent(double argout[4], int force=1) {
    return OGR_L_GetExtent(self, (OGREnvelope*)argout, force);
  }
#endif

  bool TestCapability(const char* cap) {
    return OGR_L_TestCapability(self, cap);
  }
  
  %feature( "kwargs" ) CreateField;
  OGRErr CreateField(OGRFieldDefnShadow* field_def, int approx_ok = 1) {
    return OGR_L_CreateField(self, field_def, approx_ok);
  }
  
  OGRErr StartTransaction() {
    return OGR_L_StartTransaction(self);
  }
  
  OGRErr CommitTransaction() {
    return OGR_L_CommitTransaction(self);
  }

  OGRErr RollbackTransaction() {
    return OGR_L_RollbackTransaction(self);
  }
  
  %newobject GetSpatialRef;
  OSRSpatialReferenceShadow *GetSpatialRef() {
    OGRSpatialReferenceH ref =  OGR_L_GetSpatialRef(self);
    if( ref )
        OSRReference(ref);
    return (OSRSpatialReferenceShadow*) ref;
  }
  
  GIntBig GetFeaturesRead() {
    return OGR_L_GetFeaturesRead(self);
  }

} /* %extend */


}; /* class OGRLayerShadow */
