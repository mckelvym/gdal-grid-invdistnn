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
#include <time.h>

#include "NIDS.h"
#include "get.h"
#include "image.h"
#include "point.h"
#include "tvs.h"
#include "error.h"


/*******************************************************************************
	function to parse a tvs packet
	
args:					buf			the buffer pointing to the start of the tvs packet
							p				the structure to store the tvss in

returns:
							tvser to the next byte in the buffer
*******************************************************************************/

char *parse_tvs_header(char *buf, NIDS_tvss *p) {
	int i;
	char *ptr;
	
	p->length = GET2(buf);
	p->num_points = p->length / 4;
	
	if (!(p->points = (NIDS_point*)CPLMalloc(p->num_points * sizeof(NIDS_point))))
		ERROR("parse_tvs_header");
	
	ptr = buf + 2;
	
	for (i = 0 ; i < p->num_points ; i++) {
		ptr = parse_point(ptr, p->points + i);
	}
	
	return ptr;
}

/*******************************************************************************
	function to CPLFree any dynamicly alocated memory used in tvs storage

args:
						p				the structure the tvss are stored in

returns:
						nothing
*******************************************************************************/

void free_tvs_header(NIDS_tvss *p) {

	CPLFree(p->points);
	
}

/*******************************************************************************
	function to print a tvs packet

args:
						p				the structure the tvss are stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_tvs_header(NIDS_tvss *p, char *prefix) {
	int i;
	char myprefix[PREFIX_LEN];
	
	printf("%s.tvs.length %i\n", prefix, p->length);
	printf("%s.tvs.num_points %i\n", prefix, p->num_points);
	
	for (i = 0 ; i < p->num_points ; i++) {
		snprintf(myprefix, PREFIX_LEN, "%s.tvs.points[%i]", prefix, i);
		print_point(p->points + i, prefix);
	}
	
}

/*******************************************************************************
	fuction to draw a tvs in an image

args:
						im			pointer to the raster struct
						p				the structure that holds the tvs

returns:
						nothing
*******************************************************************************/

void tvs_to_raster (
	NIDS_image *im,
	NIDS_point *p)
{
	int i, j;
	
	for (i = 10, j = 14 ; i > 0; i--, j--) {
		draw_invert_triangle(im, p->x_start, p->y_start, i, j, 5);
	}
	
}

/*******************************************************************************
	fuction to draw tvss in an image

args:
						im			pointer to the raster struct
						p				the structure that holds the tvss

returns:
						nothing
*******************************************************************************/

void tvss_to_raster (
	NIDS_image *im,
	NIDS_tvss *p)
{
	int i;
	
	for (i = 0; i < p->num_points ; i++)
		tvs_to_raster(im, p->points + i);
	
}
