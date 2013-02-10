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
#include <time.h>
#include "NIDS.h"
#include <math.h>

#define PI 3.14159265

void convert(float angle, float delta, int bin, int *x, int *y) {
	float r_angle = angle * (PI / 180);
	
	*x = bin * cos(r_angle);
	*y = bin * sin(r_angle);

}


void radial2raster (NIDS_radial *r, char *raster, int width, int height) {
	int x, y;
	int i, j;
	float k, angle;
	int bin = 1;
	
	int xcenter = width / 2;
	int ycenter = height / 2;
		
	for (i = 0 ; i < r->num_rle * 2 ; i++) {
		for (j = 0 ; j < r->run[i] ; j++, bin++) {
			for (k = 0 ; k <= r->delta ; k += 0.1) {
				
				if (r->start + k > 360)
					angle = k;
				else
					angle = r->start + k;
			
				convert(angle , r->delta, bin, &x, &y);
				
				if (xcenter + x >= width || xcenter + x < 0)
					fprintf(stderr, "WARNING: raster x value %i out of range, skipping\n", x + xcenter);
				else if (ycenter + -y >= height || ycenter + -y < 0)
					fprintf(stderr, "WARNING: raster x value %i out of range, skipping\n", y + ycenter);
				
				else {
					//printf("raster[%i]\n", (x + xcenter) + (width * (y + ycenter)));
					raster[(-x + xcenter) + (width * (y + ycenter))] = r->code[i];
				//printf ("x=%i y=%i\n", x, y);
				}
			}
		}
	}
	
}

void radials2raster (NIDS_radials *r, char *raster, int width, int height) {
	int i;
	
	for (i = 0 ; i < r->num_radials ; i++)
		radial2raster(r->radials + i, raster, width, height);
	
}
