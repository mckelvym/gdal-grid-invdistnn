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
 
#ifndef _FORECAST_H
#define _FORECAST_H

/*******************************************************************************
	function to parse a forecast or past data packet
	
args:					buf			the buffer pointing to the start of the forecast packet
							f				the structure to store the forecast in

returns:
							pointer to the next byte in the buffer
*******************************************************************************/

char *parse_forecast_header(char *buf, NIDS_forecasts *f);

/*******************************************************************************
	function to CPLFree any dynamicly alocated memory used in forecast storage

args:
						f				the structure the forecast is stored in

returns:
						nothing
*******************************************************************************/

void free_forecast_header(NIDS_forecasts *f);

/*******************************************************************************
	function to print the forecast packet

args:
						f				the structure the forecasts are stored in
						prefix	the start of the line

returns:
						nothing
*******************************************************************************/

void print_forecast_header(NIDS_forecasts *f, char *prefix);

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
	NIDS_forecasts *f);

#endif /* _FORECAST_H */

 
