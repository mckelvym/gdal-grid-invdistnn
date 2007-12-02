/******************************************************************************
 * $Id: cpl_exceptions.i 12158 2007-09-15 10:21:40Z ajolma $
 *
 * Code for Optional Exception Handling through UseExceptions(),
 * DontUseExceptions()
 *
 * It uses CPLSetErrorHandler to provide a custom function
 * which notifies the bindings of errors. 
 *
 * This is not thread safe.
 *
 ******************************************************************************
 * Copyright (c) 2005, Kevin Ruland
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *****************************************************************************/

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

%exception {
    CPLErrorReset();
    $action
    CPLErr eclass = CPLGetLastErrorType();
    if ( eclass == CE_Failure || eclass == CE_Fatal ) {
#if defined(SWIGPERL)
      SWIG_exception_fail( SWIG_RuntimeError, CPLGetLastErrorMsg() );
#else
      SWIG_exception( SWIG_RuntimeError, CPLGetLastErrorMsg() );
#endif
    }

#if defined(SWIGPERL)
    /* 
    Make warnings regular Perl warnings. This duplicates the warning
    message if DontUseExceptions() is in effect (it is not by default).
    */
    if ( eclass == CE_Warning ) {
      warn( CPLGetLastErrorMsg() );
    }
#endif

}
