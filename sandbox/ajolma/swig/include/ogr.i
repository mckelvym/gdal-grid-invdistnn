/******************************************************************************
 * $Id: ogr.i 13163 2007-11-30 22:24:37Z ajolma $
 *
 * Project:  OGR Core SWIG Interface declarations.
 * Purpose:  OGR declarations.
 * Author:   Howard Butler, hobu@iastate.edu
 *
 ******************************************************************************
 * Copyright (c) 2005, Howard Butler
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
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

%include ogr_init.i

%feature("compactdefaultargs");
%feature("autodoc");

%include ogr_headers.i
%include cpl_headers.i
%include ogr_enums.i

%include exception.i

/* typemaps are language specific */
%include gdal_typemaps.i

%import osr.i

/* renames are language specific */
%include ogr_renames.i

%include ogr_global.i

%include ogr_classes.i

/* language specific extensions */
%include ogr_extensions.i
