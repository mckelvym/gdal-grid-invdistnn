/************************************************************************/
/*                        GetWellKnownGeogCSAsWKT()                     */
/************************************************************************/
%inline %{
OGRErr GetWellKnownGeogCSAsWKT( const char *name, char **argout ) {
  OGRSpatialReferenceH srs = OSRNewSpatialReference("");
  OGRErr rcode = OSRSetWellKnownGeogCS( srs, name );
  if( rcode == OGRERR_NONE )
      rcode = OSRExportToWkt ( srs, argout );  
  OSRDestroySpatialReference( srs );
  return rcode;
}
%}

/************************************************************************/
/*                           GetUserInputAsWKT()                        */
/************************************************************************/
%inline %{
OGRErr GetUserInputAsWKT( const char *name, char **argout ) {
  OGRSpatialReferenceH srs = OSRNewSpatialReference("");
  OGRErr rcode = OSRSetFromUserInput( srs, name );
  if( rcode == OGRERR_NONE )
      rcode = OSRExportToWkt ( srs, argout );  
  OSRDestroySpatialReference( srs );
  return rcode;
}
%}

/************************************************************************/
/*                        GetProjectionMethods()                        */
/************************************************************************/
/*
 * Python has it's own custom interface to GetProjectionMethods().which returns
 * fairly complex structure.
 *
 * All other languages will have a more simplistic interface which is
 * exactly the same as the C api.
 * 
 */

#if !defined(SWIGPYTHON)
%apply (char **CSL) {(char **)};
char **OPTGetProjectionMethods();
%clear (char **);

%apply (char **CSL) {(char **)};
char **OPTGetParameterList( char *method, char **username );
%clear (char **);

void OPTGetParameterInfo( char *method, char *param, char **usrname,
                          char **type, double *defaultval );
#endif
