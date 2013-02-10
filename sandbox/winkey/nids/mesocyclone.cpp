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
#include "NIDS.h"
#include "mesocyclone.h"
#include "error.h"

#define RASTER_X_SIZE 4096
#define RASTER_Y_SIZE 4096

/*******************************************************************************
	function to parse a single mesocyclone
	
args:
							buf			the buffer pointing to the first byte of the mesocyclone
							c				the struct to store the mesocyclone in;

returns:
							pointer to the next byte in the buffer 
*******************************************************************************/

char *parse_mesocyclone (char *buf, NIDS_mesocyclone *c) {
	c->x_start = GET2(buf);
	c->y_start = GET2(buf + 2);
	c->radius = GET2(buf + 4);
	
	return buf + 6;
}

/*******************************************************************************
	function to parse a mesocyclone packet
	
args:					buf			the buffer pointing to the start of the mesocyclone packet
							c				the structure to store the mesocyclones in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_mesocyclone_header(char *buf, NIDS_mesocyclones *c) {
	int i;
	char *p;
	
	c->length = GET2(buf);
	c->num_mesocyclones = c->length / 6;
	if (!(c->mesocyclones = (NIDS_mesocyclone*)CPLMalloc(c->num_mesocyclones * sizeof(NIDS_mesocyclone))))
		ERROR("parse_mesocyclone_header");
	
	p = buf + 2;
	
	for (i = 0 ; i < c->num_mesocyclones ; i++) {
		p = parse_mesocyclone(p, c->mesocyclones + i);
	}
	
	return p;
}

/*******************************************************************************
	function to CPLFree any dynamicly alocated memory used in mesocyclone storage

args:
						c			the structure the mesocyclones are stored in

returns:
						nothing
*******************************************************************************/

void free_mesocyclone_header(NIDS_mesocyclones *c) {

	CPLFree(c->mesocyclones);
	
}

/*******************************************************************************
	function to print a single mesocyclone

args:
						c				the structure the mesocyclone is stored in
						prefix	the start of the line
						rn			the arrow number

returns:
						nothing
*******************************************************************************/

void print_mesocyclone(NIDS_mesocyclone *c, char *prefix, int rn) {
	
	printf("%s.mesocyclone.mesocyclones[%i].x_start %i\n", prefix, rn, c->x_start);
	printf("%s.mesocyclone.mesocyclones[%i].y_start %i\n", prefix, rn, c->y_start);
	printf("%s.mesocyclone.mesocyclones[%i].radius %i\n", prefix, rn, c->radius);
	
}
	
/*******************************************************************************
	function to print a mesocyclone packet

args:
						c				the structure the mesocyclones are stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_mesocyclone_header(NIDS_mesocyclones *c, char *prefix) {
	int i;
	
	printf("%s.mesocyclone.length %i\n", prefix, c->length);
	printf("%s.mesocyclone.num_mesocyclones %i\n", prefix, c->num_mesocyclones);
	
	for (i = 0 ; i < c->num_mesocyclones ; i++)
		print_mesocyclone(c->mesocyclones + i, prefix, i);
	
}

/*******************************************************************************
	fuction to draw a mesocyclone in an image

args:
						raster	pointer to the raster
						v				the structure that holds the mesocyclones
						xcenter	the x axis center in the raster
						ycenter	the y axis center in the raster

returns:
						nothing
*******************************************************************************/

void mesocyclones_to_raster (
	NIDS_image *im,
	NIDS_mesocyclones *c)
{
	int i, j;
	
	for (i = 0 ; i < c->num_mesocyclones ; i++) {
		for (j = c->mesocyclones[i].radius ; j > c->mesocyclones[i].radius - 4 ; j--) {
			draw_circle(im,
									c->mesocyclones[i].x_start,
									c->mesocyclones[i].y_start,
									c->mesocyclones[i].radius,
									4);
		}
	}
	
}


