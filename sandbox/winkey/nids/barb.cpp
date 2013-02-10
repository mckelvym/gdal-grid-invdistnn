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

#include "get.h"
#include "image.h"
#include "NIDS.h"
#include "barb.h"
#include "error.h"

/*******************************************************************************
Individual Barbs

Each barb is 5 halfwords and is repeated until byte length.
01 	Value 	Color value (0-5) representing RMS variation of wind speed
02 	I Coordinate 	 
03 	J Coordinate 	 
04 	Direction of Wind 	In degrees
05 	Wind Speed 	
*******************************************************************************/

/*******************************************************************************
	function to parse a single barb
	
args:
							buf			the buffer pointing to the first byte of the barb
							b				the struct to store the barb in;

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_barb (char *buf, NIDS_barb *b) {

	b->value = GET2(buf);
	b->x_start = GET2(buf + 2);
	b->y_start = GET2(buf + 4);
	b->heading = GET2(buf + 6);
	b->speed = GET2(buf + 8);
	
	return buf + 10;
}

/*******************************************************************************
3.4. Wind Barb Packet

These define the location and size of wind barbs to be plotted.
01 	Packet Code = 4 	Hex "4"
02 	Length of Data 	In bytes
*******************************************************************************/

/*******************************************************************************
	function to parse a barb packet
	
args:					buf			the buffer pointing to the start of the barb packet
							b				the structure to store the barbs in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_barb_header(char *buf, NIDS_barbs *b) {
	int i;
	char *p;
	
	b->length = GET2(buf);
	b->num_barbs = b->length / 10;
	if (!(b->barbs = (NIDS_barb*)CPLMalloc(b->num_barbs * sizeof(NIDS_barb))))
		ERROR("parse_barb_header");
	
	p = buf + 2;
	
	for (i = 0 ; i < b->num_barbs ; i++) {
		p = parse_barb(p, b->barbs + i);
	}
	
	return p;
}

/*******************************************************************************
	function to CPLFree any dynamicly alocated memory used in barb storage

args:
						b				the structure the barbs are stored in

returns:
						nothing
*******************************************************************************/

void free_barb_header(NIDS_barbs *b) {
	
	CPLFree(b->barbs);
	
}

/*******************************************************************************
	function to print a single barb

args:
						b				the structure the barb is stored in
						prefix	the start of the line
						rn			the barb number

returns:
						nothing
*******************************************************************************/

void print_barb(NIDS_barb *b, char *prefix, int rn) {
	
	printf("%s.barb.barbs[%i].value %i\n", prefix, rn, b->value);
	printf("%s.barb.barbs[%i].x_start %i\n", prefix, rn, b->x_start);
	printf("%s.barb.barbs[%i].y_start %i\n", prefix, rn, b->y_start);
	printf("%s.barb.barbs[%i].heading %i\n", prefix, rn, b->heading);
	printf("%s.barb.barbs[%i].speed %i\n", prefix, rn, b->speed);
	
}

/*******************************************************************************
	function to print a barb packet

args:
						b				the structure the barbs are stored in
						prefix	the start of the line
returns:
						nothing
*******************************************************************************/

void print_barb_header(NIDS_barbs *b, char *prefix) {
	int i;
	
	printf("%s.barbs.length %i\n", prefix, b->length);
	printf("%s.barbs.num_barbs %i\n", prefix, b->num_barbs);
	
	for (i = 0 ; i < b->num_barbs ; i++)
		print_barb(b->barbs + i, prefix, i);
	
}

void barb_to_raster(NIDS_image *im, NIDS_barb *b) {
	
	draw_barb(im, b->x_start, b->y_start, b->heading, b->speed);
	
}

/*******************************************************************************
	fuction to draw a barb in an image

args:
						raster	pointer to the raster
						v				the structure that holds the circles
						xcenter	the x axis center in the raster
						ycenter	the y axis center in the raster

returns:
						nothing
*******************************************************************************/

void barbs_to_raster (
	NIDS_image *im,
	NIDS_barbs *b)
{
	int i;
	
	for (i = 0 ; i < b->num_barbs ; i++) {
		barb_to_raster(im, b->barbs + i);
	}
	
}


