/******************************************************************************
 * $Id: typemaps_lua.i 8582 2005-10-11 14:11:43Z kruland $
 *
 * Name:     typemaps_lua.i
 * Project:  GDAL PHP Interface
 * Purpose:  GDAL Core SWIG Interface declarations.
 * Author:   Ivan Lucena, ivan.lucena@pmldnet.com
 *
*/

/*
 * Include the typemaps from swig library for returning of
 * standard types through arguments.
 */
%include "typemaps.i"

%import "ogr_error_map.i"

%typemap(out,fragment="OGRErrMessages") OGRErr
{
  /* %typemap(out) OGRErr */
  if ( result != 0 ) {
    lua_pushstring( L, OGRErrMessages(result) );
    SWIG_fail;
  }
}

