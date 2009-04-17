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
import java.awt.Rectangle;
import java.awt.color.ColorSpace;
import java.awt.image.BufferedImage;
import java.awt.image.DataBuffer;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Iterator;

import javax.imageio.IIOException;
import javax.imageio.ImageIO;
import javax.imageio.ImageReadParam;
import javax.imageio.ImageTypeSpecifier;
import javax.imageio.metadata.IIOMetadata;
import javax.imageio.spi.ImageReaderSpi;

import com.sun.media.imageioimpl.plugins.jpeg2000.SignatureBox;


public class JPIPImageReader extends JPIPImageReaderBase {
	private JPIPImageInputStream stream = null;

	private int width, height;
	private int colorType;
	private int nDataRead;
	
	// Constants enumerating the values of colorType
	static final int COLOR_TYPE_GRAY = 0;
	static final int COLOR_TYPE_RGB = 1;

	private J2KMetadata metadata = null;
	private ReadNewDataTask task;
	private ReadNewDataTask thumbnailTask;

	int c = 0;
	
	public void setInput(Object input) {
		super.setInput(input);

		if (input == null) {
			this.stream = null;
			return;
		}

		this.stream = (JPIPImageInputStream) input;
		this.width = stream.getXSize();
		this.height = stream.getYSize();

	}

	// abort the reader tasks
	public void abort()
	{
		if (task != null)
		{
			task.abort();
			task = null;
		}

		if (thumbnailTask != null)
		{
			thumbnailTask.abort();
			thumbnailTask = null;
		}

		super.abort();
	}

	public int getNumImages(boolean allowSearch) throws IIOException {
		return 1; // format can only encode a single image
	}

	private void checkIndex(int imageIndex) {
		if (imageIndex != 0) {
			throw new IndexOutOfBoundsException("bad index");
		}
	}

	public ImageReadParam getDefaultReadParam()
	{
		J2KImageReadParam param = new J2KImageReadParam();

		param.setQuality(stream.getNLayers());

		int level = stream.getDecompositionLevels();

		int subsampling = (int)Math.pow(2, level);
		param.setSourceSubsampling(subsampling, subsampling, 0, 0);

		return param;
	}

	public int getWidth(int imageIndex) throws IIOException {
		checkIndex(imageIndex); // must throw an exception if != 0
		return width;
	}

	public int getHeight(int imageIndex) throws IIOException {
		checkIndex(imageIndex);
		return height;
	}

	public JPIPImageReader(ImageReaderSpi originatingProvider) {
		super(originatingProvider);
	}

	/*
	 * (non-Javadoc)
	 *
	 * @see javax.imageio.ImageReader#getImageMetadata(int)
	 */
	public IIOMetadata getImageMetadata(int imageIndex) throws IOException {
		checkIndex(imageIndex);
		if ((metadata == null) && (stream != null))
		{
			// implement a minimal metadata object
			metadata = new J2KMetadata();
			metadata.addNode(new SignatureBox());			
			metadata.addNode(new HeaderBox(height, width, stream.getNComp(), stream.getBitDepth(),
			            -1, -1, -1));
			
			// get the geotiff info from the stream
			metadata.addNode(stream.getUUIDBox());      	
		}
		
		return metadata;
	}

	/*
	 * (non-Javadoc)
	 *
	 * @see javax.imageio.ImageReader#getImageTypes(int)
	 */
	public Iterator getImageTypes(int imageIndex) throws IOException {
		checkIndex(imageIndex);

		ImageTypeSpecifier imageType = null;
		int datatype = DataBuffer.TYPE_BYTE;
		if (stream.getBitDepth() > 8)
			datatype = DataBuffer.TYPE_SHORT;

		java.util.List l = new ArrayList();
		ColorSpace rgb = ColorSpace.getInstance(ColorSpace.CS_sRGB);
		int[] bandOffsets = new int[3];
		bandOffsets[0] = 0;
		bandOffsets[1] = 1;
		bandOffsets[2] = 2;
		imageType = ImageTypeSpecifier.createInterleaved(rgb, bandOffsets,
				datatype, false, false);
		l.add(imageType);
		return l.iterator();
	}

	/*
	 * (non-Javadoc)
	 *
	 * @see javax.imageio.ImageReader#getStreamMetadata()
	 */
	public IIOMetadata getStreamMetadata() throws IOException {
		return null;
	}

	public int getNumQualityLayers() {
		return stream.getNLayers();
	}

	public int getNumComponents() {
		return stream.getNComp();
	}

	public int getNumDecompositionLevels() {
		return stream.getDecompositionLevels();
	}

