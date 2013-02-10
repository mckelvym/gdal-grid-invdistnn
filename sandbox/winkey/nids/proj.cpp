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

