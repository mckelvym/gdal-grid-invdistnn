
/*
COPYRIGHT 1995-2011 ESRI

TRADE SECRETS: ESRI PROPRIETARY AND CONFIDENTIAL
Unpublished material - all rights reserved under the
Copyright Laws of the United States.

For additional information, contact:
Environmental Systems Research Institute, Inc.
Attn: Contracts Dept
380 New York Street
Redlands, California, USA 92373

email: contracts@esri.com
*/

#pragma once

// ---- includes ------------------------------------------------------------ ;

#include "Image.h"
#include <vector>
#include <cassert>
// #include <iostream>
// #include <fstream>
#include "Defines.h"

// -------------------------------------------------------------------------- ;

// ---- related classes ----------------------------------------------------- ;

class CntZ
{
public:
  float cnt, z;
  bool operator == (const CntZ& cz) const    { return cnt == cz.cnt && z == cz.z; }
  bool operator != (const CntZ& cz) const    { return cnt != cz.cnt || z != cz.z; }
  void operator += (const CntZ& cz)          { cnt += cz.cnt;  z += cz.z; }
};

class CntZXY : public CntZ
{
public:
  float x, y;
};

// -------------------------------------------------------------------------- ;

/**	Template Image
 *	
 */

template< class Element >
class TImage : public Image
{
public:
  TImage();
  TImage(const TImage& tImg);
  virtual ~TImage();

  /// assignment
  virtual TImage& operator=(const TImage& tImg);

  bool resize(long width, long height);
  bool crop(long row0, long numRows, long col0, long numCols, TImage& dstImg) const;
  bool addFrame(long top, long bottom, long left, long right, Element value, TImage& dstImg) const;
  void fill(Element value);
  void fillRow(long row, Element val);
  void fillCol(long col, Element val);
  void flipUp();
  virtual void clear();

  /// get data
  Element getPixel(long row, long col) const;
  const Element& operator() (long row, long col) const;
  const Element* getData() const;
  long getSizeInBytes() const;
  bool getColumn(long col, std::vector< Element >& colVec) const;
  bool getRange(Element& min, Element& max) const;
  bool getRangeFromValidData(Element& min, Element& max, Element invalidValue) const;

  /// set data
  void setPixel(long row, long col, Element element);
  Element& operator() (long row, long col);
  Element* getData();
  bool setColumn(long col, const std::vector< Element >& colVec);

  /// compare
  bool operator == (const Image& img) const;
  bool operator != (const Image& img) const	{ return !operator==(img); };

  /// binary file IO (internal, no unicode)
//  virtual bool write(std::ofstream& fout) const;
//  virtual bool read (std::ifstream& fin);
//  virtual bool write(const std::string& fn) const;
//  virtual bool read (const std::string& fn);


protected:
  Element* data_;

};

// -------------------------------------------------------------------------- ;
// -------------------------------------------------------------------------- ;

template< class Element >
inline Element TImage< Element >::getPixel(long i, long j) const
{
  assert(isInside(i, j));
  return data_[i * width_ + j];
}

template< class Element >
inline const Element& TImage< Element >::operator () (long i, long j) const
{
  assert(isInside(i, j));
  return data_[i * width_ + j];
}

template< class Element >
inline const Element* TImage< Element >::getData() const
{
  return data_;
}

template< class Element >
inline long TImage< Element >::getSizeInBytes() const
{
  return (getSize() * sizeof(Element) + sizeof(*this));
}

template< class Element >
inline void TImage< Element >::setPixel(long i, long j, Element element)
{
  assert(isInside(i, j));
  data_[i * width_ + j] = element;
}

template< class Element >
inline Element& TImage< Element >::operator () (long i, long j)
{
  assert(isInside(i, j));
  return data_[i * width_ + j];
}

template< class Element >
inline Element* TImage< Element >::getData()
{
  return data_;
}

// -------------------------------------------------------------------------- ;
// -------------------------------------------------------------------------- ;

template< class Element >
TImage< Element >::TImage()
{
  data_ = 0;
};

template< class Element >
TImage< Element >::TImage(const TImage& tImg)
{
  data_ = 0;
  *this = tImg;
};

template< class Element >
TImage< Element >::~TImage()
{
  clear();
};

// -------------------------------------------------------------------------- ;

