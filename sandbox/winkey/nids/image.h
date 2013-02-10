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
 
#ifndef _IMAGE_H
#define _IMAGE_H

typedef struct {
	char *raster;
	int x_center;
	int y_center;
	int width;
	int height;
	int scale;			//divisor
} NIDS_image;

void plot(
	NIDS_image *im,
	int x,
	int y,
	int value);

void draw_line(
	NIDS_image *im,
	int x0,
	int x1,
	int y0,
	int y1,
	int value);

void draw_circle(
	NIDS_image *im,
	int x0,
	int y0,
	int radius,
	int value);

void draw_barb(
	NIDS_image *im,
	int xstart,
	int ystart,
	float heading,
	int speed);

void draw_triangle(
	NIDS_image *im,
	int x,
	int y,
	int height,
	int width,
	int value);

void draw_invert_triangle(
	NIDS_image *im,
	int x,
	int y,
	int height,
	int width,
	int value);

void draw_string(
	NIDS_image *im,
	int x_start,
	int y_start,
	char *s,
	int value);

#endif /* _IMAGE_H */

 
