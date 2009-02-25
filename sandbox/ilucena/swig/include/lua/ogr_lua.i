/*
 * $Id: ogr_lua.i $
 *
 * lua specific code for ogr bindings.
 */

%init %{
  if ( OGRGetDriverCount() == 0 ) {
    OGRRegisterAll();
  }
%}

%include typemaps_lua.i
