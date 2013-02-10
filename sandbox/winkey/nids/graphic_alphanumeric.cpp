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
#include "image.h"
#include "v_vector.h"
#include "v_text.h"
#include "get.h"
#include "error.h"
#include "graphic_alphanumeric.h"

/*******************************************************************************
GRAPHIC ALPHANUMERIC BLOCK
                                              PRECISION/
FIELDNAME        TYPE    UNITS    RANGE       ACCURACY   REMARKS
Block Divider    INT*2   N/A      -1          N/A        Integer value of -1
                                                         used to delineate the
                                                         Graphic Alphanumeric
                                                         Block
Block ID         INT*2   N/A      2           N/A        Constant value of 2
                                                         which identifies this
                                                         block
Length of Block  INT*4   Bytes    1 to 65535  1          Length of block in
                                                         bytes (includes
                                                         preceding divider and
                                                         block id) from the
                                                         divider to the end of
                                                         message
Number of Pages  INT*2   N/A      1 to 48     1          Total number of pages
Page Number      INT*2   N/A      1 to 48     1          Current page number
Length of Page   INT*2   Bytes    4 to 1360   1          Number of bytes in
                                                         Text Packet 1 through
                                                         Text Packet N
Text Packet (N)  N/A     N/A      N/A         N/A        The format of these
                                                         text packets are Packet
                                                         Code 8, shown in
                                                         Figure 3-8b, and
                                                         Packet Code 10, shown
                                                         in Figure 3-8


                        MSB     HALFWORD         LSB
GRAPHIC
                               BLOCK DIVIDER (-1)
ALPHANUMERIC                     BLOCK ID (2)
                          LENGTH OF BLOCK (MSW)
BLOCK
                          LENGTH OF BLOCK (LSW)
                             NUMBER OF PAGES
REPEAT FOR
                                PAGE NUMBER
EACH PAGE
                              LENGTH OF PAGE
                               TEXT PACKET 1
                                      •
                                      •
                                      •
                               TEXT PACKET N
      Figure 3-6. Graphic Product Message (Sheet 4)

*******************************************************************************/

char *parse_graphic_alphanumeric_text(char *buf, NIDS_graphic_alphanumeric_text *g) {
	char *p;
	
	g->data_type = GET2(buf);
	
	p = buf + 2;
	
	switch(g->data_type) {
		case V_VECTOR:
			p = parse_v_vector_header(p, &(g->v_vector));
			break;
		
		case V_TEXT:
			p = parse_v_text_header(p, &(g->v_text));
			break;
		
		default:
			printf("unknown graphic_alphanumeric text %04x\n", g->data_type);
	}
	
	return p;
}

char *parse_graphic_alphanumeric_page(char *buf, NIDS_graphic_alphanumeric_page *g) {
	int i;
	char *p;
	NIDS_graphic_alphanumeric_text *temp;
	
	g->page = GET2(buf);
	g->length = GET2(buf + 2);
	
	p = buf + 4;
	g->num_texts = 0;
	g->texts = NULL;
	
	for (i = 0; p - buf + 4 < (int)g->length; i++) {
		g->num_texts++;
		
		if (!(temp = (NIDS_graphic_alphanumeric_text*) CPLRealloc(g->texts, g->num_texts * sizeof(NIDS_graphic_alphanumeric_text))))
			ERROR("parse_graphic_alphanumeric_page");
		g->texts = temp;
		
		p = parse_graphic_alphanumeric_text(p, g->texts + i);
	}
	
	return p;
};

/*******************************************************************************
	function to parse the graphic alphanumeric block
	
args:					buf			the buffer pointing to the start of the block
							g				the structure to store the block in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_graphic_alphanumeric(char *buf, NIDS_graphic_alphanumeric *g) {
	int i;
	char *p;
	
	g->id = GET2(buf + 2);
	g->length = GET4(buf + 4);
	g->num_pages = GET2(buf + 8);
	
	p = buf + 10;
	
	if (!(g->pages = (NIDS_graphic_alphanumeric_page*) CPLMalloc(g->num_pages * sizeof(NIDS_graphic_alphanumeric_page))))
		ERROR("parse_graphic_alphanumeric");
	
	for (i = 0 ; i < g->num_pages ; i++) {
		p = parse_graphic_alphanumeric_page(p, g->pages + i);
	}
	
	return p;
}
	
/*******************************************************************************
	function to check if a block is a graphic alphanumeric block
	
args:
					buf 				the buffer pointing to the start of the block

returns:
					1 	if the block is a graphic_alphanumeric block
					0		if it is not
*******************************************************************************/

int is_graphic_alphanumeric(char *buf) {
	int result = 0;
	
	if (GET2(buf + 2) == 2)
		result = 1;
	
	return result;
}
	
/*******************************************************************************
	function to CPLFree all the memory used in a graphic_alphanumeric text

args:
						g				the structure the text is stored in

returns:
						nothing
*******************************************************************************/

