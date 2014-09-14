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

//
// BitMask.cpp
//

#include "BitMask.h"
#include "RLE.h"

//#define USE_RLE

// -------------------------------------------------------------------------- ;

BitMask::BitMask()
: m_pBits(0),
  m_nCols(0),
  m_nRows(0),
  m_owner(0)
{
}

// -------------------------------------------------------------------------- ;

BitMask::BitMask(long nCols, long nRows)
: m_pBits(0),
  m_owner(0)
{
  SetSize(nCols, nRows);
}

// -------------------------------------------------------------------------- ;

BitMask::BitMask(long nCols, long nRows, Byte* pBits, bool owner)
: m_pBits(pBits),
  m_nCols(nCols),
  m_nRows(nRows),
  m_owner(owner)
{
}

// -------------------------------------------------------------------------- ;

//BitMask::BitMask(LPCTSTR file)
//: m_pBits(0),
//  m_nCols(0),
//  m_nRows(0),
//  m_owner(0)
//{
//  if (::FileExists(file))
//  {
//    File f(file);
//    if (f.Open())
//    {
//      WORD version[2];
//      if (f.Read(version, 4) == 4)
//      {
//        long nCols;
//        long nRows;
//
//  #ifdef USE_RLE
//        long numBytesRLE;
//        if (f.Read(&nCols, 4) == 4
//        &&  f.Read(&nRows, 4) == 4
//        &&  f.Read(&numBytesRLE, 4) == 4)
//        {
//          if (version[0] != 0x10)
//          {
//            SwapShort((short*)&version[0], 2);
//            SwapInt(&nCols, 1);
//            SwapInt(&nRows, 1);
//            SwapInt(&numBytesRLE, 1);
//          }
//
//          // check version
//          if (version[0] == 0x10 && version[1] <= 1)
//          {
//            BYTE* arrRLE = new BYTE[numBytesRLE];
//            if (f.Read(arrRLE, numBytesRLE) == numBytesRLE)
//            {
//              SetSize(nCols, nRows);
//              RLE rle;
//              if (!rle.decompress(arrRLE, m_pBits))
//                Clear();
//            }
//            delete[] arrRLE;
//          }
//        }
//  #else
//        if (f.Read(&nCols, 4) == 4
//        &&  f.Read(&nRows, 4) == 4)
//        {
//          if (version[0] != 0x10)
//          {
//            SwapShort((short*)&version[0], 2);
//            SwapInt(&nCols, 1);
//            SwapInt(&nRows, 1);
//          }
//          SetSize(nCols, nRows);
//          f.Read(m_pBits, Size());
//        }
//  #endif
//      }
//
//      f.Close();
//    }
//  }
//}

// -------------------------------------------------------------------------- ;

BitMask::BitMask(const BitMask& copy)
: m_pBits(0),
  m_owner(0)
{
  SetSize(copy.m_nCols, copy.m_nRows);
  if (copy.m_pBits)
    //::CopyMemory(m_pBits, copy.m_pBits, Size());
    memcpy(m_pBits, copy.m_pBits, Size());
}

// -------------------------------------------------------------------------- ;

//BitMask::BitMask(IPixelBlock* pBlock, long iBandIndex)
//: m_pBits(0),
//  m_nCols(0),
//  m_nRows(0),
//  m_owner(0)
//{
//  if (pBlock)
//  {
//    long nBands;
//    pBlock->get_Planes(&nBands);
//    if (iBandIndex >= 0 && iBandIndex < nBands)
//    {
//      pBlock->get_Width(&m_nCols);
//      pBlock->get_Height(&m_nRows);
//      ASSERT(m_nCols > 0 && m_nRows > 0);
//      if (m_nCols > 0 && m_nRows > 0)
//      {
//        Variant m;
//        IPixelBlock3Ptr ipBlock(pBlock);
//        ipBlock->get_NoDataMaskByRef(iBandIndex, &m);
//        ASSERT(m.vt != VT_EMPTY);
//        if (m.vt != VT_EMPTY)
//        {
//          m_pBits = (BYTE*)((*m.pparray)->pvData);
//          ASSERT(m_pBits);
//        }
//      }
//    }
//  }
//}

