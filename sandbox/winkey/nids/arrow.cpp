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
#include "arrow.h"
#include "error.h"

/*******************************************************************************

Individual Vectors

Each arrow is 5 halfwords and is repeated until byte length.
01 	I Coordinate 	 
02 	J Coordinate 	 
03 	Direction of Arrow 	In degrees
04 	Arrow Length 	In pixels
05 	Arrow Head Length 	In pixels
*******************************************************************************/

/*******************************************************************************
	function to parse a single arrow
	
args:
							buf			the buffer pointing to the first byte of the arrow
							a				the struct to store the arrow in;

returns:
							pointer to the next byte in the buffer 
*******************************************************************************/

char *parse_arrow (char *buf, NIDS_arrow *a) {
	a->x_start = GET2(buf);
	a->y_start = GET2(buf + 2);
	a->heading = GET2(buf + 4);
	a->length = GET2(buf + 6);
	a->head_length = GET2(buf + 8);

	return buf + 10;
}


/*******************************************************************************
3.3. Vector Arrow Packet

These define the location and size of vector arrows to be plotted.
01 	Packet Code = 5 	Hex "5"
02 	Length of Data 	In bytes
*******************************************************************************/

/*******************************************************************************
	function to parse an arrow packet
	
args:					buf			the buffer pointing to the start of the arrow packet
							a				the structure to store the arrows in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_arrow_header(char *buf, NIDS_arrows *a) {
	int i;
	char *p;
	
	a->length = GET2(buf);
	a->num_arrows = a->length / 10;
	if (!(a->arrows = (NIDS_arrow*)CPLMalloc(a->num_arrows * sizeof(NIDS_arrow))))
		ERROR("parse_arrow_header");
	
	p = buf + 2;
	
	for (i = 0 ; i < a->num_arrows ; i++) {
		p = parse_arrow(p, a->arrows + i);
	}
	
	return p;
}

/*******************************************************************************
	function to CPLFree any dynamicly alocated memory used in arrow storage

args:
						a				the structure the arrows are stored in

returns:
						nothing
*******************************************************************************/

void free_arrow_header(NIDS_arrows *a) {

	CPLFree(a->arrows);
	
}

/*******************************************************************************
	function to print a single arrow

args:
						a				the structure the arrow is stored in
						prefix	the start of the line
						rn			the arrow number

returns:
						nothing
*******************************************************************************/

void print_arrow(NIDS_arrow *a, char *prefix, int rn) {
	
	printf("%s.arrow.arrows[%i].x_start %i\n", prefix, rn, a->x_start);
	printf("%s.arrow.arrows[%i].y_start %i\n", prefix, rn, a->y_start);
	printf("%s.arrow.arrows[%i].heading %i\n", prefix, rn, a->heading);
	printf("%s.arrow.arrows[%i].length %i\n", prefix, rn, a->length);
	printf("%s.arrow.arrows[%i].head_length %i\n", prefix, rn, a->head_length);
	
}
	
/*******************************************************************************
	function to print an arrow packet

args:
						a				the structure the arrows are stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_arrow_header(NIDS_arrows *a, char *prefix) {
	int i;
	
	printf("%s.arrow.length %i\n", prefix, a->length);
	printf("%s.arrow.num_arrows %i\n", prefix, a->num_arrows);
	
	for (i = 0 ; i < a->num_arrows ; i++)
		print_arrow(a->arrows + i, prefix, i);
	
}

