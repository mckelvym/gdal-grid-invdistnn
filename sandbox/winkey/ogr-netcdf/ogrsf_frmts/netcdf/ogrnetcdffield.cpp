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

#include <ogr_feature.h>

#include "ogrnetcdffield.h"

#include <netcdf.h>

void nc2FeatureDef (
    int Ncid,
    int nVars,
    int nUnlimdimid,
    OGRFeatureDefn * poFeatureDefn )
{

    const char *pszYVar = CPLGetConfigOption ( "OGR_NETCDF_YVAR", "latitude" );
    const char *pszXVar = CPLGetConfigOption ( "OGR_NETCDF_XVAR", "longitude" );
    const char *pszZVar = CPLGetConfigOption ( "OGR_NETCDF_ZVAR", "elevation" );

    /***** go through the vars *****/
    
    int nVid;
    for (nVid = 0 ; nVid < nVars ; nVid++) {
        
        char szVName[NC_MAX_NAME+1];
        nc_type VType;
        int nVDims;
        int anDimIds[NC_MAX_VAR_DIMS];
        int nAtts;
        nc_inq_var (Ncid, nVid, szVName, &VType, &nVDims, anDimIds, &nAtts);

        if (EQUAL(pszXVar, szVName) || EQUAL(pszYVar, szVName) ||
            EQUAL(pszZVar, szVName))
            continue;
            
        if (anDimIds[0] == nUnlimdimid) {
            OGRFieldType eTypeIn;
            int nAddMe = 0;
            
            switch (VType) {
                
                case NC_BYTE:
                    
                    if (nVDims < 3) {
                        eTypeIn = OFTBinary;
                        nAddMe = 1;
                    }
                    
                    break;
                    
                case NC_CHAR:
                
                    if (nVDims == 2) {
                        eTypeIn = OFTString;
                        nAddMe = 1;
                    }
                    else if (nVDims == 3) {
                        eTypeIn = OFTStringList;
                        nAddMe = 1;
                    }

                    break;
                    
                case NC_SHORT:
                case NC_INT:
                            
                    if (nVDims == 1) {
                        eTypeIn = OFTInteger;
                        nAddMe = 1;
                    }
                    else if (nVDims == 2) {
                        eTypeIn = OFTIntegerList;
                        nAddMe = 1;
                    }
                    
                    break;
                    
                case NC_FLOAT:
                case NC_DOUBLE:
                    
                    if (nVDims == 1) {
                        eTypeIn = OFTReal;
                        nAddMe = 1;
                    }
                    else if (nVDims == 2) {
                        eTypeIn = OFTRealList;
                        nAddMe = 1;
                    }
                    break;
                    
                case NC_NAT:
                default:
                    break;
                
            }

            if (nAddMe) {
                OGRFieldDefn oFieldDefn (szVName, eTypeIn);
                poFeatureDefn->AddFieldDefn ( &oFieldDefn );
            }
                    
        }
    }

}