// -------------------------------------------------------------------------- ;

bool BitMask::SetSize(long nCols, long nRows)
{
  if (nCols != m_nCols || nRows != m_nRows || !m_owner)
  {
    Clear();
    m_pBits = new Byte[(nCols * nRows + 7) >> 3];
    m_nCols = nCols;
    m_nRows = nRows;
    m_owner = true;
    return true;
  }
  return false;
}

// -------------------------------------------------------------------------- ;

long BitMask::CountValidBits() const
{
  const Byte numBitsHB[16] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};
  const Byte* ptr = m_pBits;
  long sum = 0;
  long i = Size();
  while (i--)
  {
    sum += numBitsHB[*ptr & 15] + numBitsHB[*ptr >> 4];
    ptr++;
  }

  // subtract undefined bits potentially contained in the last byte
  for (long k = m_nCols * m_nRows; k < Size() * 8; k++)
    if (IsValid(k))
      sum--;

  return sum;
}

// -------------------------------------------------------------------------- ;

void BitMask::ClipFrameToMask(long& x, long& y, long& w, long& h) const
{
  x = max(0, x);
  y = max(0, y);
  x = min(x, m_nCols);    // then w = 0
  y = min(y, m_nRows);    // then h = 0
  w = min(w, m_nCols - x);
  h = min(h, m_nRows - y);
}

// ----------------------------------------------------------------------------
//
// Remark about the following 3 functions Copy(), Intersect(), Union():
//
//   As the positions and sizes of both src and dst rectangles can take any
//   value, the corresponding mask bytes are in general not aligned. Therefore
//   the work must be done bit by bit, in general.
//
//   If it turns out that this is not fast enough, we can change the 
//   representation from a 1D byte array to a 2D byte image with some extra
//   bits at the end of each row, and all rows byte aligned. Then 
//   corresponding bytes from src and dst mask would still not be aligned,
//   but their relative bit shift would become a const. Bitwise operation
//   can then be implemented as half byte operations, offering a speedup of
//   up to 4x. 
//   However, this would have a number of side effects, such as increased
//   complexity, and access to bits would always be 2D (and a little slower).
//   Also we would have to create and maintain two versions of the BitMask
//   class, because we also use it as an interface to the existing 1D bit mask
//   arrays. 
//
// ----------------------------------------------------------------------------

void BitMask::Copy(const BitMask& otherBits, long x0, long y0, long x1, long y1, long xsize, long ysize)
{
  if (xsize == 0)
    xsize = otherBits.m_nCols;
  if (ysize == 0)
    ysize = otherBits.m_nRows;

  otherBits.ClipFrameToMask(x1, y1, xsize, ysize);
  ClipFrameToMask(x0, y0, xsize, ysize);

  if (x0 % 8 == 0 && m_nCols % 8 == 0 && x1 % 8 == 0 && xsize % 8 == 0 && otherBits.m_nCols % 8 == 0)
  {
    for (long i = 0; i < ysize; ++i, ++y0, ++y1)
    {
      memcpy(m_pBits + (y0 * m_nCols + x0) / 8,
             otherBits.m_pBits + (y1 * otherBits.m_nCols + x1) / 8,
             xsize / 8);
    }
  }
  else
  {
    for (long i = 0; i < ysize; ++i, ++y0, ++y1)
    {
      long k = y0 * m_nCols + x0;
      long m = y1 * otherBits.m_nCols + x1;
      for (long j = 0; j < xsize; ++j, ++k, ++m)
      {
        if (otherBits.IsValid(m))
          SetValid(k);
        else
          SetInvalid(k);
      }
    }
  }
}

// -------------------------------------------------------------------------- ;

