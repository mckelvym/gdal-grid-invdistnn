/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features
 * Purpose:  Functions for getting list of projection types, and their parms.
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
 * Revision 1.4  2003/05/28 19:16:42  warmerda
 * fixed up argument names and stuff for docs
 *
 * Revision 1.3  2001/07/18 05:03:05  warmerda
 * added CPL_CVSID
 *
 * Revision 1.2  2001/01/19 21:10:46  warmerda
 * replaced tabs
 *
 * Revision 1.1  2000/08/30 20:05:00  warmerda
 * New
 *
 */

#include "ogr_srs_api.h"
#include "cpl_string.h"

CPL_CVSID("$Id$");

static char *papszParameterDefinitions[] = {
    SRS_PP_CENTRAL_MERIDIAN,    "Central Meridian",     "Long",  "0.0",
    SRS_PP_SCALE_FACTOR,        "Scale Factor",         "Ratio", "1.0",
    SRS_PP_STANDARD_PARALLEL_1, "Standard Parallel 1",  "Lat",   "0.0",
    SRS_PP_STANDARD_PARALLEL_2, "Standard Parallel 2",  "Lat",   "0.0",
    SRS_PP_LONGITUDE_OF_CENTER, "Longitude of Center",  "Long",  "0.0",
    SRS_PP_LATITUDE_OF_CENTER,  "Latitude of Center",   "Lat",   "0.0",
    SRS_PP_LONGITUDE_OF_ORIGIN, "Longitude of Origin",  "Long",  "0.0",
    SRS_PP_LATITUDE_OF_ORIGIN,  "Latitude of Origin",   "Lat",   "0.0",
    SRS_PP_FALSE_EASTING,       "False Easting",        "m",     "0.0",
    SRS_PP_FALSE_NORTHING,      "False Northing",       "m",     "0.0",
    SRS_PP_AZIMUTH,             "Azimuth",              "Angle", "0.0",
    SRS_PP_RECTIFIED_GRID_ANGLE,"Rectified Grid Angle", "Angle", "0.0", 
    NULL
};

