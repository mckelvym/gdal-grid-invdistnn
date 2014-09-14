
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

// -------------------------------------------------------------------------- ;

// ---- related classes ----------------------------------------------------- ;

// -------------------------------------------------------------------------- ;

/** Bit stuffer, for writing unsigned int arrays compressed lossless
 *
 */

class BitStuffer
{
public:
  BitStuffer()           {}
  virtual ~BitStuffer()  {}

  // these 2 do not allocate memory. Byte ptr is moved like a file pointer.
  bool write(Byte** ppByte, const std::vector<unsigned long>& dataVec) const;
  bool read( Byte** ppByte, std::vector<unsigned long>& dataVec) const;

  static unsigned long computeNumBytesNeeded(unsigned long numElem, unsigned long maxElem);
  static unsigned long numExtraBytesToAllocate()  { return 3; }

protected:
  unsigned long findMax(const std::vector<unsigned long>& dataVec) const;

  // numBytes = 1, 2, or 4
  bool writeULong(Byte** ppByte, unsigned long k, int numBytes) const;
  bool readULong( Byte** ppByte, unsigned long& k, int numBytes) const;

  static int numBytesULong(unsigned long k)  { return (k < 256) ? 1 : (k < (1 << 16)) ? 2 : 4; }
  static unsigned long numTailBytesNotNeeded(unsigned long numElem, int numBits);
};

// -------------------------------------------------------------------------- ;

