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
#include "point.h"
#include "error.h"

/*******************************************************************************
                             MSB               HALFWORD                      LSB
                                         PACKET CODE (=12 or 26)
   TVS or ETVS                         LENGTH OF BLOCK (BYTES)
   REPEAT FOR                                  I POSITION
   EACH SYMBOL                                 J POSITION
                             MSB               HALFWORD                      LSB
                                           PACKET CODE (=13)
   HAIL POSITIVE
   (FILLED)                            LENGTH OF BLOCK (BYTES)
   REPEAT FOR                                  I POSITION
   EACH SYMBOL                                 J POSITION
                             MSB               HALFWORD                      LSB
                                           PACKET CODE (=14)
   HAIL PROBABLE                       LENGTH OF BLOCK (BYTES)
   REPEAT FOR                                  I POSITION
   EACH SYMBOL                                 J POSITION
Figure 3-14. Special Graphic Symbol Packet - Packet Code 3 or 11, 12 or 26, 13 and 14
                                     (Sheet 1)

*******************************************************************************/

/*******************************************************************************
	function to parse a single point
	
args:
							buf			the buffer pointing to the first byte of the point
							p				the struct to store the point in;

returns:
							pointer to the next byte in the buffer 
*******************************************************************************/

char *parse_point (char *buf, NIDS_point *p) {
	p->x_start = GET2(buf);
	p->y_start = GET2(buf + 2);

	return buf + 4;
}

/*******************************************************************************
	function to parse a point packet
	
args:					buf			the buffer pointing to the start of the point packet
							p				the structure to store the points in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_point_header(char *buf, NIDS_points *p) {
	int i;
	char *ptr;
	
	p->length = GET2(buf);
	p->num_points = p->length / 4;
	
	if (!(p->points = (NIDS_point*)CPLMalloc(p->num_points * sizeof(NIDS_point))))
		ERROR("parse_point_header");
	
	ptr = buf + 2;
	
	for (i = 0 ; i < p->num_points ; i++) {
		ptr = parse_point(ptr, p->points + i);
	}
	
	return ptr;
}

/*******************************************************************************
	function to CPLFree any dynamicly alocated memory used in point storage

args:
						p				the structure the points are stored in

returns:
						nothing
*******************************************************************************/

void free_point_header(NIDS_points *p) {

	CPLFree(p->points);
	
}

/*******************************************************************************
	function to print a single point

args:
						p				the structure the point is stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_point(NIDS_point *p, char *prefix) {
	
	printf("%s.x_start %i\n", prefix, p->x_start);
	printf("%s.y_start %i\n", prefix, p->y_start);
	
}
	
/*******************************************************************************
	function to print a point packet

args:
						p				the structure the points are stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_point_header(NIDS_points *p, char *prefix) {
	int i;
	char myprefix[PREFIX_LEN];
	
	printf("%s.point.length %i\n", prefix, p->length);
	printf("%s.point.num_points %i\n", prefix, p->num_points);
	
	for (i = 0 ; i < p->num_points ; i++) {
		snprintf(myprefix, PREFIX_LEN, "%s.point.points[%i]", prefix, i);
		print_point(p->points + i, myprefix);
	}
	
}

