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
#include "circle.h"
#include "text.h"
#include "linked_vector.h"
#include "forecast.h"
#include "error.h"

/*******************************************************************************
              MSB       HALFWORD           LSB
SCIT PAST/         PACKET CODE (=23 or 24)
FORECAST DATA     LENGTH OF BLOCK (BYTES)
                   DISPLAY DATA PACKETS
                             •
                             •
*******************************************************************************/




/*******************************************************************************
	function to parse a forecast or past data layer
	
args:					buf			the buffer pointing to the start of the forecast
							f				the structure to store the forecast in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_forecast(char *buf, NIDS_forecast *f) {
	char *p;
	
	f->data_type = GET2(buf);
	
	p = buf + 2;
	
	switch(f->data_type) {
		case TEXT2:
			p = parse_text_header(p, &(f->text));
			break;
		
		case LINKED_VECTOR:
			p = parse_linked_vector_header(p, &(f->linked_vector));
			break;
		
		case CIRCLE3:
			p = parse_circle_header(p, &(f->circle));
			break;
		
		default:
			printf("unknown forecast layer %04x\n", f->data_type);
	}
	
	return p;
}
	


/*******************************************************************************
	function to parse a forecast or past data packet
	
args:					buf			the buffer pointing to the start of the forecast packet
							f				the structure to store the forecast in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_forecast_header(char *buf, NIDS_forecasts *f) {
	int i;
	char *p;
	NIDS_forecast *temp;
	
	f->length = GET2(buf);
	
	p = buf + 2;
	f->num_forecasts = 0;
	f->forecasts = NULL;
	
	for (i = 0; p - buf + 2 < f->length; i++) {
		f->num_forecasts++;
		
		if (!(temp = (NIDS_forecast*)CPLRealloc(f->forecasts, f->num_forecasts * sizeof(NIDS_forecast))))
			ERROR("parse_forecast_header");
		f->forecasts = temp;
		
		p = parse_forecast(p, f->forecasts + i);
	}
	
	return p;
};

/*******************************************************************************
	function to CPLFree all the memory used in a forecast layer

args:
						f				the structure the layer is stored in

returns:
						nothing
*******************************************************************************/

void free_forecast (NIDS_forecast *f) {
	
	switch (f->data_type) {

		case CIRCLE3:
			free_circle_header(&(f->circle));
			break;
		
		case TEXT2:
			free_text_header(&(f->text));
			break;
		
		case LINKED_VECTOR:
			free_linked_vector_header(&(f->linked_vector));
			break;
		
		default:
			printf("unknown forecast layer 0x%04x\n", f->data_type);
						 
	}
}

/*******************************************************************************
	function to CPLFree any dynamicly alocated memory used in forecast storage

args:
						f				the structure the forecast is stored in

returns:
						nothing
*******************************************************************************/

void free_forecast_header(NIDS_forecasts *f) {
	int i;
	
	for (i = 0 ; i < f->num_forecasts ; i++)
		free_forecast (f->forecasts + i);

	CPLFree(f->forecasts);
}

/*******************************************************************************
	function to print a single forecast layer

args:
						v				the structure the layer is stored in
						prefix	the start of the line
						rn			the layer number

returns:
						nothing
*******************************************************************************/

void print_forecast(NIDS_forecast *f, char *prefix, int rn) {

	printf("%s.data_type %04x\n", prefix, f->data_type);
	
	switch (f->data_type) {
		case CIRCLE3:
			print_circle_header(&(f->circle), prefix);
			break;
		
		case TEXT2:
			print_text_header(&(f->text), prefix);
			break;
		
		case LINKED_VECTOR:
			print_linked_vector_header(&(f->linked_vector), prefix);
			break;
		
		default:
			printf("unknown forecast layer 0x%04x\n", f->data_type);
							 
	}
	
}

/*******************************************************************************
	function to print the forecast packet

args:
						f				the structure the forecasts are stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_forecast_header(NIDS_forecasts *f, char *prefix) {
	int i;
	char myprefix[PREFIX_LEN];
	
	printf("%s.forecast.length %i\n", prefix, f->length);
	printf("%s.forecast.num_forecasts %i\n", prefix, f->num_forecasts);
	
	for (i = 0 ; i < f->num_forecasts ; i++) {
		snprintf(myprefix, PREFIX_LEN, "%s.forecast.forecasts[%i]", prefix, i);
		print_forecast(f->forecasts + i, myprefix, i);
	}
	
}

/*******************************************************************************
	fuction to draw fa singel forecast layer in an image

args:
						raster	pointer to the raster
						f				the structure that holds the forcast
						xcenter	the x axis center in the raster
						ycenter	the y axis center in the raster

returns:
						nothing
*******************************************************************************/

void forecast_to_raster (
	NIDS_image *im,
	NIDS_forecast *f)
{
	switch (f->data_type) {
		case CIRCLE3:
			circles_to_raster(im, &(f->circle));
			break;
		
		case TEXT2:
			texts_to_raster(im, &(f->text));
			break;
		
		case LINKED_VECTOR:
			linked_vectors_to_raster(im, &(f->linked_vector));
			break;
		
		default:
			printf("unknown forecast layer 0x%04x\n", f->data_type);
							 
	}
	
}

/*******************************************************************************
	fuction to draw forcasts in an image

args:
						raster	pointer to the raster
						f				the structure that holds the forcasts
						xcenter	the x axis center in the raster
						ycenter	the y axis center in the raster

returns:
						nothing
*******************************************************************************/

void forecasts_to_raster (
	NIDS_image *im,
	NIDS_forecasts *f)
{
	int i;
	
	for (i = 0 ; i < f->num_forecasts ; i++) {
		forecast_to_raster(im, f->forecasts + i);
	}
	
}
