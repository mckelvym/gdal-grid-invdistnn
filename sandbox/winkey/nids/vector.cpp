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
#include <time.h>
#include <errno.h>

#include "get.h"
#include "NIDS.h"
#include "image.h"
#include "vector.h"
#include "error.h"

/*******************************************************************************
3.6. Unlinked Vector Packet

This is a set of unlinked vectors that plot disconnected lines on the plot.
01 	Packet Code = 7 	Hex "7"
02 	Length of Data 	In bytes
03 	Vector 1, I Start 	Vector 1 start and end coordinates
04 	Vector 1, J Start 	 
05 	Vector 1, I End 	 
06 	Vector 1, J End 	 
07 	Vector 2, I Start 	Vector 2 start and end coordinates
08 	Vector 2, J Start 	 
09 	Vector 2, I End 	 
10 	Vector 2, J End 	 
11-## 	Repeated Vectors 	 
*******************************************************************************/

/*******************************************************************************
	function to parse a unlinked vector
	
args:
							buf			the buffer pointing to the first byte of the vector
							v				the struct to store the vector in;

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_vector(char *buf, NIDS_vector *v) {
	
	v->x_start = GET2(buf);
	v->y_start = GET2(buf + 2);
	v->x_end = GET2(buf + 4);
	v->y_end = GET2(buf + 6);

	return buf + 8;
}

/*******************************************************************************
	function to parse an unlinked vector packet
	
args:					buf			the buffer pointing to the start of the vector packet
							v				the structure to store the vectors in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_vector_header(char *buf, NIDS_vectors *v) {
	int i;
	char *p;
	
	v->length = GET2(buf);
	v->num_vectors = v->length / 8;
	if (!(v->vectors = (NIDS_vector*)CPLMalloc(v->num_vectors * sizeof(NIDS_vector))))
		ERROR("parse_v_vector_header");
	
	p = buf + 2;
	
	for (i = 0 ; i < v->num_vectors ; i++) {
		p = parse_vector(p, v->vectors + i);
	}
	
	return p;
}

/*******************************************************************************
	function to CPLFree any dynamicly alocated memory used in vector storage

args:
						v				the structure the vector is stored in

returns:
						nothing
*******************************************************************************/

void free_vector_header(NIDS_vectors *v) {
	
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

void print_vector(NIDS_vector *v, char *prefix, int rn) {
	
	printf("%s.vector.vectors[%i].x_start %i\n", prefix, rn, v->x_start);
	printf("%s.vector.vectors[%i].y_start %i\n", prefix, rn, v->y_start);
	printf("%s.vector.vectors[%i].x_end %i\n", prefix, rn, v->x_end);
	printf("%s.vector.vectors[%i].y_end %i\n", prefix, rn, v->y_end);
	
}


/*******************************************************************************
	function to print a unlinked vector packet

args:
						v				the structure the vectors is stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_vector_header(NIDS_vectors *v, char *prefix) {
	int i;
	
	printf("%s.vector.length %i\n", prefix, v->length);
	printf("%s.vector.num_vectors %i\n", prefix, v->num_vectors);
	
	for (i = 0 ; i < v->num_vectors ; i++)
		print_vector(v->vectors + i, prefix, i);
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

void vectors_to_raster (
	NIDS_image *im,
	NIDS_vectors *v)
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


