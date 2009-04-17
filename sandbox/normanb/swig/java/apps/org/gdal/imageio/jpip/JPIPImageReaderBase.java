/**
* ITT Visual Information Systems grants you use of this code, under the following license:
* 
* Copyright (c) 2000-2007, ITT Visual Information Solutions 
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions: 
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software. 

* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
**/
package org.gdal.imageio.jpip;

import java.awt.Dimension;

import javax.imageio.ImageReader;
import javax.imageio.spi.ImageReaderSpi;

public abstract class JPIPImageReaderBase
	extends ImageReader 
{
	public JPIPImageReaderBase(ImageReaderSpi originatingProvider)
	{
		super(originatingProvider);
	}

	public abstract Object getRawData();
	
	public abstract Object getRawDataThumbnail();
	
	public abstract int getBytesStreamed();
	
	public abstract Dimension getAdjustedRequestRegion();
	
	public abstract Dimension getAdjustedRequestRegionThumbnail();
}