static char *papszProjectionDefinitions[] = {

    "*", 
    SRS_PT_TRANSVERSE_MERCATOR,
    "Transverse Mercator",
    SRS_PP_LATITUDE_OF_ORIGIN,
    SRS_PP_CENTRAL_MERIDIAN, 
    SRS_PP_SCALE_FACTOR, 
    SRS_PP_FALSE_EASTING,
    SRS_PP_FALSE_NORTHING,

    "*", 
    SRS_PT_TRANSVERSE_MERCATOR_SOUTH_ORIENTED,
    "Transverse Mercator (South Oriented)",
    SRS_PP_LATITUDE_OF_ORIGIN, 
    SRS_PP_CENTRAL_MERIDIAN,
    SRS_PP_SCALE_FACTOR, 
    SRS_PP_FALSE_EASTING,
    SRS_PP_FALSE_NORTHING,

    "*",
    SRS_PT_TUNISIA_MINING_GRID,
    "Tunisia Mining Grid",
    SRS_PP_LATITUDE_OF_ORIGIN, 
    SRS_PP_CENTRAL_MERIDIAN, 
    SRS_PP_FALSE_EASTING, 
    SRS_PP_FALSE_NORTHING, 

    "*",
    SRS_PT_ALBERS_CONIC_EQUAL_AREA,
    "Albers Conic Equal Area",
    SRS_PP_STANDARD_PARALLEL_1,
    SRS_PP_STANDARD_PARALLEL_2,
    SRS_PP_LATITUDE_OF_CENTER, 
    SRS_PP_LONGITUDE_OF_CENTER,
    SRS_PP_FALSE_EASTING, 
    SRS_PP_FALSE_NORTHING,

    "*", 
    SRS_PT_AZIMUTHAL_EQUIDISTANT,
    "Azimuthal Equidistant",
    SRS_PP_LATITUDE_OF_CENTER, 
    SRS_PP_LONGITUDE_OF_CENTER,
    SRS_PP_FALSE_EASTING, 
    SRS_PP_FALSE_NORTHING,

    "*",
    SRS_PT_CYLINDRICAL_EQUAL_AREA,
    "Cylindrical Equal Area",
    SRS_PP_STANDARD_PARALLEL_1, 
    SRS_PP_CENTRAL_MERIDIAN, 
    SRS_PP_FALSE_EASTING, 
    SRS_PP_FALSE_NORTHING,

    "*", 
    SRS_PT_CASSINI_SOLDNER, 
    "Cassini/Soldner", 
    SRS_PP_LATITUDE_OF_ORIGIN, 
    SRS_PP_CENTRAL_MERIDIAN, 
    SRS_PP_FALSE_EASTING, 
    SRS_PP_FALSE_NORTHING,

    "*",
    SRS_PT_EQUIDISTANT_CONIC,
    "Equidistant Conic", 
    SRS_PP_STANDARD_PARALLEL_1,
    SRS_PP_STANDARD_PARALLEL_2,
    SRS_PP_LATITUDE_OF_CENTER, 
    SRS_PP_LONGITUDE_OF_CENTER,
    SRS_PP_FALSE_EASTING, 
    SRS_PP_FALSE_NORTHING,

    "*",
    SRS_PT_ECKERT_IV,
    "Eckert IV",
    SRS_PP_CENTRAL_MERIDIAN, 
    SRS_PP_FALSE_EASTING, 
    SRS_PP_FALSE_NORTHING,

    "*",
    SRS_PT_ECKERT_VI,
    "Eckert VI",
    SRS_PP_CENTRAL_MERIDIAN, 
    SRS_PP_FALSE_EASTING, 
    SRS_PP_FALSE_NORTHING,

    "*",
    SRS_PT_EQUIRECTANGULAR,
    "Equirectangular",
    SRS_PP_LATITUDE_OF_ORIGIN, 
    SRS_PP_CENTRAL_MERIDIAN, 
    SRS_PP_FALSE_EASTING, 
    SRS_PP_FALSE_NORTHING,

    "*",
    SRS_PT_GALL_STEREOGRAPHIC,
    "Gall Stereographic",
    SRS_PP_CENTRAL_MERIDIAN, 
    SRS_PP_FALSE_EASTING, 
    SRS_PP_FALSE_NORTHING,

    "*",
    SRS_PT_GNOMONIC,
    "Gnomonic",
    SRS_PP_LATITUDE_OF_ORIGIN,
    SRS_PP_CENTRAL_MERIDIAN, 
    SRS_PP_FALSE_EASTING, 
    SRS_PP_FALSE_NORTHING,

    "*",
    SRS_PT_HOTINE_OBLIQUE_MERCATOR,
    "Hotine Oblique Mercator",
    SRS_PP_LATITUDE_OF_CENTER, 
    SRS_PP_LONGITUDE_OF_CENTER,
    SRS_PP_AZIMUTH, 
    SRS_PP_RECTIFIED_GRID_ANGLE,
    SRS_PP_SCALE_FACTOR, 
    SRS_PP_FALSE_EASTING, 
    SRS_PP_FALSE_NORTHING,

    "*",
    SRS_PT_LAMBERT_AZIMUTHAL_EQUAL_AREA,
    "Lambert Azimuthal Equal Area",
    SRS_PP_LATITUDE_OF_CENTER, 
    SRS_PP_LONGITUDE_OF_CENTER,
    SRS_PP_FALSE_EASTING, 
    SRS_PP_FALSE_NORTHING,

    "*",
    SRS_PT_LAMBERT_CONFORMAL_CONIC_2SP,
    "Lambert Conformal Conic (2SP)",
    SRS_PP_STANDARD_PARALLEL_1, 
    SRS_PP_STANDARD_PARALLEL_2, 
    SRS_PP_LATITUDE_OF_ORIGIN, 
    SRS_PP_CENTRAL_MERIDIAN, 
    SRS_PP_FALSE_EASTING, 
    SRS_PP_FALSE_NORTHING,

    "*",
    SRS_PT_LAMBERT_CONFORMAL_CONIC_1SP,
    "Lambert Conformal Conic (1SP)",
    SRS_PP_LATITUDE_OF_ORIGIN,
    SRS_PP_CENTRAL_MERIDIAN, 
    SRS_PP_SCALE_FACTOR, 
    SRS_PP_FALSE_EASTING,
    SRS_PP_FALSE_NORTHING,

    "*",
    SRS_PT_LAMBERT_CONFORMAL_CONIC_2SP_BELGIUM,
    "Lambert Conformal Conic (2SP - Belgium)",
    SRS_PP_STANDARD_PARALLEL_1, 
    SRS_PP_STANDARD_PARALLEL_2, 
    SRS_PP_LATITUDE_OF_ORIGIN, 
    SRS_PP_CENTRAL_MERIDIAN, 
    SRS_PP_FALSE_EASTING, 
    SRS_PP_FALSE_NORTHING,

    "*",
    SRS_PT_MILLER_CYLINDRICAL,
    "Miller Cylindrical",
    SRS_PP_LATITUDE_OF_CENTER, 
    SRS_PP_LONGITUDE_OF_CENTER,
    SRS_PP_FALSE_EASTING, 
    SRS_PP_FALSE_NORTHING,

    "*",
    SRS_PT_MERCATOR_1SP,
    "Mercator (1SP)",
    SRS_PP_LATITUDE_OF_ORIGIN,
    SRS_PP_CENTRAL_MERIDIAN, 
    SRS_PP_SCALE_FACTOR, 
    SRS_PP_FALSE_EASTING,
    SRS_PP_FALSE_NORTHING,

    "*",
    SRS_PT_MOLLWEIDE,
    "Mollweide",
    SRS_PP_CENTRAL_MERIDIAN, 
    SRS_PP_FALSE_EASTING, 
    SRS_PP_FALSE_NORTHING,

    "*",
    SRS_PT_NEW_ZEALAND_MAP_GRID,
    "New Zealand Map Grid",
    SRS_PP_LATITUDE_OF_ORIGIN, 
    SRS_PP_CENTRAL_MERIDIAN, 
    SRS_PP_FALSE_EASTING, 
    SRS_PP_FALSE_NORTHING,

    "*",
    SRS_PT_OBLIQUE_STEREOGRAPHIC,
    "Oblique Stereographic",
    SRS_PP_LATITUDE_OF_ORIGIN, 
    SRS_PP_CENTRAL_MERIDIAN, 
    SRS_PP_SCALE_FACTOR, 
    SRS_PP_FALSE_EASTING,
    SRS_PP_FALSE_NORTHING,

    "*",
    SRS_PT_ORTHOGRAPHIC,
    "Orthographic",
    SRS_PP_LATITUDE_OF_ORIGIN, 
    SRS_PP_CENTRAL_MERIDIAN, 
    SRS_PP_FALSE_EASTING, 
    SRS_PP_FALSE_NORTHING,

    "*",
    SRS_PT_POLYCONIC,
    "Polyconic",
    SRS_PP_LATITUDE_OF_ORIGIN, 
    SRS_PP_CENTRAL_MERIDIAN, 
    SRS_PP_FALSE_EASTING, 
    SRS_PP_FALSE_NORTHING,
    
    "*",
    SRS_PT_POLAR_STEREOGRAPHIC,
    "Polar Stereographic",
    SRS_PP_LATITUDE_OF_ORIGIN,
    SRS_PP_CENTRAL_MERIDIAN, 
    SRS_PP_SCALE_FACTOR, 
    SRS_PP_FALSE_EASTING,
    SRS_PP_FALSE_NORTHING,

    "*",
    SRS_PT_ROBINSON,
    "Robinson",
    SRS_PP_LONGITUDE_OF_CENTER,
    SRS_PP_FALSE_EASTING, 
    SRS_PP_FALSE_NORTHING,

    "*",
    SRS_PT_SINUSOIDAL,
    "Sinusoidal",
    SRS_PP_LONGITUDE_OF_CENTER,
    SRS_PP_FALSE_EASTING, 
    SRS_PP_FALSE_NORTHING,

    "*"
    SRS_PT_STEREOGRAPHIC,
    "Stereographic",
    SRS_PP_LATITUDE_OF_ORIGIN,
    SRS_PP_CENTRAL_MERIDIAN, 
    SRS_PP_SCALE_FACTOR, 
    SRS_PP_FALSE_EASTING, 
    SRS_PP_FALSE_NORTHING,

    "*",
    SRS_PT_VANDERGRINTEN,
    "Van Der Grinten",
    SRS_PP_CENTRAL_MERIDIAN,
    SRS_PP_FALSE_EASTING, 
    SRS_PP_FALSE_NORTHING,

    NULL
};




