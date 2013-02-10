/*
 * This program is CPLFree software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the CPLFree Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the CPLFree Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */
 
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
