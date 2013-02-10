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
#include <math.h>

#include "NIDS.h"
#include "get.h"
#include "image.h"
#include "linked_vector.h"
#include "error.h"

/*******************************************************************************
                    MSB       HALFWORD         LSB
                               No Value
                         PACKET CODE (=6)
                   LENGTH OF DATA BLOCK (BYTES)
                         I STARTING POINT             1/4 Km or
                         J STARTING POINT             Screen Coordinates
DATA                  END I VECTOR NUMBER 1
BLOCK                 END J VECTOR NUMBER 1
                      END I VECTOR NUMBER 2
                      END J VECTOR NUMBER 2
                                   •
                                   •
*******************************************************************************/


/*******************************************************************************
	function to parse a linked vector
	
args:
							buf			the buffer pointing to the first byte of the vector
							v				the struct to store the vector in;

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_linked_vector(char *buf, NIDS_vector *v, int x_start, int y_start) {
	
	v->x_start = x_start;
	v->y_start = y_start;
	v->x_end = GET2(buf);
	v->y_end = GET2(buf + 2);

	return buf + 4;
}

/*******************************************************************************
	function to parse a linked vector packet
	
args:					buf			the buffer pointing to the start of the linked vector packet
							v				the structure to store the linked vectors in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_linked_vector_header(char *buf, NIDS_linked_vectors *v) {
	int i;
	char *p;
	int x_start;
	int y_start;
	
	v->length = GET2(buf);
	v->num_vectors = (v->length - 4) / 4;
	x_start = GET2(buf + 2);
	y_start = GET2(buf + 4);
	
	if (!(v->vectors = (NIDS_vector*)CPLMalloc(v->num_vectors * sizeof(NIDS_vector))))
		ERROR("parse_linked_vector_header");
	
	p = buf + 6;
	
	for (i = 0 ; i < v->num_vectors ; i++) {
		p = parse_linked_vector(p, v->vectors + i, x_start, y_start);
	}
	
	return p;
}

/*******************************************************************************
	function to CPLFree any dynamicly alocated memory used in linked vector storage

args:
						v				the structure the vector is stored in

returns:
						nothing
*******************************************************************************/

void free_linked_vector_header(NIDS_linked_vectors *v) {
	
	CPLFree(v->vectors);
	
}

/*******************************************************************************
	function to print a single unlinked vector

args:
						v				the structure the vector is stored in
						prefix	the start of the line
						rn			the vector number

returns:
						nothing
*******************************************************************************/

void print_linked_vector(NIDS_vector *v, char *prefix, int rn) {
	
	printf("%s.linked_vector.vectors[%i].x_start %i\n", prefix, rn, v->x_start);
	printf("%s.linked_vector.vectors[%i].y_start %i\n", prefix, rn, v->y_start);
	printf("%s.linked_vector.vectors[%i].x_end %i\n", prefix, rn, v->x_end);
	printf("%s.linked_vector.vectors[%i].y_end %i\n", prefix, rn, v->y_end);
	
}


/*******************************************************************************
	function to print a linked vector packet

args:
						v				the structure the vectors is stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_linked_vector_header(NIDS_linked_vectors *v, char *prefix) {
	int i;
	
	printf("%s.linked_vector.length %i\n", prefix, v->length);
	printf("%s.linked_vector.num_vectors %i\n", prefix, v->num_vectors);
	
	for (i = 0 ; i < v->num_vectors ; i++)
		print_linked_vector(v->vectors + i, prefix, i);
}

/*******************************************************************************
	fuction to draw a vector in an image

args:
						raster	pointer to the raster
						v				the structure that holds the vectors
						xcenter	the x axis center in the raster
						ycenter	the y axis center in the raster

returns:
						nothing
*******************************************************************************/


void linked_vectors_to_raster (
	NIDS_image *im,
	NIDS_linked_vectors *v)
{
	int i;
	
	for (i = 0 ; i < v->num_vectors ; i++) {
		draw_line(im,
							v->vectors[i].x_start,
							v->vectors[i].x_end,
							v->vectors[i].y_start,
							v->vectors[i].y_end,
							1);
	}
	
}
