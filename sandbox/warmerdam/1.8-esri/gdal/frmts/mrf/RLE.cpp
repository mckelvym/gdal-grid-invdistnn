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

#include "RLE.h"
#include <string>
#include <math.h>

// -------------------------------------------------------------------------- ;

size_t RLE::computeNumBytesRLE(const Byte* arr, size_t numBytes) const
{
  if (arr == 0 || numBytes == 0)
    return 0;

  const Byte* ptr = arr;
  size_t sum = 0;
  size_t cntOdd = 0;
  size_t cntEven = 0;
  size_t cntTotal = 0;
  bool bOdd = true;

  while (cntTotal < numBytes - 1)
  {
    if (*ptr != *(ptr + 1))
    {
      if (bOdd)
      {
        cntOdd++;
      }
      else    // switch to odd mode
      {
        cntEven++;
        sum += 2 + 1;
        bOdd = true;
        cntOdd = 0;
        cntEven = 0;
      }
    }
    else
    {
      if (!bOdd)
      {
        cntEven++;
      }
      else
      {
        // count if we have enough equal bytes so it is worth switching to even
        bool foundEnough = false;
        if (cntTotal + m_minNumEven < numBytes)
        {
          int i = 1;
          while (i < m_minNumEven && ptr[i] == ptr[0]) i++;
          foundEnough = i < m_minNumEven ? false : true;
        }

        if (!foundEnough)    // stay in odd mode
        {
          cntOdd++;
        }
        else    // switch to even mode
        {
          if (cntOdd > 0)
            sum += 2 + cntOdd;
          bOdd = false;
          cntOdd = 0;
          cntEven = 0;
          cntEven++;
        }
      }
    }
    ptr++;
    cntTotal++;

    if (cntOdd == 32767)    // prevent short counters from overflow
    {
      sum += 2 + 32767;
      cntOdd = 0;
    }
    if (cntEven == 32767)
    {
      sum += 2 + 1;
      cntEven = 0;
    }
  }

  // don't forget the last byte
  if (bOdd)
  {
    cntOdd++;
    sum += 2 + cntOdd;
  }
  else
  {
    cntEven++;
    sum += 2 + 1;
  }

  return sum + 2;    // EOF short
}

// -------------------------------------------------------------------------- ;

bool RLE::compress(const Byte* arr, size_t numBytes,
                   Byte** arrRLE, size_t& numBytesRLE, bool verify) const
{
  if (arr == 0 || numBytes == 0)
    return false;

  numBytesRLE = computeNumBytesRLE(arr, numBytes);

  *arrRLE = new Byte[numBytesRLE];
  if (!*arrRLE)
    return false;

  const Byte* srcPtr = arr;
  Byte* cntPtr = *arrRLE;
  Byte* dstPtr = cntPtr + 2;
  size_t sum = 0;
  size_t cntOdd = 0;
  size_t cntEven = 0;
  size_t cntTotal = 0;
  bool bOdd = true;

  while (cntTotal < numBytes - 1)
  {
    if (*srcPtr != *(srcPtr + 1))
    {
      *dstPtr++ = *srcPtr;
      if (bOdd)
      {
        cntOdd++;
      }
      else    // switch to odd mode
      {
        cntEven++;
        writeCount(-(short)cntEven, &cntPtr, &dstPtr);    // - sign for even cnts
        sum += 2 + 1;
        bOdd = true;
        cntOdd = 0;
        cntEven = 0;
      }
    }
    else
    {
      if (!bOdd)
      {
        cntEven++;
      }
      else
      {
        // count if we have enough equal bytes so it is worth switching to even
        bool foundEnough = false;
        if (cntTotal + m_minNumEven < numBytes)
        {
          int i = 1;
          while (i < m_minNumEven && srcPtr[i] == srcPtr[0]) i++;
          foundEnough = i < m_minNumEven ? false : true;
        }

        if (!foundEnough)    // stay in odd mode
        {
          *dstPtr++ = *srcPtr;
          cntOdd++;
        }
        else    // switch to even mode
        {
          if (cntOdd > 0)
          {
            writeCount((short)cntOdd, &cntPtr, &dstPtr);    // + sign for odd cnts
            sum += 2 + cntOdd;
          }
          bOdd = false;
          cntOdd = 0;
          cntEven = 0;
          cntEven++;
        }
      }
    }

    if (cntOdd == 32767)    // prevent short counters from overflow
    {
      writeCount((short)cntOdd, &cntPtr, &dstPtr);
      sum += 2 + 32767;
      cntOdd = 0;
    }
    if (cntEven == 32767)
    {
      *dstPtr++ = *srcPtr;
      writeCount(-(short)cntEven, &cntPtr, &dstPtr);
      sum += 2 + 1;
      cntEven = 0;
    }

    srcPtr++;
    cntTotal++;
  }

  // don't forget the last byte
  *dstPtr++ = *srcPtr;
  if (bOdd)
  {
    cntOdd++;
    writeCount((short)cntOdd, &cntPtr, &dstPtr);
    sum += 2 + cntOdd;
  }
  else
  {
    cntEven++;
    writeCount(-(short)cntEven, &cntPtr, &dstPtr);
    sum += 2 + 1;
  }

  writeCount(-32768, &cntPtr, &dstPtr);    // write end of stream symbol
  sum += 2;

  if (verify)
  {
    Byte* arr2 = 0;
    size_t numBytes2 = 0;
    if (!decompress(*arrRLE, &arr2, numBytes2) || numBytes2 != numBytes)
    {
      delete[] arr2;
      return false;
    }
    int nCheck = memcmp(arr, arr2, numBytes);
    delete[] arr2;
    if (nCheck != 0)
      return false;
  }

  return true;
}

// -------------------------------------------------------------------------- ;

bool RLE::decompress(const Byte* arrRLE, Byte** arr, size_t& numBytes) const
{
  if (!arrRLE)
    return false;

  // first count the encoded bytes
  const Byte* srcPtr = arrRLE;
  size_t sum = 0;
  short cnt = readCount(&srcPtr);
  while (cnt != -32768)
  {
    sum += abs(cnt);
    srcPtr += cnt > 0 ? cnt : 1;
    cnt = readCount(&srcPtr);
  }

  numBytes = sum;
  *arr = new Byte[numBytes];
  if (!*arr)
    return false;

  return decompress(arrRLE, *arr);
}

// -------------------------------------------------------------------------- ;

bool RLE::decompress(const Byte* arrRLE, Byte* arr) const
{
  if (!arrRLE || !arr)
    return false;

  const Byte* srcPtr = arrRLE;
  Byte* dstPtr = arr;
  short cnt = readCount(&srcPtr);
  while (cnt != -32768)
  {
    long i = abs(cnt);
    if (cnt > 0)
      while (i--) *dstPtr++ = *srcPtr++;
    else
    {
      Byte b = *srcPtr++;
      while (i--) *dstPtr++ = b;
    }
    cnt = readCount(&srcPtr);
  }

  return true;
}

// -------------------------------------------------------------------------- ;

void RLE::writeCount(short cnt, Byte** ppCnt, Byte** ppDst) const
{
  //assert(cnt != 0);
  SWAP_2(cnt);    // write short's in little endian byte order, always
  *((short*)*ppCnt) = cnt;
  *ppCnt = *ppDst;
  *ppDst += 2;
}

// -------------------------------------------------------------------------- ;

short RLE::readCount(const Byte** ppCnt) const
{
  short cnt = *((const short*)*ppCnt);
  SWAP_2(cnt);
  *ppCnt += 2;
  return cnt;
}

// -------------------------------------------------------------------------- ;

