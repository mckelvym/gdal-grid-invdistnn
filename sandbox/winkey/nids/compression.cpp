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

/*
In order to decompress products having been compressed using bzip2,
the libbzip2 library, version 1.0.1 or higher, is required. The source code can
be found at the official home page (URL): http://sources.redhat.com/bzip2 . This
web site contains complete instructions on building the libbzip2 library on a
wide range of computer architectures and operating systems. Detailed
documentation of the various library functions is also provided. Within 
libbzip2, the library function that should be used to decompress the data is:

BZ2_bzBuffToBuffDecompress(
	char *dest,
  unsigned intdestLen,
   char *source,
   unsigned intsourceLen,
   intsmall,
   int verbosity).

The destination buffer “dest” holds the decompressed product. The destination
buffer size “destLen” must be at least as large as the sum of the Message Header
block, Product Description block and the compressed product data size given by
the Product Dependent Parameters (see Table V). The source “source” points to
the compressed product data immediately following the Product Description block.
The source length “sourceLen” is the total product size (defined in the Message
Header block), less the size of the Message Header and Product Description
blocks. Depending on the architecture, “small” can either be 0 (normal case) or
non-zero. By specifying a non-zero value for “small”, the library requires less
memory utilization at the expense of increased decompression time. The verbosity
level can take on any value from 0 to 4 inclusive with higher values denoting
greater verbosity. After the product is decompressed, the products Message
Header and Product Description blocks can be prepended to the decompressed
product data.
*/