template< class Element >
bool TImage< Element >::resize(long width, long height)
{
  if (width <= 0 || height <= 0)
    return false;

  if (width == width_ && height == height_)
    return true;

  free(data_);
  width_ = 0;
  height_ = 0;

  data_ = (Element*) malloc(width * height * sizeof(Element));
  if (!data_)
  {
//    std::cout << "TImage::resize(...) failed" << std::endl;
    return false;
  }
  width_ = width;
  height_ = height;
  
  return true;
}

// -------------------------------------------------------------------------- ;

template< class Element >
bool TImage< Element >::crop(long row0, long numRows, long col0, long numCols, TImage& dstImg) const
{
  if (&dstImg == this)
  {
//  return Img_getErrorId(Img_TImage_Crop_Failed, "TImage::crop(...): dst and src image must be different");
    return false;
  }

  long row1 = row0 + numRows;		// exclusive
  long col1 = col0 + numCols;
  
  row0 = row0 < 0 ? 0 : row0;
  col0 = col0 < 0 ? 0 : col0;
  row1 = row1 > height_ ? height_ : row1;
  col1 = col1 > width_  ? width_  : col1;
  
  long dstWidth  = col1 - col0;
  long dstHeight = row1 - row0;
  
  if (numRows <= 0 || numCols <= 0 || dstWidth <= 0 || dstHeight <= 0)
  {
//  return Img_getErrorId(Img_TImage_Crop_Failed, "TImage::crop(...): rectangle has no overlap with image");
    return false;
  }

  if (!dstImg.resize(dstWidth, dstHeight))
    return false;

  Element* dstPtr = dstImg.getData();

  for (long i = 0; i < dstHeight; i++)
  {
    const Element* srcPtr = data_ + (row0 + i) * width_ + col0;
    long k = dstWidth;
    while (k--) *dstPtr++ = *srcPtr++;
  }

  return true;
}

// -------------------------------------------------------------------------- ;

template< class Element >
bool TImage< Element >::addFrame(long top, long bottom, long left, long right, Element value, TImage& dstImg) const
{
  if (&dstImg == this)
  {
//  return Img_getErrorId(Img_TImage_AddFrame_Failed, "TImage::addFrame(...): dst and src image must be different");
    return false;
  }

  top    =    top < 0 ? 0 : top;
  bottom = bottom < 0 ? 0 : bottom;
  left   =   left < 0 ? 0 : left;
  right  =  right < 0 ? 0 : right;

  long dstWidth = width_ + left + right;
  long dstHeight = height_ + top + bottom;
  
//  long warningWidth = 20000;
//  if (dstWidth > warningWidth || dstHeight > warningWidth)
//  {
//    printf("Warning in TImage< Element >::addFrame(...): new image dimensions are larger than ");
//    printf("%d: width = %d, height = %d\n", warningWidth, dstWidth, dstHeight);
//  }

  if (!dstImg.resize(dstWidth, dstHeight))
    return false;

  dstImg.fill(value);
  
  const Element* srcPtr = getData();

  for (long i = 0; i < height_; i++)
  {
    Element* dstPtr = dstImg.getData() + (top + i) * dstWidth + left;
    long k = width_;
    while (k--) *dstPtr++ = *srcPtr++;
  }

  return true;
}

// -------------------------------------------------------------------------- ;

template< class Element >
void TImage< Element >::fill(Element val)
{
  Element* ptr = getData();
  long size = getSize();

  long i = size >> 2;
  while (i--)
  {
    *(ptr    ) = val;
    *(ptr + 1) = val;
    *(ptr + 2) = val;
    *(ptr + 3) = val;
    ptr += 4;
  }
  i = size & 3;
  while (i--) *ptr++ = val;
}

// -------------------------------------------------------------------------- ;

template< class Element >
void TImage< Element >::fillRow(long row, Element val)
{
  if (row < 0 || row >= height_)
    return;

  Element* ptr = getData() + row * width_;
  long size = width_;

  long i = size >> 2;
  while (i--)
  {
    *(ptr    ) = val;
    *(ptr + 1) = val;
    *(ptr + 2) = val;
    *(ptr + 3) = val;
    ptr += 4;
  }
  i = size & 3;
  while (i--) *ptr++ = val;
}

// -------------------------------------------------------------------------- ;

template< class Element >
void TImage< Element >::fillCol(long col, Element val)
{
  if (col < 0 || col >= width_)
    return;

  Element* ptr = getData() + col;
  long i = height_;

  while (i--)
  {
    *ptr = val;
    ptr += width_;
  }
}

