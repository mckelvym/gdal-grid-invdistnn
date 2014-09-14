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

#include <string>
#include "Defines.h"

/** BitMask - Convenient and fast access to binary mask bits
 *
 */

class BitMask
{
public:
  BitMask();
  //BitMask(IPixelBlock* pBlock, long bandIndex);
  BitMask(long nCols, long nRows);
  BitMask(long nCols, long nRows, Byte* pBits, bool owner = true);
  //BitMask(LPCTSTR file);
  BitMask(const BitMask& copy);
  virtual ~BitMask()                           { Clear(); }

  // 1: valid, 0: not valid
  Byte IsValid(long k) const                   { return (m_pBits[k >> 3] & Bit(k)) > 0; }
  Byte IsValid(long row, long col) const       { return IsValid(row * m_nCols + col); }

  Byte IsValid3x3(long k) const                { return IsValid3(k - m_nCols) &&
                                                        IsValid3(k) &&
                                                        IsValid3(k + m_nCols); }

  Byte IsValid3x3(long row, long col) const    { return IsValid3x3(row * m_nCols + col); }
  Byte IsValid3(long k) const                  { return IsValid(k - 1) &&
                                                        IsValid(k) &&
                                                        IsValid(k + 1); }

  Byte IsValid3x3But1(long k) const;
  Byte IsValid3x3But1(long row, long col) const { return IsValid3x3But1(row * m_nCols + col); }

  void  SetValid(long k) const                 { m_pBits[k >> 3] |= Bit(k); }
  void  SetValid(long row, long col) const     { SetValid(row * m_nCols + col); }

  void  SetInvalid(long k) const               { m_pBits[k >> 3] &= ~Bit(k); }
  void  SetInvalid(long row, long col) const   { SetInvalid(row * m_nCols + col); }

  void  SetAllValid() const                    { memset(m_pBits, 255, Size()); }
  void  SetAllInvalid() const                  { memset(m_pBits,   0, Size()); }

  bool  SetSize(long nCols, long nRows);

  long  GetWidth() const                       { return m_nCols; }
  long  GetHeight() const                      { return m_nRows; }
  long  Size() const                           { return (m_nCols * m_nRows + 7) >> 3; }
  const Byte* Bits() const                     { return m_pBits; }
  Byte* Bits()                                 { return m_pBits; }
  Byte  Bit(long k) const                      { return (1 << 7) >> (k & 7); }

  long  CountValidBits() const;
  void  ClipFrameToMask(long& x, long& y, long& w, long& h) const;
  void  Copy(const BitMask& otherBits,      long x0, long y0, long x1 = 0, long y1 = 0, long xsize = 0, long ysize = 0);
  void  Union(const BitMask& otherBits,     long x0, long y0, long x1 = 0, long y1 = 0, long xsize = 0, long ysize = 0);
  void  Intersect(const BitMask& otherBits, long x0, long y0, long x1 = 0, long y1 = 0, long xsize = 0, long ysize = 0);
  void  Clear();
  //bool  SaveTo(LPCTSTR maskFile) const;

  //
  // template function using the bit mask
  //
  template<class T> T Add3Cells(const T* E, long k, long offset) const
  {
    assert(E);
    assert(Size() > 0);
    T z = 0;
    int cnt = 0;

    if (IsValid(k - offset))
    {
      z += E[k - offset];
      cnt++;
    }
    if (IsValid(k))
    {
      z += 2 * E[k];
      cnt += 2;
    }
    if (IsValid(k + offset))
    {
      z += E[k + offset];
      cnt++;
    }

    return (T)(z * 4 / cnt);
  }


protected:
  Byte*  m_pBits;
  long   m_nCols;
  long   m_nRows;
  bool   m_owner;

private:
  BitMask& operator=(BitMask& m) { return m; };    // forbid this, use copy()
};

// -------------------------------------------------------------------------- ;
//
// inline functions
//
inline Byte BitMask::IsValid3x3But1(long k) const
{
  long k0 = k - m_nCols;
  long k2 = k + m_nCols;

  return IsValid(k) &&    // center pixel must be valid
       ((IsValid(k0 - 1) + IsValid(k0) + IsValid(k0 + 1) +
         IsValid(k  - 1) +               IsValid(k  + 1) +
         IsValid(k2 - 1) + IsValid(k2) + IsValid(k2 + 1)) >= 7);
}

// -------------------------------------------------------------------------- ;