/************************************************************************/
/*                      OPTGetProjectionMethods()                       */
/************************************************************************/

/** 
 * Fetch list of possible projection methods.
 *
 * @return Returns NULL terminated list of projection methods.  This should
 * be freed with CSLDestroy() when no longer needed.
 */

char **OPTGetProjectionMethods()

{
    int         i;
    char        **papszList = NULL;

    for( i = 1; papszProjectionDefinitions[i] != NULL; i++ )
    {
        if( EQUAL(papszProjectionDefinitions[i-1],"*") )
            papszList = CSLAddString(papszList,papszProjectionDefinitions[i]);
    }

    return papszList;
}

/************************************************************************/
/*                        OPTGetParameterList()                         */
/************************************************************************/

/**
 * Fetch the parameters for a given projection method. 
 *
 * @param pszProjectionMethod internal name of projection methods to fetch
 * the parameters for, such as "Transverse_Mercator" 
 * (SRS_PT_TRANSVERSE_MERCATOR).
 *
 * @param ppszUserName pointer in which to return a user visible name for
 * the projection name.  The returned string should not be modified or
 * freed by the caller.  Legal to pass in NULL if user name not required.
 *
 * @return returns a NULL terminated list of internal parameter names that
 * should be freed by the caller when no longer needed.  Returns NULL if 
 * projection method is unknown.
 */