// -------------------------------------------------------------------------- ;

template< class Element >
void TImage< Element >::flipUp()
{
  std::vector< Element > buffer(width_);
  Element* buf = &buffer[0];

  long h2 = height_ >> 1;
  for (long i = 0; i < h2; i++)
  {
    Element* row0 = &data_[i * width_];
    Element* row1 = &data_[(height_ - 1 - i) * width_];

    Element* p0 = row0;
    Element* pBuf = buf;
    long j = width_;
    while (j--) *pBuf++ = *p0++;

    Element* p1 = row1;
    p0 = row0;
    j = width_;
    while (j--) *p0++ = *p1++;

    pBuf = buf;
    p1 = row1;
    j = width_;
    while (j--) *p1++ = *pBuf++;
  }
}

// -------------------------------------------------------------------------- ;

template< class Element >
void TImage< Element >::clear()
{
  free(data_);
  data_ = 0;
  width_ = 0;
  height_ = 0;
}

// -------------------------------------------------------------------------- ;

template< class Element >
bool TImage< Element >::getColumn(long col, std::vector< Element >& colVec) const
{
  assert(width_ > 0 && height_ > 0);
  assert(col >= 0 && col < width_);

  colVec.resize(height_);

  for (long i = 0; i < height_; i++)
    colVec[i] = data_[i * width_ + col];

  return true;
}

// -------------------------------------------------------------------------- ;

template< class Element >
bool TImage< Element >::getRange(Element& min, Element& max) const
{
  const Element* ptr = getData();
  long size = getSize();
  if (size == 0 || ptr == 0)
    return false;

  Element minL = *ptr;
  Element maxL = *ptr;
  long cnt = size;
  while (cnt--)
  {
    Element val = *ptr++;
    minL = val < minL ? val : minL;
    maxL = val > maxL ? val : maxL;
  }
  
  min = minL;
  max = maxL;
  
  return true;
}

// -------------------------------------------------------------------------- ;

template< class Element >
bool TImage< Element >::getRangeFromValidData(Element& min, Element& max, Element invalidValue) const
{
  const Element* ptr = getData();
  long size = getSize();
  if (size == 0 || ptr == 0)
    return false;

  Element minL = *ptr;
  Element maxL = *ptr;
  bool init = false;
  long cnt = size;
  while (cnt--)
  {
    Element val = *ptr++;
    if (val != invalidValue)
    {
      if (!init)
      {
        minL = val;
        maxL = val;
        init = true;
      }
      else
      {
        minL = val < minL ? val : minL;
        maxL = val > maxL ? val : maxL;
      }
    }
  }
  
  min = minL;
  max = maxL;
  
  return true;
}

// -------------------------------------------------------------------------- ;

template< class Element >
bool TImage< Element >::setColumn(long col, const std::vector< Element >& colVec)
{
  assert(width_ > 0 && height_ > 0);
  assert(col >= 0 && col < width_);
  assert(colVec.size() == height_);

  for (long i = 0; i < height_; i++)
    data_[i * width_ + col] = colVec[i];

  return true;
}

// -------------------------------------------------------------------------- ;

template< class Element >
TImage< Element >& TImage< Element >::operator = (const TImage& tImg)
{
  // allow copying image to itself
  if (this == &tImg) return *this;

  // only for images of the same type!
  // conversions are implemented in the derived classes
  assert(type_ == tImg.getType());

  if (!resize(tImg.getWidth(), tImg.getHeight()))
    return *this;    // return empty image if resize fails

  memcpy(getData(), tImg.getData(), getSize() * sizeof(Element));

  Image::operator=(tImg);
  
  return *this;
}

// -------------------------------------------------------------------------- ;

template< class Element >
bool TImage< Element >::operator == (const Image& img) const
{
  if (! Image::operator == (img)) return false;

  const Element* ptr0 = getData();
  const Element* ptr1 = ((const TImage&)img).getData();
  long cnt = getSize();
  while (cnt--)
    if (*ptr0++ != *ptr1++)
      return false;

  return true;
}

