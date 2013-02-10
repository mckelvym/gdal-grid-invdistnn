/******************************************************************************
 *
 * Project:  NIDS (level 3 nexrad radar) translator
 * Purpose:  Implements nidsDriver
 * Author:   Brian Case, rush at winkey dot org
 *
 ******************************************************************************
 * Copyright (c) 2012, Brian Case
 *
 * Permission is hereby granted, Free of charge, to any person obtaining a
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

#include <stdio.h>
#include <time.h>

#include "NIDS.h"
#include "color.h"
#include "error.h"

/*******************************************************************************
	function to get the scale

	args:
							name				the name of the scale

	returns:
							the scale
	
	NOTE: you must CPLFree() the result when your done with it
*******************************************************************************/

NIDS_color _2bit_severeweather[] = {
    {-32768, "000000" },
    {20, "00E0FF" },
    {35, "FFFF00" },
    {50, "FF0000" },
};

NIDS_color _3bit_reflectivity[] = {
    {-32770, "000000" },
    {5, "FFAAAA" },
    {18, "C97070" },
    {30, "00BB00" },
    {41, "FFFF70" },
    {46, "DA0000" },
    {50, "0000FF" },
    {57, "FFFFFF" },
    {32770, "000000" },
};

NIDS_color _3bit_reflectivity_clear[] = {
    {-32770, "000000" },
    {0, "000000" },
    {5, "FFAAAA" },
    {18, "C97070" },
    {30, "00BB00" },
    {41, "FFFF70" },
    {46, "DA0000" },
    {50, "0000FF" },
    {57, "FFFFFF" },
    {32770, "000000" },
};

NIDS_color _3bit_vad[] = {
    {-32770, "000000" },
    {-32769, "777790" },
    {5, "FFAAAA" },
    {18, "C97070" },
    {30, "00BB00" },
    {41, "FFFF70" },
    {46, "DA0000" },
    {50, "0000FF" },
    {32770, "000000" },
};

NIDS_color _3bit_velocity[] = {
    {-32770, "000000" },
    {-10, "00E0FF" },
    {-5, "00BB00" },
    {-1, "008F00" },
    {0, "F88700" },
    {5, "FFCF00" },
    {10, "FF0000" },
    {32769, "77007D" },
    {32770, "000000" },
};

NIDS_color _3bit_vwp[] = {
    {-32770, "000000" },
    {0, "00FF00" },
    {4, "FFFF00" },
    {8, "FF0000" },
    {12, "00E0FF" },
    {16, "FF70FF" },
    {32770, "000000" },
};

NIDS_color _4bit_echotops[] = {
    {-32770, "000000" },
    {0, "000000" },
    {5, "767676" },
    {10, "00E0FF" },
    {15, "00B0FF" },
    {20, "0090CC" },
    {25, "320096" },
    {30, "00FB90" },
    {35, "00BB00" },
    {40, "00EF00" },
    {45, "FEBF00" },
    {50, "FFFF00" },
    {55, "AE0000" },
    {60, "FF0000" },
    {65, "FFFFFF" },
    {70, "E700FF" },
    {32770, "000000" },
};


NIDS_color _4bit_reflectivity[] = {
    {-32770, "000000" },
    {5, "9C9C9C" },
    {10, "767676" },
    {15, "FFAAAA" },
    {20, "EE8C8C" },
    {25, "C97070" },
    {30, "00FB90" },
    {35, "00BB00" },
    {40, "FFFF70" },
    {45, "D0D060" },
    {50, "FF6060" },
    {55, "DA0000" },
    {60, "AE0000" },
    {65, "0000FF" },
    {70, "FFFFFF" },
    {75, "E700FF" },
    {32770, "000000" },
};

NIDS_color _4bit_reflectivity_clear[] = {
    {-32770, "000000" },
    {-28, "9C9C9C" },
    {-24, "767676" },
    {-20, "FFAAAA" },
    {-16, "EE8C8C" },
    {-12, "C97070" },
    {-8, "00FB90" },
    {-4, "00BB00" },
    {0, "FFFF70" },
    {4, "D0D060" },
    {8, "FF6060" },
    {12, "DA0000" },
    {16, "AE0000" },
    {20, "0000FF" },
    {24, "FFFFFF" },
    {28, "E700FF" },
    {32770, "000000" },
};

NIDS_color _4bit_shear[] = {
    {-32770, "000000" },
    {0, "767676" },
    {5, "AAAAAA" },
    {10, "320096" },
    {20, "0080FF" },
    {30, "00FFFF" },
    {40, "008F00" },
    {50, "00BB00" },
    {60, "00FF00" },
    {70, "C86400" },
    {80, "FFAA00" },
    {90, "FFFF00" },
    {100, "AE0000" },
    {150, "FF0000" },
    {200, "FF7D7D" },
    {32770, "000000" },
};

NIDS_color _4bit_velocity[] = {
    {32770, "000000" },
    {64, "00E0FF" },
    {50, "0080FF" },
    {36, "320096" },
    {26, "00FB90" },
    {20, "00C000" },
    {10, "008F00" },
    {-1, "CDC09F" },
    {0, "767676" },
    {10, "F88700" },
    {20, "FFCF00" },
    {26, "FFFF00" },
    {36, "AE0000" },
    {50, "D07000" },
    {64, "FF0000" },
    {32769, "77007D" },
    {32770, "000000" },
};

