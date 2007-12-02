%rename (Geometry) OGRGeometryShadow;
class OGRGeometryShadow {
  OGRGeometryShadow();
public:
%extend {
    
  ~OGRGeometryShadow() {
    OGR_G_DestroyGeometry( self );
  }
  
#ifndef SWIGJAVA
#ifdef SWIGCSHARP
%apply (void *buffer_ptr) {char *wkb_buf};
#endif
  %feature("kwargs") OGRGeometryShadow;
  OGRGeometryShadow( OGRwkbGeometryType type = wkbUnknown, char *wkt = 0, int wkb= 0, char *wkb_buf = 0, char *gml = 0 ) {
    if (type != wkbUnknown ) {
      return (OGRGeometryShadow*) OGR_G_CreateGeometry( type );
    }
    else if ( wkt != 0 ) {
      return CreateGeometryFromWkt( &wkt );
    }
    else if ( wkb != 0 ) {
      return CreateGeometryFromWkb( wkb, wkb_buf );
    }
    else if ( gml != 0 ) {
      return CreateGeometryFromGML( gml );
    }
    // throw?
    else return 0;
  }  
#ifdef SWIGCSHARP
%clear (char *wkb_buf);
#endif
#endif

  OGRErr ExportToWkt( char** argout ) {
    return OGR_G_ExportToWkt(self, argout);
  }

#ifndef SWIGCSHARP
#ifndef SWIGJAVA
  %feature("kwargs") ExportToWkb;
  OGRErr ExportToWkb( int *nLen, char **pBuf, OGRwkbByteOrder byte_order=wkbXDR ) {
    *nLen = OGR_G_WkbSize( self );
    *pBuf = (char *) malloc( *nLen * sizeof(unsigned char) );
    return OGR_G_ExportToWkb(self, byte_order, (unsigned char*) *pBuf );
  }
#endif
#endif

  const char * ExportToGML() {
    return (const char *) OGR_G_ExportToGML(self);
  }
  
  const char * ExportToKML(const char* altitude_mode=NULL) {
    return (const char *) OGR_G_ExportToKML(self, altitude_mode);
  }

  const char * ExportToJson() {
    return (const char *) OGR_G_ExportToJson(self);
  }

  %feature("kwargs") AddPoint;
  void AddPoint(double x, double y, double z = 0) {
    OGR_G_AddPoint( self, x, y, z );
  }
  
  void AddPoint_2D(double x, double y) {
    OGR_G_AddPoint_2D( self, x, y );
  }

/* The geometry now owns an inner geometry */
%apply SWIGTYPE *DISOWN {OGRGeometryShadow* other_disown};
  OGRErr AddGeometryDirectly( OGRGeometryShadow* other_disown ) {
    return OGR_G_AddGeometryDirectly( self, other_disown );
  }
%clear OGRGeometryShadow* other_disown;

  OGRErr AddGeometry( OGRGeometryShadow* other ) {
    return OGR_G_AddGeometry( self, other );
  }

  %newobject Clone;
  OGRGeometryShadow* Clone() {
    return (OGRGeometryShadow*) OGR_G_Clone(self);
  } 
    
  OGRwkbGeometryType GetGeometryType() {
    return (OGRwkbGeometryType) OGR_G_GetGeometryType(self);
  }

  const char * GetGeometryName() {
    return (const char *) OGR_G_GetGeometryName(self);
  }
  
  double GetArea() {
    return OGR_G_GetArea(self);
  }
  
  int GetPointCount() {
    return OGR_G_GetPointCount(self);
  }

  %feature("kwargs") GetX;  
  double GetX(int point=0) {
    return OGR_G_GetX(self, point);
  }

  %feature("kwargs") GetY;  
  double GetY(int point=0) {
    return OGR_G_GetY(self, point);
  }

  %feature("kwargs") GetZ;  
  double GetZ(int point=0) {
    return OGR_G_GetZ(self, point);
  } 

  void GetPoint(int iPoint = 0, double argout[3] = NULL) {
    OGR_G_GetPoint( self, iPoint, argout+0, argout+1, argout+2 );
  }

  void GetPoint_2D(int iPoint = 0, double argout[2] = NULL) {
    OGR_G_GetPoint( self, iPoint, argout+0, argout+1, NULL );
  }

  int GetGeometryCount() {
    return OGR_G_GetGeometryCount(self);
  }

  %feature("kwargs") SetPoint;    
  void SetPoint(int point, double x, double y, double z=0) {
    OGR_G_SetPoint(self, point, x, y, z);
  }

  %feature("kwargs") SetPoint_2D;
  void SetPoint_2D(int point, double x, double y) {
    OGR_G_SetPoint_2D(self, point, x, y);
  }
  
  /* Geometries own their internal geometries */
  OGRGeometryShadow* GetGeometryRef(int geom) {
    return (OGRGeometryShadow*) OGR_G_GetGeometryRef(self, geom);
  }

  %newobject GetBoundary;
  OGRGeometryShadow* GetBoundary() {
    return (OGRGeometryShadow*) OGR_G_GetBoundary(self);
  }  

  %newobject ConvexHull;
  OGRGeometryShadow* ConvexHull() {
    return (OGRGeometryShadow*) OGR_G_ConvexHull(self);
  } 

  %newobject Buffer;
  %feature("kwargs") Buffer; 
  OGRGeometryShadow* Buffer( double distance, int quadsecs=30 ) {
    return (OGRGeometryShadow*) OGR_G_Buffer( self, distance, quadsecs );
  }

  %newobject Intersection;
  OGRGeometryShadow* Intersection( OGRGeometryShadow* other ) {
    return (OGRGeometryShadow*) OGR_G_Intersection( self, other );
  }  
  
  %newobject Union;
  OGRGeometryShadow* Union( OGRGeometryShadow* other ) {
    return (OGRGeometryShadow*) OGR_G_Union( self, other );
  }  
  
  %newobject Difference;
  OGRGeometryShadow* Difference( OGRGeometryShadow* other ) {
    return (OGRGeometryShadow*) OGR_G_Difference( self, other );
  }  

  %newobject SymmetricDifference;
  OGRGeometryShadow* SymmetricDifference( OGRGeometryShadow* other ) {
    return (OGRGeometryShadow*) OGR_G_SymmetricDifference( self, other );
  } 
  
  double Distance( OGRGeometryShadow* other) {
    return OGR_G_Distance(self, other);
  }
  
  void Empty () {
    OGR_G_Empty(self);
  }

  bool IsEmpty () {
    return OGR_G_IsEmpty(self);
  }  
  
  bool IsValid () {
    return OGR_G_IsValid(self);
  }  
  
  bool IsSimple () {
    return OGR_G_IsSimple(self);
  }  
  
  bool IsRing () {
    return OGR_G_IsRing(self);
  }  
  
  bool Intersect (OGRGeometryShadow* other) {
    return OGR_G_Intersect(self, other);
  }

  bool Equal (OGRGeometryShadow* other) {
    return OGR_G_Equal(self, other);
  }
  
  bool Disjoint(OGRGeometryShadow* other) {
    return OGR_G_Disjoint(self, other);
  }

  bool Touches (OGRGeometryShadow* other) {
    return OGR_G_Touches(self, other);
  }

  bool Crosses (OGRGeometryShadow* other) {
    return OGR_G_Crosses(self, other);
  }

  bool Within (OGRGeometryShadow* other) {
    return OGR_G_Within(self, other);
  }

  bool Contains (OGRGeometryShadow* other) {
    return OGR_G_Contains(self, other);
  }
  
  bool Overlaps (OGRGeometryShadow* other) {
    return OGR_G_Overlaps(self, other);
  }

  OGRErr TransformTo(OSRSpatialReferenceShadow* reference) {
    return OGR_G_TransformTo(self, reference);
  }
  
  OGRErr Transform(OSRCoordinateTransformationShadow* trans) {
    return OGR_G_Transform(self, trans);
  }
  
  %newobject GetSpatialReference;
  OSRSpatialReferenceShadow* GetSpatialReference() {
    OGRSpatialReferenceH ref =  OGR_G_GetSpatialReference(self);
    if( ref )
        OSRReference(ref);
    return (OSRSpatialReferenceShadow*) ref;
  }
  
  void AssignSpatialReference(OSRSpatialReferenceShadow* reference) {
    OGR_G_AssignSpatialReference(self, reference);
  }
  
  void CloseRings() {
    OGR_G_CloseRings(self);
  }
  
  void FlattenTo2D() {
    OGR_G_FlattenTo2D(self);
  }

#if defined(SWIGCSHARP)  
  void GetEnvelope(OGREnvelope *env) {
    OGR_G_GetEnvelope(self, env);
  }
#else
  void GetEnvelope(double argout[4]) {
    OGR_G_GetEnvelope(self, (OGREnvelope*)argout);
  }
#endif  

#ifndef SWIGJAVA
  %newobject Centroid;
  OGRGeometryShadow* Centroid() {
    OGRGeometryShadow *pt = new_OGRGeometryShadow( wkbPoint );
    OGR_G_Centroid( self, pt );
    return pt;
  }
#endif
  
  int WkbSize() {
    return OGR_G_WkbSize(self);
  }
  
  int GetCoordinateDimension() {
    return OGR_G_GetCoordinateDimension(self);
  }

  void SetCoordinateDimension(int dimension) {
    OGR_G_SetCoordinateDimension(self, dimension);
  }
  
  int GetDimension() {
    return OGR_G_GetDimension(self);
  }

} /* %extend */

}; /* class OGRGeometryShadow */
