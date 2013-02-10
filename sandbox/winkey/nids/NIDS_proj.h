/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */
 
#ifndef _NIDS_PROJ_H
#define _NIDS_PROJ_H

/*******************************************************************************
	structure for projection transformation
*******************************************************************************/

typedef struct {
	projPJ from;
	projPJ to;
} NIDS_proj_transform;

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
	NIDS_proj_transform *transform);

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
	double *z);

#endif /* _NIDS_PROJ_H */

