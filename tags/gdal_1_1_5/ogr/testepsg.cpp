/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Test mainline for translating EPSG definitions into WKT.
 * Author:   Frank Warmerdam, warmerda@home.com
 *
 ******************************************************************************
 * Copyright (c) 2000, Frank Warmerdam
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
 ******************************************************************************
 *
 * $Log$
 * Revision 1.3  2001/01/19 21:10:47  warmerda
 * replaced tabs
 *
 * Revision 1.2  2000/03/20 14:57:27  warmerda
 * added -t for transform
 *
 * Revision 1.1  2000/03/16 19:05:07  warmerda
 * New
 *
 */

#include "ogr_spatialref.h"
#include "cpl_conv.h"

void Usage()

{
    printf( "testepsg [-t src_csnum trg_csnum x y]* [csnum]*\n" );
    printf( "  -t: transform a coordinate from source GCS/PCS to target GCS/PCS\n" );
    printf( "  csnum's on their own are translated to WKT and printed.\n" );
}

int main( int nArgc, char ** papszArgv )

{
    OGRSpatialReference oSRS;
    int i;

    for( i = 1; i < nArgc; i++ )
    {
        if( EQUAL(papszArgv[i],"-t") && i < nArgc - 4 )
        {
            OGRSpatialReference oSourceSRS, oTargetSRS;
            OGRCoordinateTransformation *poCT;
            double                      x, y;
            
            oSourceSRS.importFromEPSG( atoi(papszArgv[i+1]) );
            oTargetSRS.importFromEPSG( atoi(papszArgv[i+2]) );
            
            poCT = OGRCreateCoordinateTransformation( &oSourceSRS,
                                                      &oTargetSRS );
            x = atof( papszArgv[i+3] );
            y = atof( papszArgv[i+4] );
            
            if( poCT == NULL || !poCT->Transform( 1, &x, &y ) )
                printf( "Transformation failed.\n" );
            else
                printf( "(%f,%f) -> (%f,%f)\n", 
                        atof( papszArgv[i+3] ),
                        atof( papszArgv[i+4] ),
                        x, y );
            
            i += 4;
        }
        else 
        {
            if( oSRS.importFromEPSG( atoi(papszArgv[i]) ) != OGRERR_NONE )
                printf( "Error occured translating %s.\n", 
                        papszArgv[i] );
            else
            {
                char  *pszWKT = NULL;
                
                oSRS.exportToWkt( &pszWKT );
                printf( "WKT[%s] = %s\n", 
                        papszArgv[i], pszWKT );
                CPLFree( pszWKT );

                printf( "\n" );

                oSRS.StripCTParms();
                oSRS.exportToWkt( &pszWKT );
                printf( "Old Style WKT[%s] = %s\n", 
                        papszArgv[i], pszWKT );
                CPLFree( pszWKT );
                printf( "\n------------------------\n\n" );
            }
        }
    }
}
