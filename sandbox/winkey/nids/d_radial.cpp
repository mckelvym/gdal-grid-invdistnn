/******************************************************************************
 *
 * Project:  NIDS (level 3 nexrad radar) translator
 * Purpose:  Implements nidsDriver
 * Author:   Brian Case, rush at winkey dot org
 *
 ******************************************************************************
 * Copyright (c) 2012, Brian Case
 *
 * Permission is hereby granted, CPLFree of charge, to any person obtaining a
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
#include <time.h>
#include <math.h>

#include "NIDS.h"
#include "get.h"
#include "d_radial.h"
#include "error.h"

/*******************************************************************************
	function to parse a single radial
	
args:
							buf			the buffer pointing to the first byte of the radial
							r				the struct to store the radial in;

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_d_radial(char *buf, NIDS_d_radial *r) {
	char *p;
	int i;
	
	r->num_bytes = GET2(buf);
	r->num_bins = r->num_bytes - 4;
	r->start = GET2(buf + 2);
	r->start /= 10;
	r->delta = GET2(buf + 4);
	r->delta /= 10;
	
	if (!(r->level = (char*) CPLMalloc(r->num_bytes)))
		ERROR("parse_d_radial");
	
	p = buf + 6;
	
	for (i = 0 ; i < r->num_bins - 4; i++)
		r->level[i] = p[i];
	
	if ((i % 2))
		i++;
	
	return p + i;
}	


/*******************************************************************************
3.1. Digital Radial Data Array

Radial data contains values for each bin within a particular radial.  Each
radial is defined by a start and end angle and by distance.  There are multiple
radials that define a full scan.

01 	Packet Code 16	Hex "0x10"
02 	Index of First Range Bin 	 
03 	Number of Range Bins 	Number of bins in each radial
04 	I Center of Sweep 	Center point location in a -2048 to 2047 coordinate region(mostly 0,0)
05 	J Center of Sweep 	 
06 	Scale Factor 	Number of pixels per range bin
07 	Number of Radials 	 
*******************************************************************************/

/*******************************************************************************
	function to parse a Digital Radial Data Array packet
	
args:					buf			the buffer pointing to the start of the radial packet
							r				the structure to store the radials in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_d_radial_header(char *buf, NIDS_d_radials *r) {
	int i;
	char *p;
	
	r->index_first_bin = GET2(buf);
	r->num_bins = GET2(buf + 2);
	r->x_center = GET2(buf + 4);
	r->y_center = GET2(buf + 6);
	r->scale = GET2(buf + 8);
	r->num_radials = GET2(buf + 10);
	
	if (!(r->radials = (NIDS_d_radial*) CPLMalloc(r->num_radials * sizeof(NIDS_d_radial))))
		ERROR("parse_d_radial_header");
	
	p = buf + 12;
	
	for (i = 0 ; i < r->num_radials ; i++) {
		p = parse_d_radial(p, r->radials + i);
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

void free_d_radial(NIDS_d_radial *r) {
	
	CPLFree(r->level);
	
}

/*******************************************************************************
	function to CPLFree any dynamicly alocated memory used in Digital Radial Data
	Array storage

args:
						r				the structure the radials is stored in

returns:
						nothing
*******************************************************************************/

void free_d_radial_header(NIDS_d_radials *r) {
	int i;
	
	for (i = 0 ; i < r->num_radials ; i++)
		free_d_radial(r->radials + i);
	
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

void print_d_radial(NIDS_d_radial *r, char *prefix, int rn) {
	
	printf("%s.d_radial.radials[%i].num_bytes %i\n", prefix, rn, r->num_bytes);
	printf("%s.d_radial.radials[%i].start %f\n", prefix, rn, r->start);
	printf("%s.d_radial.radials[%i].delta %f\n", prefix, rn, r->delta);
	
}

/*******************************************************************************
	function to print a Digital Radial Data Array packet

args:
						r				the structure the radials are stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_d_radial_header(NIDS_d_radials *r, char *prefix) {
	int i;
	printf("%s.d_radial.index_first_bin %i\n", prefix, r->index_first_bin);
	printf("%s.d_radial.num_bins %i\n", prefix, r->num_bins);
	printf("%s.d_radial.x_center %i\n", prefix, r->x_center);
	printf("%s.d_radial.y_center %i\n", prefix, r->y_center);
	printf("%s.d_radial.num_radials %i\n", prefix, r->num_radials);
	
	for (i = 0 ; i < r->num_radials ; i++)
		print_d_radial(r->radials + i, prefix, i);
}



