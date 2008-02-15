/******************************************************************************
 * $Id: $
 *
 * Name:     ogr_core.i
 * Project:  GDAL SWIG Interface
 * Purpose:  Typemaps for Delphi bindings
 * Author:   Stefano Moratto
 ******************************************************************************
 * Copyright (c) 2008, Stefano Moratto <stefano.moratto@gmail.com> 
 */

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