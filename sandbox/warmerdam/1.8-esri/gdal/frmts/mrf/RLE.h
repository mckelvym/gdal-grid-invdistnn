/*
COPYRIGHT 1995-2010 ESRI

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

#include "Defines.h"

// ---- related classes ----------------------------------------------------- ;

// -------------------------------------------------------------------------- ;

/** RLE:
 *  run length encode a byte array
 *
 *  best case resize factor (all bytes are the same):
 *    (n + 1) * 3 / 32767 + 2  ~= 0.00009
 *
 *  worst case resize factor (no stretch of same bytes):
 *    n + (n + 1) * 2 / 32767 + 2  ~= 1.00006
 */

class RLE
{
public:
  RLE() : m_minNumEven(5) {};
  virtual ~RLE() {};

  size_t computeNumBytesRLE(const Byte* arr, size_t numBytes) const;

  // when done, call
  // delete[] *arrRLE;
  bool compress(const Byte* arr, size_t numBytes,
                Byte** arrRLE, size_t& numBytesRLE, bool verify = false) const;

  // when done, call
  // delete[] *arr;
  bool decompress(const Byte* arrRLE, Byte** arr, size_t& numBytes) const;

  // arr already allocated, just fill
  bool decompress(const Byte* arrRLE, Byte* arr) const;

protected:
  int m_minNumEven;

  void writeCount(short cnt, Byte** ppCnt, Byte** ppDst) const;
  short readCount(const Byte** ppCnt) const;

};

// -------------------------------------------------------------------------- ;

