/******************************************************************************
 * $Id: $
 *
 * Name:     cpl.i
 * Project:  GDAL SWIG Interface
 * Purpose:  Typemaps for Delphi bindings
 * Author:   Stefano Moratto
 ******************************************************************************
 * Copyright (c) 2008, Stefano Moratto <stefano.moratto@gmail.com> 
 */
 
/* File : example.i */
%module cpl
%include "typemaps.i"

%header %{
#include <stdarg.h>
#include <cpl_error.h>
#include <cpl_port.h>

%}


//%pragma(delphi) CALLCONV = "cdecl" 

//%typemap(m3rawintype) ...  %{ %}

%typemap(m3rawintype) (const char *, const char *, ...)     %{ PChar; b:PChar %}
%typemap(m3rawintype) (CPLErr eErrClass, int err_no, const char *fmt, ... )  %{ CPLErr ;  err_no : integer; msg:PChar %}
    

//%ignore  CPLDebug( const char *, const char *, ... ) ;
//%varargs() CPLDebug;

//%ignore  CPLError(CPLErr eErrClass, int err_no, const char *fmt, ...);
%ignore  CPLErrorV(CPLErr, int, const char *, va_list );

%include "cpl_port.h"
%include "cpl_error.h"

DeclarePlaceHolder(interface_end, CFILE);




%insert(interface_functions_wrapper) %{


%}

%insert(implementation_functions_wrapper) %{


%}








%wrapper %{

%}