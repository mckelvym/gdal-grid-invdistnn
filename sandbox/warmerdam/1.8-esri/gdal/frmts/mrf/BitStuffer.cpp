/*
COPYRIGHT 1995-2012 ESRI
TRADE SECRETS: ESRI PROPRIETARY AND CONFIDENTIAL
Unpublished material - all rights reserved under the 
Copyright Laws of the United States and applicable international
laws, treaties, and conventions.
 
For additional information, contact:
Environmental Systems Research Institute, Inc.
Attn: Contracts and Legal Services Department
380 New York Street
Redlands, California, 92373
USA
 
email: contracts@esri.com
*/

#include "BitStuffer.h"
// #include <iostream>

// -------------------------------------------------------------------------- ;

using namespace std;

// -------------------------------------------------------------------------- ;

// see the old stream IO functions below on how to call.
// if you change write(...) / read(...), don't forget to update computeNumBytesNeeded(...).

bool BitStuffer::write(Byte** ppByte, const vector<unsigned long>& dataVec) const
{
//  const string fctName = "Error in BitStuffer::write(...): ";

  if (!ppByte)
    return false;

  if (dataVec.empty())
  {
//    cout << fctName << "no data to write" << endl;
    return false;
  }

  unsigned long maxElem = findMax(dataVec);

  int numBits = 0;
  while (maxElem >> numBits)
    numBits++;
  Byte numBitsByte = (Byte)numBits;
  unsigned long numElements = (unsigned long)dataVec.size();
  unsigned long numULongs = (numElements * numBits + 31) / 32;

  // use the upper 2 bits to encode the type used for numElements: Byte, ushort, or ulong
  int n = numBytesULong(numElements);
  int bits67 = (n == 4) ? 0 : 3 - n;
  numBitsByte |= bits67 << 6;

  **ppByte = numBitsByte;
  (*ppByte)++;

  if (!writeULong(ppByte, numElements, n))
    return false;

  if (numULongs > 0)    // numBits can be 0, then we only write the header
  {
    unsigned long numBytes = numULongs * sizeof(unsigned long);
    unsigned long* arr = (unsigned long*)(*ppByte);

    memset(arr, 0, numBytes);

    // do the stuffing
    const unsigned long* srcPtr = &dataVec[0];
    unsigned long* dstPtr = arr;
    int bitPos = 0;

    for (unsigned long i = 0; i < numElements; i++)
    {
      if (32 - bitPos >= numBits)
      {
        *dstPtr |= (*srcPtr++) << (32 - bitPos - numBits);
        bitPos += numBits;
        if (bitPos == 32)    // shift >= 32 is undefined
        {
          bitPos = 0;
          dstPtr++;
        }
      }
      else
      {
        int n = numBits - (32 - bitPos);
        *dstPtr++ |= (*srcPtr  ) >> n;
        *dstPtr   |= (*srcPtr++) << (32 - n);
        bitPos = n;
      }
    }

    // save the 0-3 bytes not used in the last ULong
    unsigned long numBytesNotNeeded = numTailBytesNotNeeded(numElements, numBits);
    unsigned long n = numBytesNotNeeded;
    while (n--)
      *dstPtr >>= 8;

    dstPtr = arr;
    for (unsigned long i = 0; i < numULongs; i++)
    {
      SWAP_4(*dstPtr);
      dstPtr++;
    }

    *ppByte += numBytes - numBytesNotNeeded;
  }

  return true;
}

// -------------------------------------------------------------------------- ;

bool BitStuffer::read(Byte** ppByte, vector<unsigned long>& dataVec) const
{
//   const string fctName = "Error in BitStuffer::read(...): ";

  if (!ppByte)
    return false;

  Byte numBitsByte = **ppByte;
  (*ppByte)++;

  int bits67 = numBitsByte >> 6;
  int n = (bits67 == 0) ? 4 : 3 - bits67;

  numBitsByte &= 63;    // bits 0-5;

  unsigned long numElements = 0;
  if (!readULong(ppByte, numElements, n))
    return false;

  if (numBitsByte >= 32)
    return false;

  int numBits = numBitsByte;
  unsigned long numULongs = (numElements * numBits + 31) / 32;
  dataVec.resize(numElements, 0);    // init with 0

  if (numULongs > 0)    // numBits can be 0
  {
    unsigned long numBytes = numULongs * sizeof(unsigned long);
    unsigned long* arr = (unsigned long*)(*ppByte);

    unsigned long* srcPtr = arr;
    for (unsigned long i = 0; i < numULongs; i++)
    {
      SWAP_4(*srcPtr);
      srcPtr++;
    }

    // needed to save the 0-3 bytes not used in the last ULong
    srcPtr--;
    unsigned long lastULong = *srcPtr;
    unsigned long numBytesNotNeeded = numTailBytesNotNeeded(numElements, numBits);
    unsigned long n = numBytesNotNeeded;
    while (n--)
      *srcPtr <<= 8;

    // do the un-stuffing
    srcPtr = arr;
    unsigned long* dstPtr = &dataVec[0];
    int bitPos = 0;

    for (unsigned long i = 0; i < numElements; i++)
    {
      if (32 - bitPos >= numBits)
      {
        unsigned long n = (*srcPtr) << bitPos;
        *dstPtr++ = n >> (32 - numBits);
        bitPos += numBits;
        if (bitPos == 32)    // shift >= 32 is undefined
        {
          bitPos = 0;
          srcPtr++;
        }
      }
      else
      {
        unsigned long n = (*srcPtr++) << bitPos;
        *dstPtr = n >> (32 - numBits);
        bitPos -= (32 - numBits);
        *dstPtr++ |= (*srcPtr) >> (32 - bitPos);
      }
    }

    if (numBytesNotNeeded > 0)
      *srcPtr = lastULong;    // restore the last ULong

    *ppByte += numBytes - numBytesNotNeeded;
  }

  return true;
}

