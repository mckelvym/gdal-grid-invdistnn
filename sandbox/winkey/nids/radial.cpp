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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <time.h>

#include "NIDS.h"
#include "get.h"

#include "error.h"
#include "image.h"

#include "radial.h"

/*******************************************************************************
Individual Radials

Each radial contains Run Length Encoded (RLE) values.
Halfword	Name											Description
01				Number of RLE Halfwords 	RLE data always padded to even halfword
																		boundary
02				Radial Start Angle				Angle*10, scan direction is always clockwise
03				Radial Angle Delta				Angle*10
04				Run0 	Code0 	Run1 	Code1	Run is 4 bit value for number of bins for
																		this value
05				Run2 	Code2 	Run3 	Code3	Code is 4 bit value (0-15) for the value
																		within the run
06-##			Run Codes									Repeated for entire radial
##				RunN 	CodeN								00 	00

*******************************************************************************/

/*******************************************************************************
	function to parse a single radial
	
args:
							buf			the buffer pointing to the first byte of the radial
							r				the struct to store the radial in;

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_radial(char *buf, NIDS_radial *r) {
	char *p;
	int i;
	
	r->num_rle = GET2(buf);
	
	/***** detect bad data *****/
	
	if (r->num_rle == 0)
			ERROR("parse_radial: can't have 0 rle's");
	
	r->start = GET2(buf + 2);
	r->start /= 10;
	r->delta = GET2(buf + 4);
	r->delta /= 10;
	r->num_rle *= 2;
	if (!(r->run = (char*)CPLMalloc(r->num_rle)))
		ERROR("parse_radial");
	
	if (!(r->code = (char*)CPLMalloc(r->num_rle)))
		ERROR("parse_radial");
	
	p = buf + 6;
	
	for (i = 0 ; i < r->num_rle ; i++) {
		r->run[i] = (p[i] >> 4) & 0x0f;
		r->code[i] = p[i] & 0x0f;
	}
	
	return p + i;
}	


/*******************************************************************************
3.1. Radial Data Packet

Radial data contains values for each bin within a particular radial.  Each
radial is defined by a start and end angle and by distance.  There are multiple
radials that define a full scan.

01 	Packet Code 	Hex "AF1F"
02 	Index of First Range Bin 	 
03 	Number of Range Bins 	Number of bins in each radial
04 	I Center of Sweep 	Center point location in a -2048 to 2047 coordinate region(mostly 0,0)
05 	J Center of Sweep 	 
06 	Scale Factor 	Number of pixels per range bin
07 	Number of Radials 	 
*******************************************************************************/

/*******************************************************************************
	function to parse a radial packet
	
args:					buf			the buffer pointing to the start of the radial packet
							r				the structure to store the radials in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_radial_header(char *buf, NIDS_radials *r) {
	int i;
	char *p;
	
	r->index_first_bin = GET2(buf);
	r->num_bins = GET2(buf + 2);
	r->x_center = GET2(buf + 4);
	r->y_center = GET2(buf + 6);
	r->scale = GET2(buf + 8);
	r->num_radials = GET2(buf + 10);
	
	if (!(r->radials = (NIDS_radial*)CPLMalloc(r->num_radials * sizeof(NIDS_radial))))
		ERROR("parse_radial_header");
	
	p = buf + 12;
	
	for (i = 0 ; i < r->num_radials ; i++) {
		p = parse_radial(p, r->radials + i);
	}
	
	return p;
}

/*******************************************************************************
	function to CPLFree a radial

args:
						r			the radial to CPLFree

returns:
						nothing
*******************************************************************************/

void free_radial(NIDS_radial *r) {
	CPLFree(r->run);
	CPLFree(r->code);
}

/*******************************************************************************
	function to CPLFree any dynamicly alocated memory used in radial storage

args:
						r				the structure the radials is stored in

returns:
						nothing
*******************************************************************************/

void free_radial_header(NIDS_radials *r) {
	int i;
	
	for (i = 0 ; i < r->num_radials ; i++)
		free_radial(r->radials + i);
	
	CPLFree(r->radials);
	
}

/*******************************************************************************
	function to print a single radial

args:
						r				the structure the radial is stored in
						prefix	the start of the line

						rn			the radial number

returns:
						nothing
*******************************************************************************/

