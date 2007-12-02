%rename (Feature) OGRFeatureShadow;
class OGRFeatureShadow {
  OGRFeatureShadow();
public:
%extend {

  ~OGRFeatureShadow() {
    OGR_F_Destroy(self);
  }

  %feature("kwargs") OGRFeatureShadow;
  OGRFeatureShadow( OGRFeatureDefnShadow *feature_def = 0 ) {
      return (OGRFeatureShadow*) OGR_F_Create( feature_def );
  }

  OGRFeatureDefnShadow *GetDefnRef() {
    return (OGRFeatureDefnShadow*) OGR_F_GetDefnRef(self);
  }
  
  OGRErr SetGeometry(OGRGeometryShadow* geom) {
    return OGR_F_SetGeometry(self, geom);
  }

/* The feature takes over owernship of the geometry. */
%apply SWIGTYPE *DISOWN {OGRGeometryShadow *geom};
  OGRErr SetGeometryDirectly(OGRGeometryShadow* geom) {
    return OGR_F_SetGeometryDirectly(self, geom);
  }
%clear OGRGeometryShadow *geom;
  
  /* Feature owns its geometry */
  OGRGeometryShadow *GetGeometryRef() {
    return (OGRGeometryShadow*) OGR_F_GetGeometryRef(self);
  }
  
  %newobject Clone;
  OGRFeatureShadow *Clone() {
    return (OGRFeatureShadow*) OGR_F_Clone(self);
  }
  
  bool Equal(OGRFeatureShadow *feature) {
    return OGR_F_Equal(self, feature);
  }
  
  int GetFieldCount() {
    return OGR_F_GetFieldCount(self);
  }

  /* ---- GetFieldDefnRef --------------------- */
  OGRFieldDefnShadow *GetFieldDefnRef(int id) {
    return (OGRFieldDefnShadow *) OGR_F_GetFieldDefnRef(self, id);
  }

  OGRFieldDefnShadow *GetFieldDefnRef(const char* name) {
      int i = OGR_F_GetFieldIndex(self, name);
      if (i == -1)
	  CPLError(CE_Failure, 1, "No such field: '%s'", name);
      else
	  return (OGRFieldDefnShadow *) OGR_F_GetFieldDefnRef(self, i);
      return NULL;
  }
  /* ------------------------------------------- */

  /* ---- GetFieldAsString --------------------- */

  const char* GetFieldAsString(int id) {
    return (const char *) OGR_F_GetFieldAsString(self, id);
  }

  const char* GetFieldAsString(const char* name) {
      int i = OGR_F_GetFieldIndex(self, name);
      if (i == -1)
	  CPLError(CE_Failure, 1, "No such field: '%s'", name);
      else
	  return (const char *) OGR_F_GetFieldAsString(self, i);
      return NULL;
  }
  /* ------------------------------------------- */

  /* ---- GetFieldAsInteger -------------------- */

  int GetFieldAsInteger(int id) {
    return OGR_F_GetFieldAsInteger(self, id);
  }

  int GetFieldAsInteger(const char* name) {
      int i = OGR_F_GetFieldIndex(self, name);
      if (i == -1)
	  CPLError(CE_Failure, 1, "No such field: '%s'", name);
      else
	  return OGR_F_GetFieldAsInteger(self, i);
      return 0;
  }
  /* ------------------------------------------- */  

  /* ---- GetFieldAsDouble --------------------- */

  double GetFieldAsDouble(int id) {
    return OGR_F_GetFieldAsDouble(self, id);
  }

  double GetFieldAsDouble(const char* name) {
      int i = OGR_F_GetFieldIndex(self, name);
      if (i == -1)
	  CPLError(CE_Failure, 1, "No such field: '%s'", name);
      else
	  return OGR_F_GetFieldAsDouble(self, i);
      return 0;
  }
  /* ------------------------------------------- */  


  
  /* ---- IsFieldSet --------------------------- */
  bool IsFieldSet(int id) {
    return OGR_F_IsFieldSet(self, id);
  }

  bool IsFieldSet(const char* name) {
      int i = OGR_F_GetFieldIndex(self, name);
      if (i == -1)
	  CPLError(CE_Failure, 1, "No such field: '%s'", name);
      else
	  return OGR_F_IsFieldSet(self, i);
      return (bool)0;
  }
  /* ------------------------------------------- */  
      
  int GetFieldIndex(const char* name) {
      return OGR_F_GetFieldIndex(self, name);
  }

  int GetFID() {
    return OGR_F_GetFID(self);
  }
  
  OGRErr SetFID(int fid) {
    return OGR_F_SetFID(self, fid);
  }
  
  void DumpReadable() {
    OGR_F_DumpReadable(self, NULL);
  }

  void UnsetField(int id) {
    OGR_F_UnsetField(self, id);
  }


  void UnsetField(const char* name) {
      int i = OGR_F_GetFieldIndex(self, name);
      if (i == -1)
	  CPLError(CE_Failure, 1, "No such field: '%s'", name);
      else
	  OGR_F_UnsetField(self, i);
  }

  /* ---- SetField ----------------------------- */
  
  %apply ( tostring argin ) { (const char* value) };
  void SetField(int id, const char* value) {
    OGR_F_SetFieldString(self, id, value);
  }

  void SetField(const char* name, const char* value) {
      int i = OGR_F_GetFieldIndex(self, name);
      if (i == -1)
	  CPLError(CE_Failure, 1, "No such field: '%s'", name);
      else
	  OGR_F_SetFieldString(self, i, value);
  }
  %clear (const char* value );
  
  void SetField(int id, int value) {
    OGR_F_SetFieldInteger(self, id, value);
  }
  
  void SetField(const char* name, int value) {
      int i = OGR_F_GetFieldIndex(self, name);
      if (i == -1)
	  CPLError(CE_Failure, 1, "No such field: '%s'", name);
      else
	  OGR_F_SetFieldInteger(self, i, value);
  }
  
  void SetField(int id, double value) {
    OGR_F_SetFieldDouble(self, id, value);
  }
  
  void SetField(const char* name, double value) {
      int i = OGR_F_GetFieldIndex(self, name);
      if (i == -1)
	  CPLError(CE_Failure, 1, "No such field: '%s'", name);
      else
	  OGR_F_SetFieldDouble(self, i, value);
  }
  
  void SetField( int id, int year, int month, int day,
                             int hour, int minute, int second, 
                             int tzflag ) {
    OGR_F_SetFieldDateTime(self, id, year, month, day,
                             hour, minute, second, 
                             tzflag);
  }
  
  void SetField(const char* name, int year, int month, int day,
                             int hour, int minute, int second, 
                             int tzflag ) {
      int i = OGR_F_GetFieldIndex(self, name);
      if (i == -1)
	  CPLError(CE_Failure, 1, "No such field: '%s'", name);
      else
	  OGR_F_SetFieldDateTime(self, i, year, month, day,
				 hour, minute, second, 
				 tzflag);
  }

  /* ------------------------------------------- */  
  
  %feature("kwargs") SetFrom;
  OGRErr SetFrom(OGRFeatureShadow *other, int forgiving=1) {
    return OGR_F_SetFrom(self, other, forgiving);
  }
  
  const char *GetStyleString() {
    return (const char*) OGR_F_GetStyleString(self);
  }
  
  void SetStyleString(const char* the_string) {
    OGR_F_SetStyleString(self, the_string);
  }

  /* ---- GetFieldType ------------------------- */  
  OGRFieldType GetFieldType(int id) {
    return (OGRFieldType) OGR_Fld_GetType( OGR_F_GetFieldDefnRef( self, id));
  }
  
  OGRFieldType GetFieldType(const char* name) {
      int i = OGR_F_GetFieldIndex(self, name);
      if (i == -1) {
	  CPLError(CE_Failure, 1, "No such field: '%s'", name);
	  return (OGRFieldType)0;
      } else
	  return (OGRFieldType) OGR_Fld_GetType( 
	      OGR_F_GetFieldDefnRef( self,  i )
	      );
  }
  /* ------------------------------------------- */  
  
} /* %extend */


}; /* class OGRFeatureShadow */
