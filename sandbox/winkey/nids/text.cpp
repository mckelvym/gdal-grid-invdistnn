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
#include "text.h"
#include "error.h"

/*
3.8. Text and Symbol Packet

This is for plotting text and symbols.
01 	Packet Code = 1 	Hex "1"
02 	Length of Data 	In bytes
03 	I Start 	Vector 1 start and end coordinates
04 	J Start 	 
05 	Char1 	Char2 	Each character in string
06 	Char3 	Char4 	 
07-## 	Repeated 	 
## 	CharN-1 	CharN 	 

*/

/*******************************************************************************
	function to parse a text packet
	
args:					buf			the buffer pointing to the start of the text packet
							t				the structure to store the text in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_text_header(char *buf, NIDS_text *t) {
	char *p;
	int i;
	
	t->length = GET2(buf);
	t->num_chars = t->length - 4;
	t->x_start = GET2(buf + 2);
	t->y_start = GET2(buf + 4);
	
	if (!(t->chars = (char*)CPLMalloc(t->num_chars + 1)))
		ERROR("parse_text_header");
	
	p = buf + 6;
	
	for (i = 0 ; i < t->num_chars ; i++, p++) {
		t->chars[i] = *p;
	}
	t->chars[i] = 0;
	
	if (i % 2)
		p++;
	
	return p;
}
	
/*******************************************************************************
	function to CPLFree any dynamicly alocated memory used in text storage

args:
						t				the structure the text is stored in

returns:
						nothing
*******************************************************************************/

void free_text_header(NIDS_text *t) {
	
	CPLFree(t->chars);
	
}

/*******************************************************************************
	function to print a text packet

args:
						t				the structure the text is stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_text_header(NIDS_text *t, char *prefix) {
	
	printf("%s.v_text.length %i\n", prefix, t->length);
	printf("%s.text.num_chars %i\n", prefix, t->num_chars);
	printf("%s.text.x_start %i\n", prefix, t->x_start);
	printf("%s.text.y_start %i\n", prefix, t->y_start);
	printf("%s.text.chars %s\n", prefix, t->chars);
	
}

/*******************************************************************************
	fuction to draw a text in an image

args:
						raster	pointer to the raster
						t				the structure that holds the text
						xcenter	the x axis center in the raster
						ycenter	the y axis center in the raster

returns:
						nothing
*******************************************************************************/

void texts_to_raster (
	NIDS_image *im,
	NIDS_text *t)
{
	
	draw_string(im, t->x_start, t->y_start, t->chars, 1);
	
}
