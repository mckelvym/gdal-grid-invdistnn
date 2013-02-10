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
 
#ifndef _MYZLIB_H
#define _MYZLIB_H

#define BUFSIZE 8192

typedef struct {
	char *buf;
	size_t alloced;
	size_t used;
	size_t parsed;
} buffer;

char *zerr(int err);

int is_zlib(unsigned char *buf);

int unzlib(buffer *in, buffer *out);


#endif /* MYZLIB_H */