void BitMask::Intersect(const BitMask& otherBits, long x0, long y0, long x1, long y1, long xsize, long ysize)
{
  if (xsize == 0)
    xsize = otherBits.m_nCols;
  if (ysize == 0)
    ysize = otherBits.m_nRows;

  otherBits.ClipFrameToMask(x1, y1, xsize, ysize);
  ClipFrameToMask(x0, y0, xsize, ysize);

  if (x0 % 8 == 0 && m_nCols % 8 == 0 && x1 % 8 == 0 && xsize % 8 == 0 && otherBits.m_nCols % 8 == 0)
  {
    for (long i = 0; i < ysize; ++i, ++y0, ++y1)
    {
      long k = (y0 * m_nCols + x0) >> 3;
      long m = (y1 * otherBits.m_nCols + x1) >> 3;
      for (long j = 0; j < (xsize >> 3); ++j, ++k, ++m)
        m_pBits[k] &= otherBits.m_pBits[m];
    }
  }
  else
  {
    for (long i = 0; i < ysize; ++i, ++y0, ++y1)
    {
      long k = y0 * m_nCols + x0;
      long m = y1 * otherBits.m_nCols + x1;
      for (long j = 0; j < xsize; ++j, ++k, ++m)
        if (!otherBits.IsValid(m))
          SetInvalid(k);
    }
  }
}

// -------------------------------------------------------------------------- ;

void BitMask::Union(const BitMask& otherBits, long x0, long y0, long x1, long y1, long xsize, long ysize)
{
  if (xsize == 0)
    xsize = otherBits.m_nCols;
  if (ysize == 0)
    ysize = otherBits.m_nRows;

  otherBits.ClipFrameToMask(x1, y1, xsize, ysize);
  ClipFrameToMask(x0, y0, xsize, ysize);

  if (x0 % 8 == 0 && m_nCols % 8 == 0 && x1 % 8 == 0 && xsize % 8 == 0 && otherBits.m_nCols % 8 == 0)
  {
    for (long i = 0; i < ysize; ++i, ++y0, ++y1)
    {
      long k = (y0 * m_nCols + x0) >> 3;
      long m = (y1 * otherBits.m_nCols + x1) >> 3;
      for (long j = 0; j < (xsize >> 3); ++j, ++k, ++m)
        m_pBits[k] |= otherBits.m_pBits[m];
    }
  }
  else
  {
    for (long i = 0; i < ysize; ++i, ++y0, ++y1)
    {
      long k = y0 * m_nCols + x0;
      long m = y1 * otherBits.m_nCols + x1;
      for (long j = 0; j < xsize; ++j, ++k, ++m)
        if (otherBits.IsValid(m))
          SetValid(k);
    }
  }
}

// -------------------------------------------------------------------------- ;

//bool BitMask::SaveTo(LPCTSTR file) const
//{
//  File f(file);
//  f.Create();
//  // write version
//  WORD VERSION[] = {0x10, 1};
//  f.Write(&VERSION[0], 4);
//  long nCols = m_nCols;    // because of const
//  long nRows = m_nRows;
//  f.Write(&nCols, 4);
//  f.Write(&nRows, 4);
//  bool rv = true;
//
//#ifdef USE_RLE
//  RLE rle;
//  BYTE* arrRLE = 0;
//  size_t numBytesRLE = 0;
//
//#if defined(_DEBUG)
//  rv = rle.compress(m_pBits, Size(), &arrRLE, numBytesRLE, true);    // verify is ON
//#else
//  rv = rle.compress(m_pBits, Size(), &arrRLE, numBytesRLE, false);    // verify is OFF
//#endif 
//
//  if (rv)
//  {
//    assert(numBytesRLE < MAXLONG);
//    long n = (long)numBytesRLE;
//    f.Write(&n, 4);
//    f.Write(arrRLE, int(numBytesRLE));
//  }
//  delete[] arrRLE;
//#else
//  f.Write(m_pBits, Size());
//#endif
//
//  f.Close();
//  return rv;
//}

// -------------------------------------------------------------------------- ;

void BitMask::Clear()
{
  if (m_owner)
    delete[] m_pBits;

  m_pBits = 0;
  m_owner = 0;
  m_nCols = 0;
  m_nRows = 0;
}

// -------------------------------------------------------------------------- ;

