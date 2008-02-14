//-delphi -opaquerecord -noproxy -I$(ProgramFiles)\FWTOOLS2.0.3\include gdal.i 

%module gdal
%include <typemaps\typemaps.swg>

%header %{
#include <stdarg.h>
#include <gdal.h>

%}
%typemap(ctype)                double [ANY] "double *"
//%typemap(mememberin)             double [20] "double []"

%typemap(m3rawintype)   char ** 	"TArrayOfString" 

%typemap(m3rawintype)   void**  	"array of Pointer" 

%import "cpl.i"
%import "ogr_core.i"
%import "ogr_srs_api.i"

%pragma(delphi) USES = "cpl, ogr_core, ogr_srs_api" 

%typemap(m3rawintype)   int **     "PPInteger"
%typemap(m3rawinmode)   int **     ""

%include "gdal.h"

%insert(interface_begin) %{
type  

	PPInteger = ^PInteger;
	PPointer = ^Pointer;
	
	TArrayOfString = array of string;


%}


%insert(interface_functions_wrapper) %{

function ToStrings(const aDelimitedTxt: string): TArrayOfString;

%}

%insert(implementation_functions_wrapper) %{

function ToStrings(const aDelimitedTxt: string): TArrayOfString;
 var
   i                 : integer;
 begin
   with TStringList.Create do
   begin
     DelimitedText := aDelimitedTxt;
     SetLength(Result, Count + 1);
     for i := 0 to Count - 1 do
       Result[i] := Strings[i];
     Free;
   end;
 end;


%}

%wrapper %{

%}

