%rename (FieldDefn) OGRFieldDefnShadow;

class OGRFieldDefnShadow {
  OGRFieldDefnShadow();
public:
%extend {

  ~OGRFieldDefnShadow() {
    OGR_Fld_Destroy(self);
  }

  %feature("kwargs") OGRFieldDefnShadow;
  OGRFieldDefnShadow( const char* name_null_ok="unnamed", 
                      OGRFieldType field_type=OFTString) {
    return (OGRFieldDefnShadow*) OGR_Fld_Create(name_null_ok, field_type);
  }

  const char * GetName() {
    return (const char *) OGR_Fld_GetNameRef(self);
  }
  
  const char * GetNameRef() {
    return (const char *) OGR_Fld_GetNameRef(self);
  }
  
  void SetName( const char* name) {
    OGR_Fld_SetName(self, name);
  }
  
  OGRFieldType GetType() {
    return OGR_Fld_GetType(self);
  }

  void SetType(OGRFieldType type) {
    OGR_Fld_SetType(self, type);
  }
  
  OGRJustification GetJustify() {
    return OGR_Fld_GetJustify(self);
  }
  
  void SetJustify(OGRJustification justify) {
    OGR_Fld_SetJustify(self, justify);
  }
  
  int GetWidth () {
    return OGR_Fld_GetWidth(self);
  }
  
  void SetWidth (int width) {
    OGR_Fld_SetWidth(self, width);
  }
  
  int GetPrecision() {
    return OGR_Fld_GetPrecision(self);
  }
  
  void SetPrecision(int precision) {
    OGR_Fld_SetPrecision(self, precision);
  }

  const char * GetFieldTypeName(OGRFieldType type) {
    return OGR_GetFieldTypeName(type);
  }

} /* %extend */


}; /* class OGRFieldDefnShadow */