void free_graphic_alphanumeric_text (NIDS_graphic_alphanumeric_text *g) {
	
	switch (g->data_type) {
		case V_VECTOR:
			free_v_vector_header(&(g->v_vector));
			break;
		
		case V_TEXT:
			free_v_text_header(&(g->v_text));
			break;
		
		default:
			printf("unknown graphic_alphanumeric text 0x%04x\n", g->data_type);
						 
	}
}

/*******************************************************************************
function to CPLFree a graphic alphanumeric page
*******************************************************************************/

void free_graphic_alphanumeric_page(NIDS_graphic_alphanumeric_page *g) {
	int i;
	
	for (i = 0 ; i < g->num_texts ; i++)
		free_graphic_alphanumeric_text(g->texts + i);

	CPLFree(g->texts);
}

/*******************************************************************************
	function to CPLFree any dynamicly alocated memory used in graphic alphanumeric storage

args:
						h				the structure the product symbology is stored in

returns:
						nothing
*******************************************************************************/

void free_graphic_alphanumeric(NIDS_graphic_alphanumeric *g) {
	int i;
	
	for (i = 0 ; i < g->num_pages ; i++)
		free_graphic_alphanumeric_page(g->pages + i);

	CPLFree(g->pages);
	
}
	
/*******************************************************************************
	function to print a graphic_alphanumeric text

args:
						g				the structure the text is stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_graphic_alphanumeric_text (NIDS_graphic_alphanumeric_text *g, char *prefix) {
		
	printf ("%s.data_type %i\n", prefix, g->data_type);
		
	switch (g->data_type) {
		case V_VECTOR:
			print_v_vector_header(&(g->v_vector), prefix);
			break;
		
		case V_TEXT:
			print_v_text_header(&(g->v_text), prefix);
			break;
		
		default:
			printf("unknown graphic_alphanumeric text 0x%04x\n", g->data_type);
							 
	}
	
}

/*******************************************************************************
function to print a graphic alphanumeric page

args:
						g				the structure the page is stored in
						prefix	the start of the line

*******************************************************************************/

void print_graphic_alphanumeric_page(NIDS_graphic_alphanumeric_page *g, char *prefix) {
	int i;
	char myprefix[PREFIX_LEN];
	
	printf("%s.page %i\n", prefix, g->page);
	printf("%s.length %i\n", prefix, g->length);
	printf("%s.num_texts %i\n", prefix, g->num_texts);
	
	for (i = 0 ; i < g->num_texts ; i++) {
		snprintf(myprefix, PREFIX_LEN, "%s.texts[%i]", prefix, i);
		print_graphic_alphanumeric_text(g->texts + i, myprefix);
	}

}

/*******************************************************************************
	function to print the graphic_alphanumeric block

args:
						g				the structure the block is stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_graphic_alphanumeric(NIDS_graphic_alphanumeric *g, char *prefix) {
	int i;
	char myprefix[PREFIX_LEN];
	
	printf("%s.graphic.id %i\n", prefix, g->id);
	printf("%s.graphic.length %u\n", prefix, g->length);
	printf("%s.graphic.num_pages %i\n", prefix, g->num_pages);
	
	
	for (i = 0 ; i < g->num_pages ; i++) {
		snprintf(myprefix, PREFIX_LEN, "%s.graphic.pages[%i]", prefix, i);
		print_graphic_alphanumeric_page(g->pages + i, myprefix);
	}

}

/*******************************************************************************
	function to convert a graphic_alphanumeric text to a raster

args:
						g				the structure the text is stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void graphic_alphanumeric_text_to_raster (
	NIDS_image *im,
	NIDS_graphic_alphanumeric_text *g)
{
	
	im->x_center = 0;
	im->y_center = 0;
	
	switch (g->data_type) {
		case V_VECTOR:
			v_vectors_to_raster(im, &(g->v_vector));
			break;
		
		case V_TEXT:
			v_texts_to_raster(im, &(g->v_text));
			break;
		
		default:
			printf("unknown graphic_alphanumeric text 0x%04x\n", g->data_type);
							 
	}
	
}

/*******************************************************************************
	function to convert a graphic_alphanumeric page to a raster

args:
						g				the structure the graphic_alphanumeric page is stored in

returns:
						nothing
*******************************************************************************/

void graphic_alphanumeric_page_to_raster(
	NIDS_image *im,
	NIDS_graphic_alphanumeric_page *g)
{
	int i;
	
	for (i = 0 ; i < g->num_texts ; i++)
		graphic_alphanumeric_text_to_raster(im, g->texts + i);

}

/*******************************************************************************
	function to convert the graphic_alphanumeric block to a raster

args:
						g				the structure the graphic_alphanumeric is stored in

returns:
						nothing
*******************************************************************************/

void graphic_alphanumeric_to_raster(
	NIDS_image *im,
	NIDS_graphic_alphanumeric *g)
{
	int i;
	
	for (i = 0 ; i < g->num_pages ; i++)
		graphic_alphanumeric_page_to_raster(im, g->pages + i);
	
}