// -------------------------------------------------------------------------- ;

//bool BitStuffer::write(ofstream& fout, const vector<unsigned long>& dataVec) const
//{
//  if (!fout.good())
//    return false;
//
//  unsigned long numElem = (unsigned long)dataVec.size();
//  unsigned long maxElem = findMax(dataVec);
//  unsigned long numBytes = computeNumBytesNeeded(numElem, maxElem);
//
//  Byte* arr = (Byte*)malloc(numBytes + numExtraBytesToAllocate());
//  if (!arr)
//    return false;
//
//  Byte* ptr = arr;
//  if (!write(&ptr, dataVec))
//  {
//    free(arr);
//    return false;
//  }
//
//  fout.write((char*)arr, numBytes);
//
//  free(arr);
//  return fout.good();
//}

// -------------------------------------------------------------------------- ;

//bool BitStuffer::read(ifstream& fin, vector<unsigned long>& dataVec) const
//{
//  if (!fin.good())
//    return false;
//
//  // we need to read into the file to know how many bytes we have to allocate
//  Byte numBitsByte = 0;
//  unsigned long numElements = 0;
//  unsigned long numBytesHeader = 0;
//
//  fin.read((char*)&numBitsByte, 1);
//  numBytesHeader += 1;
//
//  int bits67 = numBitsByte >> 6;
//  int n = (bits67 == 0) ? 4 : 3 - bits67;
//
//  numBitsByte &= 63;    // bits 0-5;
//
//  Byte buf[4] = {0};
//  fin.read((char*)buf, n);
//  numBytesHeader += n;
//
//  Byte* ptr = buf;
//  if (!readULong(&ptr, numElements, n))
//    return false;
//
//  if (numBitsByte >= 32)
//    return false;
//
//  int numBits = numBitsByte;
//  unsigned long numULongs = (numElements * numBits + 31) / 32;
//  unsigned long numBytes = numBytesHeader + numULongs * sizeof(unsigned long) -
//    numTailBytesNotNeeded(numElements, numBits);
//
//  Byte* arr = (Byte*)malloc(numBytes + numExtraBytesToAllocate());
//
//  fin.seekg(-numBytesHeader, ios::cur);    // rewind the read header bytes
//
//  fin.read((char*)arr, numBytes);
//  if (!fin.good())
//  {
//    free(arr);
//    return false;
//  }
//
//  ptr = arr;
//  bool rv = read(&ptr, dataVec);
//
//  free(arr);
//  return rv;
//}

// -------------------------------------------------------------------------- ;

unsigned long BitStuffer::computeNumBytesNeeded(unsigned long numElem, unsigned long maxElem)
{
  int numBits = 0;
  while (maxElem >> numBits)
    numBits++;
  unsigned long numULongs = (numElem * numBits + 31) / 32;
  unsigned long numBytes = 1 + numBytesULong(numElem) + numULongs * sizeof(unsigned long) -
    numTailBytesNotNeeded(numElem, numBits);

  return numBytes;
}

// -------------------------------------------------------------------------- ;
// -------------------------------------------------------------------------- ;

unsigned long BitStuffer::findMax(const vector<unsigned long>& dataVec) const
{
  unsigned long maxElem = 0;
  const unsigned long* ptr = &dataVec[0];
  size_t i = dataVec.size();
  while (i--)
  {
    unsigned long n = *ptr++;
    maxElem = max(n, maxElem);
  }
  return maxElem;
}

// -------------------------------------------------------------------------- ;

bool BitStuffer::writeULong(Byte** ppByte, unsigned long k, int numBytes) const
{
  Byte* ptr = *ppByte;

  if (numBytes == 1)
  {
    *ptr = (Byte)k;
  }
  else if (numBytes == 2)
  {
    unsigned short s = (unsigned short)k;
    SWAP_2(s);
    *((unsigned short*)ptr) = s;
  }
  else if (numBytes == 4)
  {
    SWAP_4(k);
    *((unsigned long*)ptr) = k;
  }
  else
    return false;

  *ppByte = ptr + numBytes;
  return true;
}

// -------------------------------------------------------------------------- ;

bool BitStuffer::readULong(Byte** ppByte, unsigned long& k, int numBytes) const
{
  Byte* ptr = *ppByte;

  if (numBytes == 1)
  {
    k = *ptr;
  }
  else if (numBytes == 2)
  {
    unsigned short s = *((unsigned short*)ptr);
    SWAP_2(s);
    k = s;
  }
  else if (numBytes == 4)
  {
    k = *((unsigned long*)ptr);
    SWAP_4(k);
  }
  else
    return false;

  *ppByte = ptr + numBytes;
  return true;
}

// -------------------------------------------------------------------------- ;

unsigned long BitStuffer::numTailBytesNotNeeded(unsigned long numElem, int numBits)
{
  int numBitsTail = (numElem * numBits) & 31;
  int numBytesTail = (numBitsTail + 7) >> 3;
  return (numBytesTail > 0) ? 4 - numBytesTail : 0;
}

// -------------------------------------------------------------------------- ;

