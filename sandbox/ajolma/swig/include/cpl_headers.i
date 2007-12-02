%inline %{
  void VeryQuiteErrorHandler(CPLErr eclass, int code, const char *msg ) {
    /* If the error class is CE_Fatal, we want to have a message issued
       because the CPL support code does an abort() before any exception
       can be generated */
    if (eclass == CE_Fatal ) {
      CPLDefaultErrorHandler(eclass, code, msg );
    }
  }

  void UseExceptions() {
    CPLSetErrorHandler( (CPLErrorHandler) VeryQuiteErrorHandler );
  }
  
  void DontUseExceptions() {
    CPLSetErrorHandler( CPLDefaultErrorHandler );
  }

#include "ogr_core.h"
  static char const *
    OGRErrMessages( int rc ) {
    switch( rc ) {
    case OGRERR_NONE:
      return "OGR Error: None";
    case OGRERR_NOT_ENOUGH_DATA:
      return "OGR Error: Not enough data to deserialize";
    case OGRERR_NOT_ENOUGH_MEMORY:
      return "OGR Error: Not enough memory";
    case OGRERR_UNSUPPORTED_GEOMETRY_TYPE:
      return "OGR Error: Unsupported geometry type";
    case OGRERR_UNSUPPORTED_OPERATION:
      return "OGR Error: Unsupported operation";
    case OGRERR_CORRUPT_DATA:
      return "OGR Error: Corrupt data";
    case OGRERR_FAILURE:
      return "OGR Error: General Error";
    case OGRERR_UNSUPPORTED_SRS:
      return "OGR Error: Unsupported SRS";
    case OGRERR_INVALID_HANDLE:
      return "OGR Error: Invalid handle";
    default:
      return "OGR Error: Unknown";
    }
  }

  void Debug( const char *msg_class, const char *message ) {
    CPLDebug( msg_class, message );
  }
  void Error( CPLErr msg_class = CE_Failure, int err_code = 0, const char* msg = "error" ) {
    CPLError( msg_class, err_code, msg );
  }

  CPLErr PushErrorHandler( char const * pszCallbackName = "CPLQuietErrorHandler" ) {
    CPLErrorHandler pfnHandler = NULL;
    if( EQUAL(pszCallbackName,"CPLQuietErrorHandler") )
      pfnHandler = CPLQuietErrorHandler;
    else if( EQUAL(pszCallbackName,"CPLDefaultErrorHandler") )
      pfnHandler = CPLDefaultErrorHandler;
    else if( EQUAL(pszCallbackName,"CPLLoggingErrorHandler") )
      pfnHandler = CPLLoggingErrorHandler;

    if ( pfnHandler == NULL )
      return CE_Fatal;

    CPLPushErrorHandler( pfnHandler );

    return CE_None;
  }

%}