NIDS_color n0r[] = {
    {-32770, "000000" },
    {-30, "ccfefc" },
    {-25, "cc9acc" },
    {-20, "9c669c" },
    {-15, "643264" },
    {-10, "ccce9c" },
    {-5, "9c9a64" },
    {0, "646664" },
    {5, "04eae4" },
    {10, "049ef4" },
    {15, "0402f4" },
    {20, "04fe04" },
    {25, "04c604" },
    {30, "048e04" },
    {35, "fcfa04" },
    {40, "e4be04" },
    {45, "fc9604" },
    {50, "fc0204" },
    {55, "d40204" },
    {60, "bc0204" },
    {65, "fc02fc" },
    {70, "9c56c4" },
    {75, "fcfefc" },
    {32770, "000000" },
};



NIDS_color *color_getscale(
	char *name)
{
	char filename[1024];
	FILE *fp;
	char line[1024];
	
	size_t used = 0, alloced = 0;
	
	NIDS_color *scales = NULL;
	NIDS_color *temp = NULL;
	
    if (EQUAL ("2bit_severeweather", name))
        scales = _2bit_severeweather;
    else if (EQUAL ("3bit_reflectivity", name))
        scales = _3bit_reflectivity;
    else if (EQUAL ("3bit_reflectivity_clear", name))
        scales = _3bit_reflectivity_clear;
    else if (EQUAL ("3bit_vad", name))
        scales = _3bit_vad;
    else if (EQUAL ("3bit_velocity", name))
        scales = _3bit_velocity;
    else if (EQUAL ("3bit_vwp", name))
        scales = _3bit_vwp;
    else if (EQUAL ("4bit_echotops", name))
        scales = _4bit_echotops;
    //else if (EQUAL ("4bit_precip", name))
    //    scales = _4bit_precip;
    else if (EQUAL ("4bit_reflectivity", name))
        scales = _4bit_reflectivity;
    else if (EQUAL ("4bit_reflectivity_clear", name))
        scales = _4bit_reflectivity_clear;
    else if (EQUAL ("4bit_shear", name))
        scales = _4bit_shear;
    else if (EQUAL ("4bit_velocity", name))
        scales = _4bit_velocity;
    else if (EQUAL ("n0r", name))
        scales = n0r;
    else
        scales = NULL;

	
	return scales;
}


void get_product_dependent_color(int msgcode, NIDS_color **result) {
	
	switch (msgcode) {
		case 16:
		case 17:
		case 18:
		case 35:
		case 36:
		case 63:
		case 64:
		case 65:
		case 66:
		case 67:
		case 85:
		case 89:
		case 90:
		case 95:
		case 96:
			*result = color_getscale("3bit_reflectivity");
			break;
		
		case 19:
		case 20:
		case 21:
		case 33:
		case 37:
		case 38:
		case 43:
		case 50:
		case 57:
		case 97:
		case 98:
		case 137:
			*result = color_getscale("4bit_reflectivity");
			break;
		
		case 22:
		case 23:
		case 24:
		case 86:
			*result = color_getscale("3bit_velocity");
			break;
		
		case 25:
		case 26:
		case 27:
		case 44:
		case 51:
		case 55:
		case 56:
			*result = color_getscale("4bit_velocity");
			break;

		case 28:
		case 29:
		case 30:
		case 45:
			*result = color_getscale("3bit_spectrum");
			break;
		
		case 41:
			*result = color_getscale("4bit_echotops");
			break;

		case 46:
		case 87:
			*result = color_getscale("4bit_shear");
			break;
		
		case 31:
		case 78:
		case 79:
		case 80:
		case 144:
		case 145:
		case 146:
		case 147:
		case 150:
		case 151:
			*result = color_getscale("4bit_precip");
			break;

		case 47:
			*result = color_getscale("2bit_severeweather");
			break;
		
		case 48:
			*result = color_getscale("3bit_vad");
			break;

		case 59:
		case 60:
		case 141:
			*result = color_getscale("3bit_reflectivity");
			break;
		
/*
47 11           Severe Weather Probability                       2.2 x 2.2 Nmi x Nmi                                    124                  N/A           Geographic
                                                                                                                                                           Alphanumeric
48 12           VAD Wind Profile                                 5 Knots                                                N/A                  5             Non-geographic

74 26           Radar Coded Message                              1/16 LFM                                        248                        9           Alphanumeric

84 12           Velocity Azimuth Display                         5 Knots                                       N/A                   8                  Non-geographic
                                                                                                                                                        Alphanumeric

132     36 Clutter Likelihood Reflectivity .54 x 1 Nmi. x Deg                124        11  Radial Image
133     37 Clutter Likelihood Doppler      .54 x 1 Nmi. x Deg                124        12  Radial Image
134     39 High Resolution VIL             .54 x 1 Nmi x Deg                 248        256 Radial Image
136     38 SuperOb                         Adaptable, default = 5 km x 6 deg Adaptable, N/A Latitude, Longitude
                                                                             default =      (ICD packet code 27)
                                                                             100 km
149       20        Digital Mesocyclone Detection      N/A                                      124             N/A   Generic Data Format
*/
	}

	return;
}
	
/*******************************************************************************
	function to get a color from a scale

	args:
							scales			the scale to check
							value				the value to check for in the scale

	returns:
							the color for the value
*******************************************************************************/
 
char *color_checkscale(
	NIDS_color *scales,
	float value)
{
	NIDS_color *scale;
	
	for (scale = scales ;
			 value > scale->value && *((scale + 1)->color) != 0;
			 scale++);
	
	return scale->color;
}


