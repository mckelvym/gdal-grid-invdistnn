/******************************************************************************
 * $Id: $
 *
 * Name:     ogr_api.i
 * Project:  GDAL SWIG Interface
 * Purpose:  Typemaps for Delphi bindings
 * Author:   Stefano Moratto
 ******************************************************************************
 * Copyright (c) 2008, Stefano Moratto <stefano.moratto@gmail.com> 
 */

%module ogr_api
%include "typemaps.i"

//%pragma(delphi) CALLCONV = "cdecl" 

%header %{
//#include <stdafx.h>
#include <stdarg.h>
#include <ogr_api.h>

%}

%import "cpl.i"
%import "ogr_core.i"
%import "ogr_srs_api.i"

%pragma(delphi) USES = "cpl, ogr_core, ogr_srs_api" 


//DeclarePlaceHolder(interface_end, OGRGeometryH);
//DeclarePlaceHolder(interface_end, OGRSFDriverH);
//DeclarePlaceHolder(interface_end, CFILE);

%include "ogr_api.h"



%insert(interface_functions_wrapper) %{


%}

%insert(implementation_functions_wrapper) %{


%}




%wrapper %{


%}
