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
#include "ogrnetcdffield.h"
#include "ogrnetcdffeat.h"

OGRFeature *nc2feat (
    int nNcid,
    int nVars,
    int nUnlimdimid,
    int nFID,
    OGRFeatureDefn * poFeatDefn,
    OGRSpatialReference *poSRS)
{

    OGRFeature *poFeat = new OGRFeature ( poFeatDefn );

    /***** geometry *****/

    OGRGeometry *poGeom = nc2geom ( nNcid, nFID, poSRS );
    poFeat->SetGeometryDirectly ( poGeom );

    /***** fields *****/

    nc2field (nNcid, nVars, nUnlimdimid, nFID, poFeat);

    return poFeat;
}