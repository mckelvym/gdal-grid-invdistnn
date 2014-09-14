
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

#include <string>
#include "Defines.h"

// -------------------------------------------------------------------------- ;

// ---- related classes ----------------------------------------------------- ;

// -------------------------------------------------------------------------- ;

/**	Base class for all image classes
 *	
 */

class Image
{
public:

  virtual ~Image()    {};

  enum Type
  {
    BYTE,
    RGB,
    SHORT,
    LONG,
    FLOAT,
    DOUBLE,
    COMPLEX,
    POINT3F,
    CNT_Z,
    CNT_ZXY,
    Last_Type_
  };

  /// access
  bool isByteImage() const       { return type_ == BYTE; }
  bool isRGBImage() const        { return type_ == RGB; }
  bool isShortImage() const      { return type_ == SHORT; }
  bool isLongImage() const       { return type_ == LONG; }
  bool isFloatImage() const      { return type_ == FLOAT; }
  bool isDoubleImage() const     { return type_ == DOUBLE; }
  bool isComplexImage() const    { return type_ == COMPLEX; }
  bool isPoint3fImage() const    { return type_ == POINT3F; }
  bool isCntZImage() const       { return type_ == CNT_Z; }
  bool isCntZXYImage() const     { return type_ == CNT_ZXY; }
  Type getType() const           { return type_; }
  long getWidth() const          { return width_; }
  long getHeight() const         { return height_; }
  long getSize() const           { return width_ * height_; }

  bool isInside(long row, long col) const;

  virtual std::string getTypeString() const = 0;
  virtual long getSizeInBytes() const = 0;


protected:

  Image() : type_(Last_Type_), width_(0), height_(0)   {};

  /// compare
  bool operator == (const Image& img) const;
  bool operator != (const Image& img) const	{ return !operator==(img); };

  Type type_;
  long width_, height_;

};

// -------------------------------------------------------------------------- ;

inline bool Image::isInside(long row, long col) const
{
  return row >= 0 && row < height_ && col >= 0 && col < width_;
}

// -------------------------------------------------------------------------- ;

inline bool Image::operator == (const Image& img) const
{
  return type_ == img.type_ && width_ == img.width_ && height_ == img.height_;
}

// -------------------------------------------------------------------------- ;