	/*
	 * (non-Javadoc)
	 *
	 * @see javax.imageio.ImageReader#read(int, javax.imageio.ImageReadParam)
	 */
	public BufferedImage read(int imageIndex, ImageReadParam param)
	throws IOException {
		J2KImageReadParam j2kParam = (J2KImageReadParam)param;

		if (!j2kParam.isThumbnail())
		{
			// start a reader task for the main image
			if (task == null)
			{
				task = new ReadNewDataTask(imageIndex);
				task.setParameters(param);
				task.start();
			}
			else
			{
				task.setParameters(param);
			}

			// pass along the new parameters and tell it to restart
		}
		else
		{
			// start a reader task for the thumbnail image
			if (thumbnailTask == null)
			{
				thumbnailTask = new ReadNewDataTask(imageIndex);
				thumbnailTask.setParameters(param);
				thumbnailTask.start();
			}
			else
			{
				// pass along the new parameters and tell it to restart
				thumbnailTask.setParameters(param);
			}
		}

		return null;
	}

	/**
	 * Return the number of bytes that have been received so far through
	 * JPIP .
	 * @return Number of bytes received so far
	 */
	public int getBytesStreamed()
	{
		return nDataRead;
	}

	public Dimension getAdjustedRequestRegion() {
		// TODO Auto-generated method stub
		return null;
	}

	public Dimension getAdjustedRequestRegionThumbnail() {
		// TODO Auto-generated method stub
		return null;
	}

	public Object getRawData() {
		return null;
	}

	public Object getRawDataThumbnail() {
		return null;
	}

	class ReadNewDataTask extends Thread
	{
		private int imageIndex;

		private boolean isRestartRequested = false;
		private boolean isAbortRequested = false;
		private boolean isThumbnail =false;
		private J2KImageReadParam param;
		private int bytesRead = 0;

		public ReadNewDataTask(int imageIndex)
		{
			this.imageIndex = imageIndex;
		}

		public void setParameters(ImageReadParam param)
		{
			this.param = (J2KImageReadParam)param;
			isRestartRequested = true;
		}

		public void run()
		{
			Rectangle requestRegion = null;
			int sourceXSubsampling = 0;
			int sourceYSubsampling = 0;
			int subsampling = 1;
			int level = stream.getDecompositionLevels();
			int quality = stream.getNLayers();
			int[] bands = null;
			boolean isThumbnail = false;
			boolean doRestart = false;
			
			try
			{
				while (!isAbortRequested)
				{
					if (isRestartRequested && (param != null))
					{
						doRestart = true;
						isRestartRequested = false;

						// set up the input stream
						requestRegion = param.getSourceRegion();
						sourceXSubsampling = param.getSourceXSubsampling();
						sourceYSubsampling = param.getSourceYSubsampling();
						isThumbnail = param.isThumbnail();
						quality = param.getQuality();
						bands = param.getSourceBands();

						// use default band mapping if one wasn't specified
						if (bands == null)
						{
							int comps = stream.getNComp();

							if (comps < 3)
							{
								// generate a greyscale image based on the 
								// first component
								bands = new int[1];
								bands[0] = 0;
							}
							else
							{
								// generate a colour image based on the first three
								// components
								bands = new int[3];
								bands[0] = 0; bands[1] = 1; bands[2] = 2;
							}
						}

						// check the subsampling values
						if (sourceXSubsampling != sourceYSubsampling)
						{
							throw new IIOException(
									"Unsupported resolution request - X and Y subsampling values must be equal");
						}
						else if ((sourceXSubsampling != 1) &&
									(sourceXSubsampling % 2 != 0))
						{
							throw new IIOException(
									"Unsupported resolution request - X and Y subsampling values must be powers of 2"
									);
						}
						else
						{
							subsampling = sourceXSubsampling;
							// turn the subsampling value into a level (power of 2)
							level = (int)(Math.log(subsampling) / Math.log(2.0));
						}
						
						// default to all quality layers
						if (quality < 0)
							quality = stream.getNLayers();
					}
					
					
	
					if ((doRestart) || !stream.isWindowDone(isThumbnail))
					{
						doRestart = false;
						
						nDataRead = stream.getNDataRead(true) + stream.getNDataRead(false);
						
						processImageProgress(getBytesStreamed());

						BufferedImage img = stream.read(requestRegion, bands, quality, level, isThumbnail);
						if (img != null)
						{
							if (isThumbnail)
							{
								// process thumbnail update
								Rectangle region = stream.getRegion(isThumbnail);
								processThumbnailUpdate(img, (int)region.getX(),
										(int)region.getY(), (int)region.getWidth(), 
										(int)region.getHeight(), subsampling, subsampling, bands);
							}
							else
							{
								// process main image update
								Rectangle region = stream.getRegion(isThumbnail);

								processImageUpdate(img, (int)region.getX(),
										(int)region.getY(), (int)region.getWidth(), 
										(int)region.getHeight(), subsampling, subsampling, bands);
							}
						}
						
						if (stream.isWindowDone(isThumbnail))
						{
							if (isThumbnail)
							{
								processThumbnailComplete();
							}
							else
							{
								processImageComplete();
							}
						}

					}
					
					
					Thread.sleep(param.getRefresh());
				}
				
			}
			catch(Exception e)
			{
				e.printStackTrace();
				throw new RuntimeException("Error decompressing data", e);
			}
		}

		public void abort()
		{
			isAbortRequested = true;
		}
	}
}