void print_radial(NIDS_radial *r, char *prefix, int rn) {
	
	printf("%s.rad.radials[%i].num_rle %i\n", prefix, rn, r->num_rle);
	printf("%s.rad.radials[%i].start %f\n", prefix, rn, r->start);
	printf("%s.rad.radials[%i].delta %f\n", prefix, rn, r->delta);
	
}

/*******************************************************************************
	function to print a radial packet

args:
						r				the structure the radials are stored in
						prefix	the start of the line


returns:
						nothing
*******************************************************************************/

void print_radial_header(NIDS_radials *r, char *prefix) {
	int i;
	printf("%s.rad.index_first_bin %i\n", prefix, r->index_first_bin);
	printf("%s.rad.num_bins %i\n", prefix, r->num_bins);
	printf("%s.rad.x_center %i\n", prefix, r->x_center);
	printf("%s.rad.y_center %i\n", prefix, r->y_center);
	printf("%s.rad.scale %i\n", prefix, r->scale);
	printf("%s.rad.num_radials %i\n", prefix, r->num_radials);
	
	for (i = 0 ; i < r->num_radials ; i++)
		print_radial(r->radials + i, prefix, i);
}

#define PI 3.14159265

void radial_convert(float angle, int bin, int *x, int *y) {
	float r_angle = angle * (PI / 180);
	
	*x = bin * cos(r_angle);
	*y = bin * sin(r_angle);

}

/*******************************************************************************
	function to convert a single radial to a raster
*******************************************************************************/

void radial_to_raster (NIDS_radial *r, NIDS_image *im) {
	int x, y;
	int i, j;
	float k, angle;
	int bin = 1;
	
	for (i = 0 ; i < r->num_rle ; i++) {
		for (j = 0 ; j < r->run[i] ; j++, bin++) {
			for (k = -(r->delta / 2) ; k <= r->delta / 2; k += 0.1) {
				
				if (r->start + k > 360)
					angle = k;
				else
					angle = r->start + k;
			
				radial_convert(angle, bin, &x, &y);
				
				plot(im, y, -x, r->code[i]);
			}
		}
	}
	
}

/*******************************************************************************
	function to convert radials to a raster

args:
						r				the structure that holds the radials
						width		pointer to return the width of the raster in
						height	pointer to return the height of the raster in

returns:
						a char pointer to the raster data

*******************************************************************************/

void radials_to_raster (
	NIDS_image *im,
	NIDS_radials *r)
{
	int i;

	//im->x_center = im->width / 2;
	//im->x_center = im->width / 2;
	//r->x_center,
	//r->y_center);
	
	for (i = 0 ; i < r->num_radials ; i++)
		radial_to_raster(r->radials + i, im);
	
}


/*******************************************************************************
	function to convert a single radial to 3d vectors

args:
						v				the structure that holds the 3d vectors
						r				the structure that holds the radial
						elev		the elevation angle of the beam

returns:
						nothing
*******************************************************************************/
/*
void radial_to_vectors3d (
	ThreeD_atmos_buf *buf,
	NIDS_radial *r,
	float elev,
	void *extra)
{
	double point[4];
	
	float angle, zangle;
	float r_angle, r_zangle;
	int i;
	int binstart = 0, binend = 0;
	angle = r->start;
	zangle = elev;
	
	for (i = 0 ; i < r->num_rle ; i++) {
		binstart = binend;
		binend = r->run[i];
		
		
		for (j = 0 ; j < r->run[i] ; j++, bin++) {
			
			r_angle = angle * (PI / 180);
			r_zangle = zangle * (PI / 180);
			
			point[0] = bin * sin(r_angle); //x
			point[1] = -(bin * cos(r_angle)); //y
			point[2] = (bin * sin(r_zangle)); //z
			point[3] = r->code[i];
			
			KDTree_insert(tree, point, NULL);
			
		}
	}
	
}
*/
/*******************************************************************************
	function to convert radials to 3d vectors

args:
						v				the structure that holds the 3d vectors
						r				the structure that holds the radials
						elev		the elevation angle of the beam

returns:
						nothing

*******************************************************************************/
/*
void radials_to_3datmos (
	ThreeD_atmos_buf *buf,
	NIDS_radials *r,
	float elev,
	void *extra)
{
	int i;

	for (i = 0 ; i < r->num_radials ; i++)
		radial_to_vectors3d(buf, r->radials + i, elev, extra);
	
}

*/
