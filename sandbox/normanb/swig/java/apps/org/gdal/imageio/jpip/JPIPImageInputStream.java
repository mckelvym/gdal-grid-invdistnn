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

import java.awt.Rectangle;
import java.awt.image.BandedSampleModel;
import java.awt.image.BufferedImage;
import java.awt.image.DataBuffer;
import java.awt.image.DataBufferByte;
import java.awt.image.DataBufferInt;
import java.awt.image.Raster;
import java.awt.image.SampleModel;
import java.awt.image.SinglePixelPackedSampleModel;
import java.awt.image.WritableRaster;
import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.net.URL;
import java.nio.ByteBuffer;
import java.util.Hashtable;
import java.util.Vector;

import javax.imageio.IIOException;
import javax.imageio.stream.ImageInputStreamImpl;

import org.gdal.gdal.Dataset;
import org.gdal.gdal.Driver;
import org.gdal.gdal.GDALAsyncRasterIO;
import org.gdal.gdal.gdal;
import org.gdal.gdalconst.gdalconstConstants;



public class JPIPImageInputStream extends ImageInputStreamImpl
implements JPIPImageInputStreamInterface
{
	private int nThumbDataRead;
	private int nMainDataRead;

	private Dataset dataset;  // GDAL dataset
	private GDALAsyncRasterIO arioMain;
	private GDALAsyncRasterIO arioThumb;
	private ByteBuffer bufThumb = ByteBuffer.allocateDirect(Integer.parseInt(gdaljpip.getString("JPIPImageInputStream.bufferSize")));  // use views for the different data types
	private ByteBuffer bufMain = ByteBuffer.allocateDirect(Integer.parseInt(gdaljpip.getString("JPIPImageInputStream.bufferSize")));

	private boolean thumbWindowDone = false;
	private boolean mainWindowDone = false;

	private int width;	
	private int height;
	private int nComp;
	private int nLayers;
	private int nResLevels;
	private int bitDepth;

	private String request;

	private Rectangle rectMain = null;
	private Rectangle rectThumb = null;

	protected UUIDBox getUUIDBox() throws IIOException
	{
		UUIDBox box = null;
		// create a degenerate geotiff from the current information
		if ((dataset != null)
				&& (dataset.GetProjection() != null))
		{
			try {
				Driver gtiffDriver = gdal.GetDriverByName("GTiff"); //$NON-NLS-1$

				if (gtiffDriver == null)
					return null;

				File tempFile  = File.createTempFile("uuid", ".tif"); //$NON-NLS-1$ //$NON-NLS-2$
				String gtiff = tempFile.toString();
				Dataset dsGtiff = gtiffDriver.Create(gtiff,
						1, 1, 1, gdalconstConstants.GDT_Byte, new Vector());

				double[] transform = new double[6];
				dataset.GetGeoTransform(transform);
				dsGtiff.SetProjection(dataset.GetProjection());
				dsGtiff.SetGeoTransform(transform);
				dsGtiff.FlushCache();
				dsGtiff.delete();

				FileInputStream is = new FileInputStream(tempFile);
				int len = (int)tempFile.length();
				byte[] data = new byte[len];
				is.read(data);

				tempFile.delete();

				// add the uuid identifiers
				byte[] uuid = new byte[8];
				ByteArrayOutputStream bos = new ByteArrayOutputStream();
				DataOutputStream dos = new DataOutputStream(bos);
				dos.write(new byte[8]); // dummy jpip hdr
				dos.writeBytes("uuid"); //$NON-NLS-1$
				dos.writeInt(len);
				dos.write(data);
				dos.flush();

				box = new UUIDBox(bos.toByteArray());
				return box;
			} catch (IOException e) {
				throw new IIOException("Error creating UUID Box", e); //$NON-NLS-1$
			}



		}

		return box;
	}

	/**
	 * @param srcRegion
	 *            region requested (at 1:1)
	 * @param level
	 * 			discard level requested
	 * @param bands
	 *            band mapping
	 * @param quality
	 *            maximum quality layers
	 * @param isThumbnail
	 *            flag indicating if this is the thumbnail image
	 * @throws IIOException
	 */
	protected synchronized BufferedImage read(Rectangle srcRegion, int[] bands,
			int quality, int level, boolean isThumbnail) throws IIOException {

		if (isThumbnail)
			thumbWindowDone = false;
		else
			mainWindowDone = false;
		
		if (bands.length > 3)
			throw new IIOException("Maximum of 3 bands allowed"); //$NON-NLS-1$

		int codestream = 0;

		// xoff, yoff, xsize, ysize in raster space
		// buf_xsize, buf_ysize current desired screen dimensions
		// do byte buffers only for now
		int[] iNull = {0};
		Vector options = new Vector();
		options.add("LEVEL=" + level); //$NON-NLS-1$
		options.add("LAYERS=" + quality); //$NON-NLS-1$

		GDALAsyncRasterIO ario = null;
		Rectangle region = null;

		if (isThumbnail)
		{
			if ((rectThumb == null) || !rectThumb.equals(srcRegion))
			{
				rectThumb = srcRegion;
				if (arioThumb != null)
					dataset.EndAsyncRasterIO(arioThumb);

				options.add("PRIORITY=" + "0"); //$NON-NLS-1$ //$NON-NLS-2$
				arioThumb = dataset.BeginAsyncRasterIO_Direct(srcRegion.x, srcRegion.y,
						srcRegion.width, srcRegion.height,(int)Math.sqrt(bufThumb.capacity()), (int)Math.sqrt(bufThumb.capacity()), gdalconstConstants.GDT_Byte, bufThumb,
						bands, 0, 0, 0, options);
			}			
			ario = arioThumb;
			region = rectThumb;
		}
		else
		{	
			if ((rectMain == null) || !rectMain.equals(srcRegion))
			{
				rectMain = srcRegion;
				if (arioMain != null)
					dataset.EndAsyncRasterIO(arioMain);

				options.add("PRIORITY=" + "1"); //$NON-NLS-1$ //$NON-NLS-2$
				
				arioMain = dataset.BeginAsyncRasterIO_Direct(srcRegion.x, srcRegion.y,
						srcRegion.width, srcRegion.height,(int)Math.sqrt(bufMain.capacity()), (int)Math.sqrt(bufMain.capacity()), gdalconstConstants.GDT_Byte, bufMain,
						bands, 0, 0, 0, options);
			}

			ario = arioMain;
			region = rectMain;
		}

		int[] xoff = {0};
		int[] yoff = {0};
		int[] buf_xsize = {0};
		int[] buf_ysize = {0};
		BufferedImage img = null;

		try
		{

			ario.LockBuffer();

			int result = ario.GetNextUpdatedRegion(true, 0, xoff, yoff, buf_xsize, buf_ysize);
			
			if ((result == gdalconstConstants.GARIO_UPDATE) || (result == gdalconstConstants.GARIO_COMPLETE))
			{
				// check the buffer contents
				ByteBuffer buf;
				if (isThumbnail)
				{
					buf = bufThumb;
					nThumbDataRead += ario.getNDataRead();
				}
				else
				{
					buf = bufMain;
					nMainDataRead += ario.getNDataRead();
				}

				if ((buf_xsize[0] > 0)  && (buf_ysize[0] > 0))
				{
					buf.rewind();

					// type int32
					int pixels = buf_xsize[0] * buf_ysize[0];
					int[] ints = new int[pixels];
					buf.asIntBuffer().get(ints);

					DataBuffer imgBuffer = new DataBufferInt(ints, pixels);
					SampleModel sampleModel = new SinglePixelPackedSampleModel(
							DataBuffer.TYPE_INT, buf_xsize[0], buf_ysize[0], 
							buf_xsize[0], new int[]{0xff00, 0xff0000, 0xff000000});
					WritableRaster raster = Raster.createWritableRaster(sampleModel, imgBuffer, null);
					img = new BufferedImage(buf_xsize[0], buf_ysize[0], BufferedImage.TYPE_INT_RGB);
					img.setData(raster);
				}
				else
				{
					if (result == gdalconstConstants.GARIO_ERROR)
						throw new IIOException("Error : " + gdal.GetLastErrorNo() + " " + gdal.GetLastErrorMsg() ); //$NON-NLS-1$ //$NON-NLS-2$
					else if (result == gdalconstConstants.GARIO_PENDING)
						// getting a pending result
						throw new IIOException("RECEIVED UNEXPECTED IMAGE PENDING RESULT"); //$NON-NLS-1$
				}



				if ((result == gdalconstConstants.GARIO_COMPLETE))
				{
					if (isThumbnail)
						thumbWindowDone = true;
					else
						mainWindowDone = true;
				}
			}

		}
		finally
		{
			if (ario != null)
				ario.UnlockBuffer();
		}


		return img;

	}

	/**
	 * @param stream
	 * @param cacheDir
	 * @throws IOException
	 */
	public JPIPImageInputStream(URL url, File cacheDir) throws IOException {
		request = url.toString().replace("http", "jpip"); //$NON-NLS-1$ //$NON-NLS-2$
		dataset = gdal.Open(request, gdalconstConstants.GA_ReadOnly);
		if (dataset == null) {
			System.err.println("GDALOpen failed - " + gdal.GetLastErrorNo()); //$NON-NLS-1$
			System.err.println(gdal.GetLastErrorMsg());
		}

		this.width = dataset.getRasterXSize();
		this.height = dataset.getRasterYSize();

		Hashtable meta = dataset.GetMetadata_Dict("JPIP"); //$NON-NLS-1$
		this.nComp = Integer.parseInt((String)meta.get("JPIP_NCOMPS")); //$NON-NLS-1$
		this.nLayers = Integer.parseInt((String)meta.get("JPIP_NQUALITYLAYERS")); //$NON-NLS-1$
		this.nResLevels = Integer.parseInt((String)meta.get("JPIP_NRESOLUTIONLEVELS")); //$NON-NLS-1$
		this.bitDepth = Integer.parseInt((String)meta.get("JPIP_SPRECISION")); //$NON-NLS-1$
	}

	public int read() throws IOException {
		throw new IOException("Not Implemented!"); //$NON-NLS-1$
	}

	public int read(byte[] b, int off, int len) throws IOException {
		throw new IOException("Not Implemented!"); //$NON-NLS-1$
	}


	public long length() {
		return -1;
	}

	public int getNDataRead(boolean isThumbnail)
	{
		if (isThumbnail)
		{
			return nThumbDataRead;
		}
		else
		{
			return nMainDataRead;
		}
	}

	public void abort() throws IIOException {
		if (dataset != null)
		{
			if (arioMain != null)
			{
				dataset.EndAsyncRasterIO(arioMain);
				arioMain = null;
			}

			if (arioThumb != null)
			{
				dataset.EndAsyncRasterIO(arioThumb);
				arioThumb = null;
			}
		}
	}

	public boolean isWindowDone(boolean isThumbnail) {
		if (isThumbnail)
			return thumbWindowDone;
		else
			return mainWindowDone;
	}

	public Rectangle getRegion(boolean isThumbnail)
	{
		if (dataset != null)
		{
			if (isThumbnail)
				return new Rectangle(arioThumb.getXOff(), arioThumb.getYOff(),
						arioThumb.getXSize(), arioThumb.getYSize());
			else
				return new Rectangle(arioMain.getXOff(), arioMain.getYOff(),
						arioMain.getXSize(), arioMain.getYSize());
		}
		else 
			return null;
	}

	public int getBitDepth() {
		// TODO Auto-generated method stub
		return 0;
	}

	public int getDecompositionLevels() {
		return nResLevels;
	}

	public int getNComp() {
		return nComp;
	}

	public int getNLayers() {
		return nLayers;
	}

	public int getXTileSize() {
		return width;
	}

	public int getYTileSize() {
		return height;
	}

	public int getXSize() {
		return width;
	}

	public int getYSize() {
		return height;
	}

}
