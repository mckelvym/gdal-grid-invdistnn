%rename (SpatialReference) OSRSpatialReferenceShadow;
class OSRSpatialReferenceShadow {
private:
  OSRSpatialReferenceShadow();
public:
%extend {


  %feature("kwargs") OSRSpatialReferenceShadow;
  OSRSpatialReferenceShadow( char const * wkt = "" ) {
    return (OSRSpatialReferenceShadow*) OSRNewSpatialReference(wkt);
  }

  ~OSRSpatialReferenceShadow() {
    if (OSRDereference( self ) == 0 ) {
      OSRDestroySpatialReference( self );
    }
  }


%newobject __str__;
  char *__str__() {
    char *buf = 0;
    OSRExportToPrettyWkt( self, &buf, 0 );
    return buf;
  }

  int IsSame( OSRSpatialReferenceShadow *rhs ) {
    return OSRIsSame( self, rhs );
  }

  int IsSameGeogCS( OSRSpatialReferenceShadow *rhs ) {
    return OSRIsSameGeogCS( self, rhs );
  }

  int IsGeographic() {
    return OSRIsGeographic(self);
  }

  int IsProjected() {
    return OSRIsProjected(self);
  }

  int IsLocal() {
    return OSRIsLocal(self);
  }

  OGRErr SetAuthority( const char * pszTargetKey,
                       const char * pszAuthority,
                       int nCode ) {
    return OSRSetAuthority( self, pszTargetKey, pszAuthority, nCode );
  }

  const char *GetAttrValue( const char *name, int child = 0 ) {
    return OSRGetAttrValue( self, name, child );
  }

/*
  const char *__getattr__( const char *name ) {
    return OSRGetAttrValue( self, name, 0 );
  }
*/

  OGRErr SetAttrValue( const char *name, const char *value ) {
    return OSRSetAttrValue( self, name, value ); 
  }

/*
  OGRErr __setattr__( const char *name, const char *value ) {
    return OSRSetAttrValue( self, name, value );
  }
*/

  OGRErr SetAngularUnits( const char*name, double to_radians ) {
    return OSRSetAngularUnits( self, name, to_radians );
  }

  double GetAngularUnits() {
    // Return code ignored.
    return OSRGetAngularUnits( self, 0 );
  }

  OGRErr SetLinearUnits( const char*name, double to_meters ) {
    return OSRSetLinearUnits( self, name, to_meters );
  }

  double GetLinearUnits() {
    // Return code ignored.
    return OSRGetLinearUnits( self, 0 );
  }

  const char *GetLinearUnitsName() {
    const char *name = 0;
    if ( OSRIsProjected( self ) ) {
      name = OSRGetAttrValue( self, "PROJCS|UNIT", 0 );
    }
    else if ( OSRIsLocal( self ) ) {
      name = OSRGetAttrValue( self, "LOCAL_CS|UNIT", 0 );
    }

    if (name != 0) 
      return name;

    return "Meter";
  }

  const char *GetAuthorityCode( const char *target_key ) {
    return OSRGetAuthorityCode( self, target_key );
  }

  const char *GetAuthorityName( const char *target_key ) {
    return OSRGetAuthorityName( self, target_key );
  }

  OGRErr SetUTM( int zone, int north =1 ) {
    return OSRSetUTM( self, zone, north );
  }

  OGRErr SetStatePlane( int zone, int is_nad83 = 1, char const *unitsname = "", double units = 0.0 ) {
    return OSRSetStatePlaneWithUnits( self, zone, is_nad83, unitsname, units );
  }

  OGRErr AutoIdentifyEPSG() {
    return OSRAutoIdentifyEPSG( self );
  }

  OGRErr SetProjection( char const *arg ) {
    return OSRSetProjection( self, arg );
  }

  OGRErr SetProjParm( const char *name, double val ) {
    return OSRSetProjParm( self, name, val ); 
  }

  double GetProjParm( const char *name, double default_val = 0.0 ) {
    // Return code ignored.
    return OSRGetProjParm( self, name, default_val, 0 );
  }

  OGRErr SetNormProjParm( const char *name, double val ) {
    return OSRSetNormProjParm( self, name, val );
  }

  double GetNormProjParm( const char *name, double default_val = 0.0 ) {
    // Return code ignored.
    return OSRGetNormProjParm( self, name, default_val, 0 );
  }

%feature( "kwargs" ) SetACEA;
  OGRErr SetACEA( double stdp1, double stdp2,
 		double clat, double clong,
                double fe, double fn ) {
    return OSRSetACEA( self, stdp1, stdp2, clat, clong, 
                       fe, fn );
  }    

%feature( "kwargs" ) SetAE;
  OGRErr SetAE( double clat, double clong,
              double fe, double fn ) {
    return OSRSetAE( self, clat, clong, 
                     fe, fn );
  }

%feature( "kwargs" ) SetBonne;
  OGRErr SetBonne( double stdp, double cm, double fe, double fn ) {
    return OSRSetBonne( self, stdp, cm, fe, fn );
  }

%feature( "kwargs" ) SetCEA;
  OGRErr SetCEA( double stdp1, double cm,
               double fe, double fn ) {
    return OSRSetCEA( self, stdp1, cm, 
                      fe, fn );
  }

%feature( "kwargs" ) SetCS;
  OGRErr SetCS( double clat, double clong,
              double fe, double fn ) {
    return OSRSetCS( self, clat, clong, 
                     fe, fn );
  }

%feature( "kwargs" ) SetEC;
  OGRErr SetEC( double stdp1, double stdp2,
              double clat, double clong,
              double fe, double fn ) {
    return OSRSetEC( self, stdp1, stdp2, clat, clong, 
                     fe, fn );
  }

%feature( "kwargs" ) SetEckertIV;
  OGRErr SetEckertIV( double cm,
                    double fe, double fn ) {
    return OSRSetEckertIV( self, cm, fe, fn);
  }

%feature( "kwargs" ) SetEckertVI;
  OGRErr SetEckertVI( double cm,
                    double fe, double fn ) {
    return OSRSetEckertVI( self, cm, fe, fn);
  }

%feature( "kwargs" ) SetEquirectangular;
  OGRErr SetEquirectangular( double clat, double clong,
                           double fe, double fn ) {
    return OSRSetEquirectangular( self, clat, clong, 
                                  fe, fn );
  }

%feature( "kwargs" ) SetGS;
  OGRErr SetGS( double cm,
              double fe, double fn ) {
    return OSRSetGS( self, cm, fe, fn );
  }
    
%feature( "kwargs" ) SetGH;
  OGRErr SetGH( double cm,
              double fe, double fn ) {
    return OSRSetGH( self, cm, fe, fn );
  }
    
%feature( "kwargs" ) SetGEOS;
  OGRErr SetGEOS( double cm, double satelliteheight,
                double fe, double fn ) {
    return OSRSetGEOS( self, cm, satelliteheight,
                       fe, fn );
  }
    
%feature( "kwargs" ) SetGnomonic;
  OGRErr SetGnomonic( double clat, double clong,
                    double fe, double fn ) {
    return OSRSetGnomonic( self, clat, clong, 
                           fe, fn );
  }

%feature( "kwargs" ) SetHOM;
  OGRErr SetHOM( double clat, double clong,
               double azimuth, double recttoskew,
               double scale,
               double fe, double fn ) {
    return OSRSetHOM( self, clat, clong, azimuth, recttoskew,
                      scale, fe, fn );
  }

%feature( "kwargs" ) SetHOM2PNO;
  OGRErr SetHOM2PNO( double clat,
                   double dfLat1, double dfLong1,
                   double dfLat2, double dfLong2,
                   double scale,
                   double fe, double fn ) {
    return OSRSetHOM2PNO( self, clat, dfLat1, dfLong1, dfLat2, dfLong2, 
                          scale, fe, fn );
  }

%feature( "kwargs" ) SetKrovak;
  OGRErr SetKrovak( double clat, double clong,
                  double azimuth, double pseudostdparallellat,
                  double scale, 
                  double fe, double fn ) {
    return OSRSetKrovak( self, clat, clong, 
                         azimuth, pseudostdparallellat, 
                         scale, fe, fn );
  }

%feature( "kwargs" ) SetLAEA;
  OGRErr SetLAEA( double clat, double clong,
                double fe, double fn ) {
    return OSRSetLAEA( self, clat, clong, 
                       fe, fn );
  }

%feature( "kwargs" ) SetLCC;
  OGRErr SetLCC( double stdp1, double stdp2,
               double clat, double clong,
               double fe, double fn ) {
    return OSRSetLCC( self, stdp1, stdp2, clat, clong, 
                      fe, fn );
  }

%feature( "kwargs" ) SetLCC1SP;
  OGRErr SetLCC1SP( double clat, double clong,
                  double scale,
                  double fe, double fn ) {
    return OSRSetLCC1SP( self, clat, clong, scale, 
                         fe, fn );
  }

%feature( "kwargs" ) SetLCCB;
  OGRErr SetLCCB( double stdp1, double stdp2,
                double clat, double clong,
                double fe, double fn ) {
    return OSRSetLCCB( self, stdp1, stdp2, clat, clong, 
                       fe, fn );
  }
    
%feature( "kwargs" ) SetMC;
  OGRErr SetMC( double clat, double clong,
              double fe, double fn ) {
    return OSRSetMC( self, clat, clong,    
                     fe, fn );
  }

%feature( "kwargs" ) SetMercator;
  OGRErr SetMercator( double clat, double clong,
                    double scale, 
                    double fe, double fn ) {
    return OSRSetMercator( self, clat, clong, 
                           scale, fe, fn );
  }

%feature( "kwargs" ) SetMollweide;
  OGRErr  SetMollweide( double cm,
                      double fe, double fn ) {
    return OSRSetMollweide( self, cm, 
                            fe, fn );
  }

%feature( "kwargs" ) SetNZMG;
  OGRErr SetNZMG( double clat, double clong,
                double fe, double fn ) {
    return OSRSetNZMG( self, clat, clong, 
                       fe, fn );
  }

%feature( "kwargs" ) SetOS;
  OGRErr SetOS( double dfOriginLat, double dfCMeridian,
              double scale,
              double fe,double fn) {
    return OSRSetOS( self, dfOriginLat, dfCMeridian, scale, 
                     fe, fn );
  }
    
%feature( "kwargs" ) SetOrthographic;
  OGRErr SetOrthographic( double clat, double clong,
                        double fe,double fn) {
    return OSRSetOrthographic( self, clat, clong, 
                               fe, fn );
  }

%feature( "kwargs" ) SetPolyconic;
  OGRErr SetPolyconic( double clat, double clong,
                     double fe, double fn ) {
    return OSRSetPolyconic( self, clat, clong, 
                            fe, fn );
  }

%feature( "kwargs" ) SetPS;
  OGRErr SetPS( double clat, double clong,
              double scale,
              double fe, double fn) {
    return OSRSetPS( self, clat, clong, scale,
                     fe, fn );
  }
    
%feature( "kwargs" ) SetRobinson;
  OGRErr SetRobinson( double clong, 
                    double fe, double fn ) {
    return OSRSetRobinson( self, clong, fe, fn );
  }
    
%feature( "kwargs" ) SetSinusoidal;
  OGRErr SetSinusoidal( double clong, 
                      double fe, double fn ) {
    return OSRSetSinusoidal( self, clong, fe, fn );
  }
    
%feature( "kwargs" ) SetStereographic;
  OGRErr SetStereographic( double clat, double clong,
                         double scale,
                         double fe,double fn) {
    return OSRSetStereographic( self, clat, clong, scale, 
                                fe, fn );
  }
    
%feature( "kwargs" ) SetSOC;
  OGRErr SetSOC( double latitudeoforigin, double cm,
               double fe, double fn ) {
    return OSRSetSOC( self, latitudeoforigin, cm,
	              fe, fn );
  }
    
%feature( "kwargs" ) SetTM;
  OGRErr SetTM( double clat, double clong,
              double scale,
              double fe, double fn ) {
    return OSRSetTM( self, clat, clong, scale, 
                     fe, fn );
  }

%feature( "kwargs" ) SetTMVariant;
  OGRErr SetTMVariant( const char *pszVariantName,
                     double clat, double clong,
                     double scale,
                     double fe, double fn ) {
    return OSRSetTMVariant( self, pszVariantName, clat, clong,  
                            scale, fe, fn );
  }

%feature( "kwargs" ) SetTMG;
  OGRErr SetTMG( double clat, double clong, 
               double fe, double fn ) {
    return OSRSetTMG( self, clat, clong, 
                      fe, fn );
  }

%feature( "kwargs" ) SetTMSO;
  OGRErr SetTMSO( double clat, double clong,
                double scale,
                double fe, double fn ) {
    return OSRSetTMSO( self, clat, clong, scale, 
                       fe, fn );
  }

%feature( "kwargs" ) SetVDG;
  OGRErr SetVDG( double clong,
               double fe, double fn ) {
    return OSRSetVDG( self, clong, fe, fn );
  }

  OGRErr SetWellKnownGeogCS( const char *name ) {
    return OSRSetWellKnownGeogCS( self, name );
  }

  OGRErr SetFromUserInput( const char *name ) {
    return OSRSetFromUserInput( self, name );
  }

  OGRErr CopyGeogCSFrom( OSRSpatialReferenceShadow *rhs ) {
    return OSRCopyGeogCSFrom( self, rhs );
  }

  OGRErr SetTOWGS84( double p1, double p2, double p3,
                     double p4 = 0.0, double p5 = 0.0,
                     double p6 = 0.0, double p7 = 0.0 ) {
    return OSRSetTOWGS84( self, p1, p2, p3, p4, p5, p6, p7 );
  }

  OGRErr GetTOWGS84( double argout[7] ) {
    return OSRGetTOWGS84( self, argout, 7 );
  }

  OGRErr SetLocalCS( const char *pszName ) {
    return OSRSetLocalCS( self, pszName );
  }

  OGRErr SetGeogCS( const char * pszGeogName,
                    const char * pszDatumName,
                    const char * pszEllipsoidName,
                    double dfSemiMajor, double dfInvFlattening,
                    const char * pszPMName = "Greenwich",
                    double dfPMOffset = 0.0,
                    const char * pszUnits = "degree",
                    double dfConvertToRadians =  0.0174532925199433 ) {
    return OSRSetGeogCS( self, pszGeogName, pszDatumName, pszEllipsoidName,
                         dfSemiMajor, dfInvFlattening,
                         pszPMName, dfPMOffset, pszUnits, dfConvertToRadians );
  }

  OGRErr SetProjCS( const char *name = "unnamed" ) {
    return OSRSetProjCS( self, name );
  }

%apply (char **ignorechange) { (char **) };
  OGRErr ImportFromWkt( char **ppszInput ) {
    return OSRImportFromWkt( self, ppszInput );
  }
%clear (char **);

  OGRErr ImportFromProj4( char *ppszInput ) {
    return OSRImportFromProj4( self, ppszInput );
  }
  
  OGRErr ImportFromUrl( char *url ) {
    return OSRImportFromUrl( self, url );
  }
%apply (char **options) { (char **) };
  OGRErr ImportFromESRI( char **ppszInput ) {
    return OSRImportFromESRI( self, ppszInput );
  }
%clear (char **);

  OGRErr ImportFromEPSG( int arg ) {
    return OSRImportFromEPSG(self, arg);
  }

  OGRErr ImportFromPCI( char const *proj, char const *units = "METRE",
                        double argin[17] = 0 ) {
    return OSRImportFromPCI( self, proj, units, argin );
  }

  OGRErr ImportFromUSGS( long proj_code, long zone = 0,
                         double argin[15] = 0,
                         long datum_code = 0 ) {
    return OSRImportFromUSGS( self, proj_code, zone, argin, datum_code );
  }

  OGRErr ImportFromXML( char const *xmlString ) {
    return OSRImportFromXML( self, xmlString );
  }

  OGRErr ExportToWkt( char **argout ) {
    return OSRExportToWkt( self, argout );
  }

  OGRErr ExportToPrettyWkt( char **argout, int simplify = 0 ) {
    return OSRExportToPrettyWkt( self, argout, simplify );
  }

  OGRErr ExportToProj4( char **argout ) {
    return OSRExportToProj4( self, argout );
  }

%apply (char **argout) { (char **) };
%apply (double *argout[ANY]) { (double *parms[17] ) };
  OGRErr ExportToPCI( char **proj, char **units, double *parms[17] ) {
    return OSRExportToPCI( self, proj, units, parms );
  }
%clear (char **);
%clear (double *parms[17]);

%apply (long *OUTPUT) { (long*) };
%apply (double *argout[ANY]) { (double *parms[15]) }
  OGRErr ExportToUSGS( long *code, long *zone, double *parms[15], long *datum ) {
    return OSRExportToUSGS( self, code, zone, parms, datum );
  }
%clear (long*);
%clear (double *parms[15]);

  OGRErr ExportToXML( char **argout, const char *dialect = "" ) {
    return OSRExportToXML( self, argout, dialect );
  }

%newobject CloneGeogCS;
  OSRSpatialReferenceShadow *CloneGeogCS() {
    return (OSRSpatialReferenceShadow*) OSRCloneGeogCS(self);
  }

/*
 * Commented out until the headers have changed to make OSRClone visible.
%newobject Clone;
  OSRSpatialReferenceShadow *Clone() {
    return (OSRSpatialReferenceShadow*) OSRClone(self);
  }
*/

  OGRErr Validate() {
    return OSRValidate(self);
  }

  OGRErr StripCTParms() {
    return OSRStripCTParms(self);
  }

  OGRErr FixupOrdering() {
    return OSRFixupOrdering(self);
  }

  OGRErr Fixup() {
    return OSRFixup(self);
  }

  OGRErr MorphToESRI() {
    return OSRMorphToESRI(self);
  }

  OGRErr MorphFromESRI() {
    return OSRMorphFromESRI(self);
  }


} /* %extend */
};
