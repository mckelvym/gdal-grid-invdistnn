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

import java.io.IOException;
import java.util.Locale;

import javax.imageio.ImageReader;
import javax.imageio.spi.ImageReaderSpi;

import org.gdal.gdal.gdal;

public class JPIPImageReaderSpi extends ImageReaderSpi {

	static final String vendorName = "GDAL";

	static final String version = "0.1";

	static final String readerClassName = "org.gdal.imageio.jpip.JPIPImageReader";

	static final String[] names = { "jpp-stream" };

	static final String[] suffixes = null;

	static final String[] MIMETypes = { "image/jpp-stream" };

	static final String[] writerSpiNames = {};

	// Metadata formats, more information below
	static final boolean supportsStandardStreamMetadataFormat = false;

	static final String nativeStreamMetadataFormatName = null;

	static final String nativeStreamMetadataFormatClassName = null;

	static final String[] extraStreamMetadataFormatNames = null;

	static final String[] extraStreamMetadataFormatClassNames = null;

	static final boolean supportsStandardImageMetadataFormat = false;

	static final String nativeImageMetadataFormatName = null;

	static final String nativeImageMetadataFormatClassName = null;

	static final String[] extraImageMetadataFormatNames = null;

	static final String[] extraImageMetadataFormatClassNames = null;

	public JPIPImageReaderSpi() {
		super(
				vendorName,
				version,
				names,
				suffixes,
				MIMETypes,
				readerClassName,
				STANDARD_INPUT_TYPE, // Accept ImageInputStreams
				writerSpiNames, // no writer
				supportsStandardStreamMetadataFormat,
				nativeStreamMetadataFormatName,
				nativeStreamMetadataFormatClassName,
				extraStreamMetadataFormatNames,
				extraStreamMetadataFormatClassNames,
				supportsStandardImageMetadataFormat,
				nativeImageMetadataFormatName,
				nativeImageMetadataFormatClassName,
				extraImageMetadataFormatNames,
				extraImageMetadataFormatClassNames);
	}

	public String getDescription(Locale locale) {
		return "GDAL JPIP decoder";
	}

	public boolean canDecodeInput(Object source) throws IOException {
		if (source instanceof JPIPImageInputStream) {
			return true;
		} else {
			return false;
		}
	}

	public ImageReader createReaderInstance(Object extension) {
		return new JPIPImageReader(this);
	}

	static {
		gdal.AllRegister();
		gdal.SetConfigOption("CPL_DEBUG", "ON");
	}
	
}
