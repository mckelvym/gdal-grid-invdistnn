%module ogr_srs_api
%include "typemaps.i"

//%pragma(delphi) CALLCONV = "cdecl" 


%header %{
#include <stdarg.h>
#include <ogr_srs_api.h>

%}

%import "ogr_core.i"
%pragma(delphi) USES = "cpl, ogr_core" 
%include "ogr_srs_api.h"



%insert(interface_functions_wrapper) %{

%}

%insert(implementation_functions_wrapper) %{

%}

%wrapper %{


%}