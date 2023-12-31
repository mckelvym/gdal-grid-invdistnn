#!/usr/bin/env python
#******************************************************************************
# 
#  Project:  GDAL
#  Purpose:  Example doing range based classification
#  Author:   Frank Warmerdam, warmerdam@pobox.com
# 
#******************************************************************************
#  Copyright (c) 2008, Frank Warmerdam <warmerdam@pobox.com>
# 
#  Permission is hereby granted, free of charge, to any person obtaining a
#  copy of this software and associated documentation files (the "Software"),
#  to deal in the Software without restriction, including without limitation
#  the rights to use, copy, modify, merge, publish, distribute, sublicense,
#  and/or sell copies of the Software, and to permit persons to whom the
#  Software is furnished to do so, subject to the following conditions:
# 
#  The above copyright notice and this permission notice shall be included
#  in all copies or substantial portions of the Software.
# 
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
#  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
#  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
#  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
#  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#  DEALINGS IN THE SOFTWARE.
#******************************************************************************

import gdal
import gdalnumeric
try:
    import numpy
except:
    import Numeric as numpy


class_defs = [(1, 10, 20),
              (2, 20, 30),
              (3, 128, 255)]

src_ds = gdal.Open('utm.tif')
xsize = src_ds.RasterXSize
ysize = src_ds.RasterYSize

src_image = gdalnumeric.LoadFile( 'utm.tif' )

dst_image = numpy.zeros((xsize,ysize))

for class_info in class_defs:
    class_id = class_info[0]
    class_start = class_info[1]
    class_end = class_info[2]

    class_value = numpy.ones((xsize,ysize)) * class_id
    
    mask = numpy.bitwise_and(
        numpy.greater_equal(src_image,class_start),
        numpy.less_equal(src_image,class_end))

    dst_image = numpy.choose( mask, (dst_image,class_value) )

gdalnumeric.SaveArray( dst_image, 'classes.tif' )