// -------------------------------------------------------------------------- ;
// -------------------------------------------------------------------------- ;
/*
template< class Element >
bool TImage< Element >::write(std::ofstream& fout) const
{
  if (!fout.good())
    return false;

  fout.write(this->getTypeString().c_str(), this->getTypeString().length());
  int version = 4;

  fout.write((char*)&version, sizeof(int));
  fout.write((char*)&type_,   sizeof(int));
  fout.write((char*)&height_, sizeof(long));
  fout.write((char*)&width_,  sizeof(long));

  if (fout.fail())
    return false;

  fout.write((char*)getData(), getSize() * sizeof(Element));
  return !fout.fail();
}

// -------------------------------------------------------------------------- ;

template< class Element >
bool TImage< Element >::read(std::ifstream& fin)
{
  if (!fin.good())
    return false;

  size_t len = this->getTypeString().length();
  std::string typeString(len, '0');
  fin.read((char*)&typeString[0], len);

  if (typeString != this->getTypeString())
  {
    std::cout << "TImage::read( ... ): image is of type " << typeString << ", not of type " << this->getTypeString() << std::endl;
    return false;
  }

  int version = 0, type = 0;
  long width = 0, height = 0;

  fin.read((char*)&version, sizeof(int));
  fin.read((char*)&type,    sizeof(int));
  fin.read((char*)&height,  sizeof(long));
  fin.read((char*)&width,   sizeof(long));

  if (fin.fail())
    return false;

  if (type != type_)
  {
    std::cout << "TImage::read( ... ): image type mismatch" << std::endl;
    return false;
  }

  if (width > 20000 || height > 20000)
  {
    std::cout << "TImage::read( ... ): image too large, width or height > 20,000" << std::endl;
    return false;
  }

  if (!resize(width, height))
    return false;

  fin.read((char*)getData(), getSize() * sizeof(Element));
  return !fin.fail();
}

// -------------------------------------------------------------------------- ;

template< class Element >
bool TImage< Element >::write(const std::string& fn) const
{
  std::ofstream fout(fn.c_str(), std::ios::out | std::ios::binary);
  if (!fout.good())
  {
    std::cout << "TImage::write( ... ): cannot open file \"" << fn << "\"" << std::endl;
    return false;
  }
  return write(fout);
}

// -------------------------------------------------------------------------- ;

template< class Element >
bool TImage< Element >::read(const std::string& fn)
{
  std::ifstream fin(fn.c_str(), std::ios::in | std::ios::binary);
  if (!fin.good())
  {
    std::cout << "TImage::read( ... ): cannot open file \"" << fn << "\"" << std::endl;
    return false;
  }
  return read(fin);
}


// -------------------------------------------------------------------------- ;
// -------------------------------------------------------------------------- ;

class CntZXYImage : public TImage< CntZXY >
{
public:
  CntZXYImage()                        { type_ = CNT_ZXY; }
  virtual ~CntZXYImage()               {};
  std::string getTypeString() const    { return "CntZXYImage "; }
};

// -------------------------------------------------------------------------- ;

//class ComplexImage : public TImage< std::complex< float > >
//{
//public:
//  ComplexImage()                       { type_ = COMPLEX; }
//  virtual ~ComplexImage()              {};
//  std::string getTypeString() const    { return "ComplexImage "; }
//};

// -------------------------------------------------------------------------- ;

class DoubleImage : public TImage< double >
{
public:
  DoubleImage()                        { type_ = DOUBLE; }
  virtual ~DoubleImage()               {};
  std::string getTypeString() const    { return "DoubleImage "; }
};

// -------------------------------------------------------------------------- ;

class FloatImage : public TImage< float >
{
public:
  FloatImage()                         { type_ = FLOAT; }
  virtual ~FloatImage()                {};
  std::string getTypeString() const    { return "FloatImage "; }
};

// -------------------------------------------------------------------------- ;

class LongImage : public TImage< long >
{
public:
  LongImage()                          { type_ = LONG; }
  virtual ~LongImage()                 {};
  std::string getTypeString() const    { return "LongImage "; }
};

// -------------------------------------------------------------------------- ;

class ShortImage : public TImage< short >
{
public:
  ShortImage()                         { type_ = SHORT; }
  virtual ~ShortImage()                {};
  std::string getTypeString() const    { return "ShortImage "; }
};

// -------------------------------------------------------------------------- ;

class ByteImage : public TImage< Byte >
{
public:
  ByteImage()                          { type_ = BYTE; }
  virtual ~ByteImage()                 {};
  std::string getTypeString() const    { return "ByteImage "; }
};

// -------------------------------------------------------------------------- ;

*/
