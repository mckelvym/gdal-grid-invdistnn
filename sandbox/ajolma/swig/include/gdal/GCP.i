struct GDAL_GCP {
%extend {
%mutable;
  double GCPX;
  double GCPY;
  double GCPZ;
  double GCPPixel;
  double GCPLine;
  char *Info;
  char *Id;
%immutable;

  GDAL_GCP( double x, double y, double z = 0,
            double pixel = 0, double line = 0,
            const char *info = NULL, const char *id = NULL) {
    GDAL_GCP *self = (GDAL_GCP*) CPLMalloc( sizeof( GDAL_GCP ) );
    self->dfGCPX = x;
    self->dfGCPY = y;
    self->dfGCPZ = z;
    self->dfGCPPixel = pixel;
    self->dfGCPLine = line;
    self->pszInfo =  CPLStrdup( (info == 0) ? "" : info );
    self->pszId = CPLStrdup( (id==0)? "" : id );
    return self;
  }

  ~GDAL_GCP() {
    if ( self->pszInfo )
      CPLFree( self->pszInfo );
    if ( self->pszId )
      CPLFree( self->pszId );
    CPLFree( self );
  }


} /* extend */
}; /* GDAL_GCP */
%inline %{

    GDAL_GCP *new_GDAL_GCP( double x, double y, double z,
			    double pixel, double line,
			    const char *info, const char *id) {
	GDAL_GCP *self = (GDAL_GCP*) CPLMalloc( sizeof( GDAL_GCP ) );
	self->dfGCPX = x;
	self->dfGCPY = y;
	self->dfGCPZ = z;
	self->dfGCPPixel = pixel;
	self->dfGCPLine = line;
	self->pszInfo =  CPLStrdup( (info == 0) ? "" : info );
	self->pszId = CPLStrdup( (id==0)? "" : id );
	return self;
    }

double GDAL_GCP_GCPX_get( GDAL_GCP *h ) {
  return h->dfGCPX;
}
void GDAL_GCP_GCPX_set( GDAL_GCP *h, double val ) {
  h->dfGCPX = val;
}
double GDAL_GCP_GCPY_get( GDAL_GCP *h ) {
  return h->dfGCPY;
}
void GDAL_GCP_GCPY_set( GDAL_GCP *h, double val ) {
  h->dfGCPY = val;
}
double GDAL_GCP_GCPZ_get( GDAL_GCP *h ) {
  return h->dfGCPZ;
}
void GDAL_GCP_GCPZ_set( GDAL_GCP *h, double val ) {
  h->dfGCPZ = val;
}
double GDAL_GCP_GCPPixel_get( GDAL_GCP *h ) {
  return h->dfGCPPixel;
}
void GDAL_GCP_GCPPixel_set( GDAL_GCP *h, double val ) {
  h->dfGCPPixel = val;
}
double GDAL_GCP_GCPLine_get( GDAL_GCP *h ) {
  return h->dfGCPLine;
}
void GDAL_GCP_GCPLine_set( GDAL_GCP *h, double val ) {
  h->dfGCPLine = val;
}
const char * GDAL_GCP_Info_get( GDAL_GCP *h ) {
  return h->pszInfo;
}
void GDAL_GCP_Info_set( GDAL_GCP *h, const char * val ) {
  if ( h->pszInfo ) 
    CPLFree( h->pszInfo );
  h->pszInfo = CPLStrdup(val);
}
const char * GDAL_GCP_Id_get( GDAL_GCP *h ) {
  return h->pszId;
}
void GDAL_GCP_Id_set( GDAL_GCP *h, const char * val ) {
  if ( h->pszId ) 
    CPLFree( h->pszId );
  h->pszId = CPLStrdup(val);
}

%} //%inline 

%apply (IF_FALSE_RETURN_NONE) { (FALSE_IS_ERR) };
FALSE_IS_ERR GDALGCPsToGeoTransform( int nGCPs, GDAL_GCP const * pGCPs, 
    	                             double argout[6], int bApproxOK ); 
%clear (FALSE_IS_ERR);
