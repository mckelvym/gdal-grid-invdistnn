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


import javax.imageio.ImageReadParam;

public class J2KImageReadParam extends ImageReadParam {
	private int quality = -1;
	private int refresh = 500;
	private float[] draMin = null;
	private float[] draMax = null;
	private double sharpGain = 1.0;

	private boolean thumbnail = false;

	public J2KImageReadParam() {
		super();
	}

	public int getQuality() {
		return quality;
	}

	public void setQuality(int quality) {
		this.quality = quality;
	}

	public int getRefresh() {
		return refresh;
	}

	public void setRefresh(int refresh) {
		this.refresh = refresh;
	}

	public boolean isThumbnail() {
		return thumbnail;
	}

	public void setThumbnail(boolean thumbnail) {
		this.thumbnail = thumbnail;
	}
	
	public void setDraMin(float[] draMin)
	{
		this.draMin = (float[])draMin.clone();
	}
	
	public float[] getDraMin()
	{
		return draMin;
	}
	
	public void setDraMax(float[] draMax)
	{
		this.draMax = (float[])draMax.clone();
	}
	
	public float[] getDraMax()
	{
		return draMax;
	}
	
	public double getSharpGain() {
		return sharpGain;
	}

	public void setSharpGain(double sharpGain) {
		this.sharpGain = sharpGain;
	}
}
