%rename (FeatureDefn) OGRFeatureDefnShadow;
class OGRFeatureDefnShadow {
  OGRFeatureDefnShadow();
public:
%extend {
  
  ~OGRFeatureDefnShadow() {
    /*OGR_FD_Destroy(self);*/
    OGR_FD_Release( OGRFeatureDefnH(self) );
  }

  %feature("kwargs") OGRFeatureDefnShadow;
  OGRFeatureDefnShadow(const char* name_null_ok=NULL) {
    OGRFeatureDefnH h = OGR_FD_Create(name_null_ok);
    OGR_FD_Reference(h);
    return (OGRFeatureDefnShadow* )h;
  }
  
  const char* GetName(){
    return OGR_FD_GetName(self);
  }
  
  int GetFieldCount(){
    return OGR_FD_GetFieldCount(self);
  }
  
  /* FeatureDefns own their FieldDefns */
  OGRFieldDefnShadow* GetFieldDefn(int i){
    return (OGRFieldDefnShadow*) OGR_FD_GetFieldDefn(self, i);
  }

  int GetFieldIndex(const char* name) {
      return OGR_FD_GetFieldIndex(self, name);
  }
  
  void AddFieldDefn(OGRFieldDefnShadow* defn) {
    OGR_FD_AddFieldDefn(self, defn);
  }
  
  OGRwkbGeometryType GetGeomType() {
    return (OGRwkbGeometryType) OGR_FD_GetGeomType(self);
  }
  
  void SetGeomType(OGRwkbGeometryType geom_type) {
    OGR_FD_SetGeomType(self, geom_type);
  }
  
  int GetReferenceCount(){
    return OGR_FD_GetReferenceCount(self);
  }
  
} /* %extend */


}; /* class OGRFeatureDefnShadow */
