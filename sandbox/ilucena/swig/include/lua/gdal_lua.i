/*
 * $Id: gdal_lua.i $
 *
 * lua specific code for gdal bindings.
 */

%init %{
  if (GDALGetDriverCount() == 0 ) {
    GDALAllRegister();
  }
%}

%include typemaps_lua.i
