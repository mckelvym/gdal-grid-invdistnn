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
#include "v_text.h"
#include "error.h"

/*
Write Text (Uniform Value)
                                                         PRECISION/
FIELDNAME               TYPE     UNITS    RANGE          ACCURACY       REMARKS
Packet Code             INT*2    N/A      8              N/A            Packet Type 8
Length of Block         INT*2    Bytes    1 to 32767     1              Number of bytes in
                                                                        block not including self
                                                                        or packet code
Value (Level) of Text   INT*2    N/A      0 to 15        1              Color Level of text
I Starting Point        INT*2    Km/4     -2048 to +2047 1              I coordinate for text
                                 or                                     starting point
                                 Pixels
J Starting Point        INT*2    Km/4     -2048 to +2047 1              J coordinate for text
                                 or                                     starting point
                                 Pixels
Character 1 to N        Char     8 bit    ASCII          N/A            Characters are ASCII
                                 ASCII    Character Set

*/

/*******************************************************************************
	function to parse a valued text packet
	
args:					buf			the buffer pointing to the start of the valued text packet
							t				the structure to store the text in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_v_text_header(char *buf, NIDS_v_text *t) {
	char *p;
	int i;
	
	t->length = GET2(buf);
	t->num_chars = t->length - 6;
	t->value = GET2(buf + 2);
	t->x_start = GET2(buf + 4);
	t->y_start = GET2(buf + 6);
	
	if (!(t->chars = (char*)CPLMalloc(t->num_chars + 1)))
		ERROR("parse_v_text_header");
	
	p = buf + 8;
	
	for (i = 0 ; i < t->num_chars ; i++, p++) {
		t->chars[i] = *p;
	}
	t->chars[i] = 0;
	
	if (i % 2)
		p++;

	return p;
}
	
/*******************************************************************************
	function to CPLFree any dynamicly alocated memory used in valued text storage

args:
						t				the structure the text is stored in

returns:
						nothing
*******************************************************************************/

void free_v_text_header(NIDS_v_text *t) {
	
	CPLFree(t->chars);
	
}

/*******************************************************************************
	function to print a valued text packet

args:
						t				the structure the text is stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_v_text_header(NIDS_v_text *t, char *prefix) {
	
	printf("%s.v_text.length %i\n", prefix, t->length);
	printf("%s.v_text.num_chars %i\n", prefix, t->num_chars);
	printf("%s.v_text.value %i\n", prefix, t->value);
	printf("%s.v_text.x_start %i\n", prefix, t->x_start);
	printf("%s.v_text.y_start %i\n", prefix, t->y_start);
	printf("%s.v_text.chars %s\n", prefix, t->chars);
	
}

/*******************************************************************************
	fuction to draw a v_text in an image

args:
						raster	pointer to the raster
						t				the structure that holds the text
						xcenter	the x axis center in the raster
						ycenter	the y axis center in the raster

returns:
						nothing
*******************************************************************************/

void v_texts_to_raster (
	NIDS_image *im,
	NIDS_v_text *t)
{
	
	draw_string(im, t->x_start, t->y_start, t->chars, t->value);
	
}
