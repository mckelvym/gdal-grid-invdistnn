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
#include "storm_id.h"
#include "error.h"

/*******************************************************************************
            MSB             HALFWORD               LSB
                        PACKET CODE (=15)
STORM ID             LENGTH OF BLOCK (BYTES)
REPEAT FOR                  I POSITION
EACH SYMBOL                 J POSITION
                CHARACTER 1            CHARACTER 2
*******************************************************************************/

/*******************************************************************************
	function to parse a single storm id
	
args:
							buf			the buffer pointing to the first byte of the storm id
							s				the struct to store the storm id in;

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_storm_id(char *buf, NIDS_storm_id *s) {
	char *p;
	
	s->x_pos = GET2(buf);
	s->y_pos = GET2(buf + 2);
	
	p = buf + 4;
	
	s->id[0] = p[0];
	s->id[1] = p[1];
	s->id[2] = 0;
	
	
	return p + 2;
}	



/*******************************************************************************
	function to parse a storm id packet
	
args:					buf			the buffer pointing to the start of the storm id packet
							s				the structure to store the storm ids in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_storm_id_header(char *buf, NIDS_storm_ids *s) {
	char *p;
	int i;
	
	s->length = GET2(buf);
	s->num_ids = s->length / 6;
	

	
	if (!(s->ids = (NIDS_storm_id*)CPLMalloc(s->num_ids * sizeof(NIDS_storm_id))))
		ERROR("parse_storm_id_header");
	
	p = buf + 2;
	
	for (i = 0 ; i < s->num_ids ; i++) {
		p = parse_storm_id(p, s->ids + i);
	}
	
	return p;
}
	
/*******************************************************************************
	function to CPLFree any dynamicly alocated memory used in storm_id storage

args:
						s				the structure the storm_ids are stored in

returns:
						nothing
*******************************************************************************/

void free_storm_id_header(NIDS_storm_ids *s) {
	
	CPLFree(s->ids);
	
}

/*******************************************************************************
	function to print a storm id

args:
						s				the structure the storm id is stored in
						prefix	the start of the line
						rn			the row number

returns:
						nothing
*******************************************************************************/

void print_storm_id(NIDS_storm_id *s, char *prefix, int rn) {
	
	printf("%s.storm_id.ids[%i].x_pos %i\n", prefix, rn, s->x_pos);
	printf("%s.storm_id.ids[%i].y_pos %i\n", prefix, rn, s->y_pos);
	printf("%s.storm_id.ids[%i].id %s\n", prefix, rn, s->id);
	
}
/*******************************************************************************
	function to print a storm id packet

args:
						s				the structure the storm ids are stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_storm_id_header(NIDS_storm_ids *s, char *prefix) {
	int i;
	
	printf("%s.storm_id.length %i\n", prefix, s->length);
	printf("%s.storm_id.num_ids %i\n", prefix, s->num_ids);
	
	for (i = 0; i < s->num_ids; i++)
		print_storm_id(s->ids + i, prefix, i);
	
}
/*******************************************************************************
*******************************************************************************/

void storm_id_to_raster(
	NIDS_image *im,
	NIDS_storm_id *s)
{

	draw_string(im, s->x_pos, s->y_pos, s->id, 1);
	
}

/*******************************************************************************
	fuction to draw a storm id in an image

args:
						t				the structure that holds the storm id
						width		pointer to return the width of the raster in
						height	pointer to return the height of the raster in

returns:
						a char pointer to the raster data
*******************************************************************************/

void storm_ids_to_raster (
	NIDS_image *im,
	NIDS_storm_ids *s)
{
	int i;
	
	for (i = 0; i < s->num_ids; i++) {
		storm_id_to_raster(im, s->ids + i);
	}
	
}
