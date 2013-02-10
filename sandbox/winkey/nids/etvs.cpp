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

#include "NIDS.h"
#include "get.h"
#include "image.h"
#include "point.h"
#include "etvs.h"
#include "error.h"


/*******************************************************************************
	function to parse a etvs packet
	
args:					buf			the buffer pointing to the start of the etvs packet
							p				the structure to store the etvss in

returns:
							etvser to the next byte in the buffer
*******************************************************************************/

char *parse_etvs_header(char *buf, NIDS_etvss *p) {
	int i;
	char *ptr;
	
	p->length = GET2(buf);
	p->num_points = p->length / 4;
	
	if (!(p->points = (NIDS_point*)CPLMalloc(p->num_points * sizeof(NIDS_point))))
		ERROR("parse_etvs_header");
	
	ptr = buf + 2;
	
	for (i = 0 ; i < p->num_points ; i++) {
		ptr = parse_point(ptr, p->points + i);
	}
	
	return ptr;
}

/*******************************************************************************
	function to CPLFree any dynamicly alocated memory used in etvs storage

args:
						p				the structure the etvss are stored in

returns:
						nothing
*******************************************************************************/

void free_etvs_header(NIDS_etvss *p) {

	CPLFree(p->points);
	
}

/*******************************************************************************
	function to print a etvs packet

args:
						p				the structure the etvss are stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_etvs_header(NIDS_etvss *p, char *prefix) {
	int i;
	char myprefix[PREFIX_LEN];
	
	printf("%s.etvs.length %i\n", prefix, p->length);
	printf("%s.etvs.num_points %i\n", prefix, p->num_points);
	
	for (i = 0 ; i < p->num_points ; i++) {
		snprintf(myprefix, PREFIX_LEN, "%s.etvs.points[%i]", prefix, i);
		print_point(p->points + i, prefix);
	}
	
}

/*******************************************************************************
	fuction to draw a etvs in an image

args:
						im			pointer to the raster struct
						p				the structure that holds the etvs

returns:
						nothing
*******************************************************************************/

void etvs_to_raster (
	NIDS_image *im,
	NIDS_point *p)
{
	
	draw_invert_triangle(im, p->x_start, p->y_start, 10, 14, 5);
	
}

/*******************************************************************************
	fuction to draw etvss in an image

args:
						im			pointer to the raster struct
						p				the structure that holds the etvss

returns:
						nothing
*******************************************************************************/

void etvss_to_raster (
	NIDS_image *im,
	NIDS_etvss *p)
{
	int i;
	
	for (i = 0; i < p->num_points ; i++)
		etvs_to_raster(im, p->points + i);
	
}
