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
#include <ogr_spatialref.h>
#include "NIDS.h"
#include "NIDS_proj.h"



/*******************************************************************************
	function to init the projection transformation

	args:
								data				pointer to the nids data
								transform		transform structure
	
 returns:
								nothing
*******************************************************************************/

void NIDS_proj_set_transform(
	NIDS *data,
	NIDS_proj_transform *transform)
{
	
	char to[1000];
	char from[1000];
	
	snprintf(to, sizeof(to), "+proj=latlong +ellps=WGS84");
	snprintf(from, sizeof(from),
						 "+proj=stere +x_0=0 +y_0=0 +lat_0=%f +lon_0=%f"
						 " +a=6378388 +b=6356906 +units=m",
						 data->prod.lat, data->prod.lon);
	
	transform->to = pj_init_plus(to);
	transform->from = pj_init_plus(from);
	
	return;
}

/*******************************************************************************
	function to transform a point

	args:
								transform		transform structure
								x						x val to transform in place
								y						y val to transform in place
								z						z val to transform in place
								
 returns:
								nothing
*******************************************************************************/

void NIDS_proj_do_transform(
	NIDS_proj_transform *transform,
	double *x,
	double *y,
	double *z)
{
	
	/***** nids pixels are 1/4 km resolution *****/
	
	*x *= 250;
	*y *= 250;
	*z *= 250;
	
	pj_transform(transform->from, transform->to, 1, 1, x, y, z);
	
	*x *= RAD_TO_DEG;
	*y *= RAD_TO_DEG;
	*z *= RAD_TO_DEG;
	
	return;
}