char **OPTGetParameterList( const char *pszProjectionMethod, 
                            char ** ppszUserName )

{
    char **papszList = NULL;
    int  i;

    for( i = 1; papszProjectionDefinitions[i] != NULL; i++ )
    {
        if( papszProjectionDefinitions[i-1][0] == '*' 
            && EQUAL(papszProjectionDefinitions[i],pszProjectionMethod) )
        {
            i++;

            if( ppszUserName != NULL )
                *ppszUserName = papszProjectionDefinitions[i];

            i++;
            while( papszProjectionDefinitions[i] != NULL 
                   && papszProjectionDefinitions[i][0] != '*' )
            {
                papszList = CSLAddString( papszList, 
                                          papszProjectionDefinitions[i] );
                i++;
            }
            return papszList;
        }
    }
    
    return NULL;
}

/************************************************************************/
/*                        OPTGetParameterInfo()                         */
/************************************************************************/

/**
 * Fetch information about a single parameter of a projection method. 
 *
 * @param pszProjectionMethod name of projection method for which the parameter
 * applies.  Not currently used, but in the future this could affect defaults.
 * This is the internal projection method name, such as "Tranverse_Mercator".
 *
 * @param pszParameterName name of the parameter to fetch information about.
 * This is the internal name such as "central_meridian" 
 * (SRS_PP_CENTRAL_MERIDIAN). 
 * 
 * @param ppszUserName location at which to return the user visible name for
 * the parameter.  This pointer may be NULL to skip the user name.  The 
 * returned name should not be modified or freed.
 *
 * @param ppszType location at which to return the parameter type for
 * the parameter.  This pointer may be NULL to skip.  The  returned type 
 * should not be modified or freed.  The type values are described above.
 *
 * @param pdfDefaultValue location at which to put the default value for
 * this parameter.  The pointer may be NULL.
 *
 * @return TRUE if parameter found, or FALSE otherwise.
 */

int OPTGetParameterInfo( const char * pszProjectionMethod,
                         const char * pszParameterName,
                         char ** ppszUserName,
                         char ** ppszType,
                         double *pdfDefaultValue )

{
    int         i;

    (void) pszProjectionMethod;

    for( i = 0; papszParameterDefinitions[i] != NULL; i += 4 )
    {
        if( EQUAL(papszParameterDefinitions[i],pszParameterName) )
        {
            if( ppszUserName != NULL )
                *ppszUserName = papszParameterDefinitions[i+1];
            if( ppszType != NULL )
                *ppszType = papszParameterDefinitions[i+2];
            if( pdfDefaultValue != NULL )
                *pdfDefaultValue = atof(papszParameterDefinitions[i+3]);

            return TRUE;
        }
    }

    return FALSE;
}

