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
#include "NIDS.h"
#include "circle.h"
#include "error.h"

#define RASTER_X_SIZE 4096
#define RASTER_Y_SIZE 4096

/*******************************************************************************
	function to parse a single circle
	
args:
							buf			the buffer pointing to the first byte of the circle
							c				the struct to store the circle in;

returns:
							pointer to the next byte in the buffer 
*******************************************************************************/

char *parse_circle (char *buf, NIDS_circle *c) {
	c->x_start = GET2(buf);
	c->y_start = GET2(buf + 2);
	c->radius = GET2(buf + 4);
	
	return buf + 6;
}

/*******************************************************************************
	function to parse a circle packet
	
args:					buf			the buffer pointing to the start of the circle packet
							c				the structure to store the circles in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_circle_header(char *buf, NIDS_circles *c) {
	int i;
	char *p;
	
	c->length = GET2(buf);
	c->num_circles = c->length / 6;
	if (!(c->circles = (NIDS_circle*)CPLMalloc(c->num_circles * sizeof(NIDS_circle))))
		ERROR("parse_circle_header");
	
	p = buf + 2;
	
	for (i = 0 ; i < c->num_circles ; i++) {
		p = parse_circle(p, c->circles + i);
	}
	
	return p;
}

/*******************************************************************************
	function to CPLFree any dynamicly alocated memory used in circle storage

args:
						c			the structure the circles are stored in

returns:
						nothing
*******************************************************************************/

void free_circle_header(NIDS_circles *c) {

	CPLFree(c->circles);
	
}

/*******************************************************************************
	function to print a single circle

args:
						c				the structure the circle is stored in
						prefix	the start of the line
						rn			the arrow number

returns:
						nothing
*******************************************************************************/

void print_circle(NIDS_circle *c, char *prefix, int rn) {
	
	printf("%s.circle.circles[%i].x_start %i\n", prefix, rn, c->x_start);
	printf("%s.circle.circles[%i].y_start %i\n", prefix, rn, c->y_start);
	printf("%s.circle.circles[%i].radius %i\n", prefix, rn, c->radius);
	
}
	
/*******************************************************************************
	function to print a circle packet

args:
						c				the structure the circles are stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_circle_header(NIDS_circles *c, char *prefix) {
	int i;
	
	printf("%s.circle.length %i\n", prefix, c->length);
	printf("%s.circle.num_circles %i\n", prefix, c->num_circles);
	
	for (i = 0 ; i < c->num_circles ; i++)
		print_circle(c->circles + i, prefix, i);
	
}

/*******************************************************************************
	fuction to draw a circle in an image

args:
						raster	pointer to the raster
						v				the structure that holds the circles
						xcenter	the x axis center in the raster
						ycenter	the y axis center in the raster

returns:
						nothing
*******************************************************************************/

void circles_to_raster (
	NIDS_image *im,
	NIDS_circles *c)
{
	int i;
	
	for (i = 0 ; i < c->num_circles ; i++) {
		draw_circle(im,
								c->circles[i].x_start,
								c->circles[i].y_start,
								c->circles[i].radius,
								1);
	}
	
}


