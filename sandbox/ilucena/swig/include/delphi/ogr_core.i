/* File : example.i */
%module ogr_core
%include "typemaps.i"

%header %{
#include <stdarg.h>
#include <cpl_error.h>
#include <cpl_port.h>
#include <ogr_core.h>

%}



%import "cpl.i"
%include "ogr_core.h"

%pragma(delphi) USES = "cpl" 

DeclarePlaceHolder(interface_end, CFILE);




%insert(interface_functions_wrapper) %{


%}

%insert(implementation_functions_wrapper) %{


%}








%wrapper %{

%}