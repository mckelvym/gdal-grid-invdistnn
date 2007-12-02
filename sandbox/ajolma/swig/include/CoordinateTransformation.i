%rename (CoordinateTransformation) OSRCoordinateTransformationShadow;
class OSRCoordinateTransformationShadow {
private:
  OSRCoordinateTransformationShadow();
public:
%extend {

  OSRCoordinateTransformationShadow( OSRSpatialReferenceShadow *src, OSRSpatialReferenceShadow *dst ) {
    OSRCoordinateTransformationShadow *obj = (OSRCoordinateTransformationShadow*) OCTNewCoordinateTransformation( src, dst );
    if (obj == 0 ) {
      CPLError(CE_Failure, 1, "Failed to create coordinate transformation");
      return NULL;
    }
    return obj;
  }

  ~OSRCoordinateTransformationShadow() {
    OCTDestroyCoordinateTransformation( self );
  }

// Need to apply argin typemap second so the numinputs=1 version gets applied
// instead of the numinputs=0 version from argout.
%apply (double argout[ANY]) {(double inout[3])};
%apply (double argin[ANY]) {(double inout[3])};
  void TransformPoint( double inout[3] ) {
    OCTTransform( self, 1, &inout[0], &inout[1], &inout[2] );
  }
%clear (double inout[3]);

  void TransformPoint( double argout[3], double x, double y, double z = 0.0 ) {
    argout[0] = x;
    argout[1] = y;
    argout[2] = z;
    OCTTransform( self, 1, &argout[0], &argout[1], &argout[2] );
  }
  
#ifdef SWIGCSHARP
  %apply (double *inout) {(double*)};
#endif
  void TransformPoints( int nCount, double *x, double *y, double *z ) {
    OCTTransform( self, nCount, x, y, z );
  }
#ifdef SWIGCSHARP
  %clear (double*);
#endif

} /*extend */
};
