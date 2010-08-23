/******************************************************************************
 *
 * Project:  NETCDF Translator
 * Purpose:  Implements OGRNETCDFDriver
 * Author:   Brian Case, rush at winkey dot org
 *
 ******************************************************************************
 * Copyright (c) 2010, Brian Case
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

#include <ogrsf_frmts.h>
#include <ogr_geometry.h>

#include "ogrnetcdfgeom.h"

#include <netcdf.h>

OGRGeometry *nc2geom (
    int nNcid,
    long nFID,
    OGRSpatialReference *poSRS )
{

    OGRGeometry *poGeom = NULL;
    
    const char *pszYVar = CPLGetConfigOption ( "OGR_NETCDF_YVAR", "latitude" );
    const char *pszXVar = CPLGetConfigOption ( "OGR_NETCDF_XVAR", "longitude" );
    const char *pszZvar = CPLGetConfigOption ( "OGR_NETCDF_ZVAR", "elevation" );

    int nXVarId;
    int nYVarId;
    int nZVarId;
    int bHasZ = 0;
    
    if (NC_NOERR != nc_inq_varid (nNcid, pszXVar, &nXVarId) ||
        NC_NOERR != nc_inq_varid (nNcid, pszYVar, &nYVarId)) {
        return poGeom;
    }

    if (NC_NOERR == nc_inq_varid (nNcid, pszZvar, &nZVarId))
        bHasZ = 1;

    
    int nXDims; 
    int nYDims;
    int nZDims;
    
    nc_inq_varndims (nNcid, nXVarId, &nXDims);
    nc_inq_varndims (nNcid, nYVarId, &nYDims);

    /***** point *****/
    
    if (nXDims == 1 && nYDims == 1) {
        if (!bHasZ) {
            double x;
            double y;
            size_t count[1] = {1};
            size_t start[1] = {nFID};
            
            nc_get_vara_double(nNcid, nXVarId, start, count, &x);
            nc_get_vara_double(nNcid, nYVarId, start, count, &y);

            poGeom = new OGRPoint (x, y);
        }
        else {
            double x;
            double y;
            double z;
            size_t count[1] = {1};
            size_t start[1] = {nFID};

            nc_get_vara_double(nNcid, nXVarId, start, count, &x);
            nc_get_vara_double(nNcid, nYVarId, start, count, &y);
            nc_get_vara_double(nNcid, nZVarId, start, count, &z);

            poGeom = new OGRPoint (x, y, z);
            
        }

    }
    
    return poGeom;
}
    