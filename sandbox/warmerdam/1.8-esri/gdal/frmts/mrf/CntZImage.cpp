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

#include "CntZImage.h"
#include "BitStuffer.h"
#include "BitMask.h"
#include "RLE.h"
#include "Defines.h"
#include <math.h>
#include <float.h>
// #include <iostream>

// -------------------------------------------------------------------------- ;

using namespace std;

// -------------------------------------------------------------------------- ;

CntZImage::CntZImage()
{
  type_ = CNT_Z;
  InfoFromComputeNumBytes info0 = {0};
  m_infoFromComputeNumBytes = info0;
};

CntZImage::~CntZImage()
{
};

// -------------------------------------------------------------------------- ;

bool CntZImage::resizeFill0(long width, long height)
{
  if (!resize(width, height))
    return false;

  memset(getData(), 0, width * height * sizeof(CntZ));
  return true;
}

// -------------------------------------------------------------------------- ;

bool CntZImage::hasValidPixel() const
{
  for (long i = 0; i < height_; i++)
  {
    const CntZ* ptr = data_ + i * width_;
    for (long j = 0; j < width_; j++)
    {
      if (ptr->cnt > 0)
        return true;
      ptr++;
    }
  }
  return false;
}

// -------------------------------------------------------------------------- ;

void CntZImage::normalize()
{
  for (long i = 0; i < height_; i++)
  {
    CntZ* ptr = data_ + i * width_;
    for (long j = 0; j < width_; j++)
    {
      if (ptr->cnt > 0)
      {
        ptr->z /= ptr->cnt;
        ptr->cnt = 1;
      }
      ptr++;
    }
  }
}

// -------------------------------------------------------------------------- ;

// computes the size of a CntZImage of any width and height, but all void / invalid,
// and then compressed

unsigned long CntZImage::computeNumBytesNeededToWriteVoidImage()
{
  unsigned long cnt = 0;

  CntZImage zImg;
  cnt += (unsigned long)zImg.getTypeString().length();    // "CntZImage ", 10 bytes
  cnt += 2 * sizeof(int);
  cnt += 2 * sizeof(long);
  cnt += 1 * sizeof(double);

  // cnt part
  cnt += 3 * sizeof(long);
  cnt += sizeof(float);
//  cnt += zImg.numBytesCntTile(width * height, 0, 0, false);

  // z part
  cnt += 3 * sizeof(long);
  cnt += sizeof(float);
  cnt += zImg.numBytesZTile(0, 0, 0, 0);    // = 1

  return cnt;
}

// -------------------------------------------------------------------------- ;

bool CntZImage::write(Byte** ppByte,
                      double maxZError,
                      bool useInfoFromPrevComputeNumBytes,
                      bool onlyZPart) const
{
  return write(0, ppByte, maxZError, useInfoFromPrevComputeNumBytes, onlyZPart);
}

// -------------------------------------------------------------------------- ;

bool CntZImage::read(Byte** ppByte,
                     double maxZError,
                     bool onlyHeader,
                     bool onlyZPart)
{
  return read(0, ppByte, maxZError, onlyHeader, onlyZPart);
}

// -------------------------------------------------------------------------- ;

//bool CntZImage::write(FILE* fp, double maxZError) const
//{
//  return write(fp, 0, maxZError, false, false);
//}
//
//// -------------------------------------------------------------------------- ;
//
//bool CntZImage::read(FILE* fp, double maxZError, bool onlyHeader)
//{
//  return read(fp, 0, maxZError, onlyHeader, false);
//}

// -------------------------------------------------------------------------- ;

//bool CntZImage::write(const String& fn, double maxZError) const
//{
//  FileOpen fileOpen;
//  FILE* fp = fileOpen.Open((BString)fn, L"wb");
//  if (!fp)
//  {
//    cout << "Error in CntZImage::write(...): cannot open file" << endl;
//    return false;
//  }
//  return write(fp, maxZError);
//}

// -------------------------------------------------------------------------- ;

//bool CntZImage::read(const String& fn, double maxZError, bool onlyHeader)
//{
//  FileOpen fileOpen;
//  FILE* fp = fileOpen.Open((BString)fn, L"rb");
//  if (!fp)
//  {
//    //cout << "Error in CntZImage::read(...): cannot open file" << endl;
//    return false;
//  }
//  return read(fp, maxZError, onlyHeader);
//}

// -------------------------------------------------------------------------- ;
// -------------------------------------------------------------------------- ;

unsigned long CntZImage::computeNumBytesNeededToWrite(double maxZError,
                                                      bool onlyZPart,
                                                      InfoFromComputeNumBytes& info) const
{
  const string fctName = "Error in CntZImage::computeNumBytesNeededToWrite(...): ";

  unsigned long cnt = 0;

  cnt += (unsigned long)getTypeString().length();
  cnt += 2 * sizeof(int);
  cnt += 2 * sizeof(long);
  cnt += 1 * sizeof(double);

  long numTilesVert, numTilesHori, numBytesOpt;
  float maxValInImg;

  // cnt part first
  if (!onlyZPart)
  {
    float cntMin, cntMax;
    if (!computeCntStats(0, height_, 0, width_, cntMin, cntMax))
      return false;

    bool bCntsNoInt = true;
    numTilesVert = 0;    // no tiling
    numTilesHori = 0;
    maxValInImg = cntMax;

    //cout << "cnt range = [ " << cntMin << ", " << cntMax << " ] :  ";

    if (cntMin == cntMax)    // cnt part is const
    {
      bCntsNoInt = fabsf(cntMax - (long)(cntMax + 0.5f)) > 0.0001;
      numBytesOpt = 0;    // nothing else to encode

      //cout << "encode as CONST" << endl;
    }
    else
    {
      bCntsNoInt = cntsNoInt();

      if (!bCntsNoInt && cntMin == 0 && cntMax == 1)    // cnt part is binary mask, use fast RLE class
      {
        // convert to bit mask
        BitMask bitMask(width_, height_);
        bitMask.SetAllValid();
        const CntZ* srcPtr = getData();
        long k = 0;
        for (long i = 0; i < height_; i++)
          for (long j = 0; j < width_; j++, k++)
          {
            if (srcPtr->cnt <= 0)
              bitMask.SetInvalid(k);
            srcPtr++;
          }

        // determine numBytes needed to encode
        RLE rle;
        numBytesOpt = (long)rle.computeNumBytesRLE((const Byte*)bitMask.Bits(), bitMask.Size());

        //cout << "encode as BIT MASK" << endl;
      }
      else
      {
        if (!findTiling(false, 0, bCntsNoInt, numTilesVert, numTilesHori, numBytesOpt, maxValInImg))
        {
//          cout << fctName << "find cnt tiling failed" << endl;
          return 0;
        }

        //cout << "encode as TILES" << endl;
      }
    }

    info.cntsNoInt       = bCntsNoInt;
    info.numTilesVertCnt = numTilesVert;
    info.numTilesHoriCnt = numTilesHori;
    info.numBytesCnt     = numBytesOpt;
    info.maxCntInImg     = maxValInImg;

    cnt += 3 * sizeof(long);
    cnt += sizeof(float);
    cnt += numBytesOpt;
  }

  // z part second
  {
    if (!findTiling(true, maxZError, false, numTilesVert, numTilesHori, numBytesOpt, maxValInImg))
    {
//      cout << fctName << "find z tiling failed" << endl;
      return 0;
    }

    info.maxZError     = maxZError;
    info.numTilesVertZ = numTilesVert;
    info.numTilesHoriZ = numTilesHori;
    info.numBytesZ     = numBytesOpt;
    info.maxZInImg     = maxValInImg;

    cnt += 3 * sizeof(long);
    cnt += sizeof(float);
    cnt += numBytesOpt;
  }

  return cnt;
}

// -------------------------------------------------------------------------- ;
// if you change the file format, don't forget to update not only write and 
// read functions, and the file version number, but also the computeNumBytes...
// and numBytes... functions

bool CntZImage::write(void* fp,
                      Byte** ppByte,
                      double maxZError,
                      bool useInfoFromPrevComputeNumBytes,
                      bool onlyZPart) const
{
  const string fctName = "Error in CntZImage::write(...): ";

  const int version = 11;

  if (!ppByte && !fp)
  {
//    cout << fctName << "output stream not ok" << endl;
    return false;
  }
  if (getSize() == 0)
  {
//    cout << fctName << "no data to write" << endl;
    return false;
  }

  int versionSwap = version;
  int typeSwap    = type_;
  long heightSwap = height_;
  long widthSwap  = width_;
  double maxZErrorSwap = maxZError;

  SWAP_4(versionSwap);
  SWAP_4(typeSwap);
  SWAP_4(heightSwap);
  SWAP_4(widthSwap);
  SWAP_8(maxZErrorSwap);

  if (ppByte)
  {
    Byte* ptr = *ppByte;

    memcpy(ptr, getTypeString().c_str(), getTypeString().length());
    ptr += getTypeString().length();

    *((int*)ptr) = versionSwap;  ptr += sizeof(int);
    *((int*)ptr) = typeSwap;     ptr += sizeof(int);
    *((long*)ptr) = heightSwap;  ptr += sizeof(long);
    *((long*)ptr) = widthSwap;   ptr += sizeof(long);
    *((double*)ptr) = maxZErrorSwap;  ptr += sizeof(double);

    *ppByte = ptr;
  }
  else
  {
    //fwrite(getTypeString().c_str(), 1, getTypeString().length(), fp);

    //fwrite(&versionSwap, sizeof(versionSwap), 1, fp);
    //fwrite(&typeSwap,    sizeof(typeSwap),    1, fp);
    //fwrite(&heightSwap,  sizeof(heightSwap),  1, fp);
    //fwrite(&widthSwap,   sizeof(widthSwap),   1, fp);
    //fwrite(&maxZErrorSwap, sizeof(maxZErrorSwap), 1, fp);

    //if (ferror(fp))
    //{
    //  cout << fctName << "write header failed" << endl;
    //  return false;
    //}
  }

  InfoFromComputeNumBytes info = {0};

  if (useInfoFromPrevComputeNumBytes && (maxZError == m_infoFromComputeNumBytes.maxZError))
  {
    info = m_infoFromComputeNumBytes;
  }
  else if (0 == computeNumBytesNeededToWrite(maxZError, onlyZPart, info))
    return false;

  for (int iPart = 0; iPart < 2; iPart++)
  {
    bool zPart = iPart ? true : false;    // first cnt part, then z part

    if (!zPart && onlyZPart)
      continue;

    bool bCntsNoInt = false;
    long numTilesVert, numTilesHori, numBytesOpt, numBytesWritten;
    float maxValInImg;

    if (!zPart)
    {
      bCntsNoInt   = info.cntsNoInt;
      numTilesVert = info.numTilesVertCnt;
      numTilesHori = info.numTilesHoriCnt;
      numBytesOpt  = info.numBytesCnt;
      maxValInImg  = info.maxCntInImg;
    }
    else
    {
      numTilesVert = info.numTilesVertZ;
      numTilesHori = info.numTilesHoriZ;
      numBytesOpt  = info.numBytesZ;
      maxValInImg  = info.maxZInImg;
    }

    long numTilesVertSwap = numTilesVert;
    long numTilesHoriSwap = numTilesHori;
    long numBytesOptSwap  = numBytesOpt;
    float maxValInImgSwap = maxValInImg;

    SWAP_4(numTilesVertSwap);
    SWAP_4(numTilesHoriSwap);
    SWAP_4(numBytesOptSwap);
    SWAP_4(maxValInImgSwap);

    Byte* bArr = 0;

    if (ppByte)
    {
      Byte* ptr = *ppByte;
      *((long*)ptr) = numTilesVertSwap;  ptr += sizeof(long);
      *((long*)ptr) = numTilesHoriSwap;  ptr += sizeof(long);
      *((long*)ptr) = numBytesOptSwap;   ptr += sizeof(long);
      *((float*)ptr) = maxValInImgSwap;  ptr += sizeof(float);

      *ppByte = ptr;
      bArr = ptr;
    }
    else
    {
      //fwrite(&numTilesVertSwap, sizeof(numTilesVertSwap), 1, fp);
      //fwrite(&numTilesHoriSwap, sizeof(numTilesHoriSwap), 1, fp);
      //fwrite(&numBytesOptSwap,  sizeof(numBytesOptSwap),  1, fp);
      //fwrite(&maxValInImgSwap,  sizeof(maxValInImgSwap),  1, fp);

      //bArr = (Byte*) malloc(numBytesOpt + numExtraBytesToAllocate());
      //if (!bArr)
      //{
      //  cout << fctName << "allocate buffer failed" << endl;
      //  return false;
      //}
    }

    //pt.start();

    if (!zPart && numTilesVert == 0 && numTilesHori == 0)    // no tiling for cnt part
    {
      if (numBytesOpt == 0)    // cnt part is const
        numBytesWritten = 0;

      if (numBytesOpt > 0)    // cnt part is binary mask, use fast RLE class
      {
        // convert to bit mask
        BitMask bitMask(width_, height_);
        bitMask.SetAllValid();
        const CntZ* srcPtr = getData();
        long k = 0;
        for (long i = 0; i < height_; i++)
          for (long j = 0; j < width_; j++, k++)
          {
            if (srcPtr->cnt <= 0)
              bitMask.SetInvalid(k);
            srcPtr++;
          }

        // do RLE encoding, set numBytesWritten
        Byte* pArrRLE;
        size_t numBytesRLE;
        RLE rle;
        if (!rle.compress((const Byte*)bitMask.Bits(), bitMask.Size(), &pArrRLE, numBytesRLE, false))
          return false;

        memcpy(bArr, pArrRLE, numBytesRLE);
        delete[] pArrRLE;
        numBytesWritten = (long)numBytesRLE;
      }
    }
    else
    {
      // encode tiles to buffer
      float maxVal;
      if (!writeTiles(zPart, maxZError, bCntsNoInt, numTilesVert, numTilesHori, bArr, numBytesWritten, maxVal))
      {
        if (!ppByte)
          free(bArr);
        //cout << fctName << "encode to buffer failed" << endl;
        return false;
      }
    }

    if (numBytesWritten != numBytesOpt)
    {
      if (!ppByte)
        free(bArr);
      //cout << fctName << "num bytes written differs from num bytes computed" << endl;
      return false;
    }

    //pt.stop();
    //cout << "time {encode to buffer} = " << pt.ms() << " ms" << endl;

    if (ppByte)
    {
      *ppByte += numBytesWritten;
    }
    else
    {
      //size_t nBytes = fwrite(bArr, 1, numBytesWritten, fp);
      //free(bArr);
      //if (ferror(fp) || nBytes != numBytesWritten)
      //{
      //  cout << fctName << "write out buffer failed" << endl;
      //  return false;
      //}
    }
  }

  return true;
}

// -------------------------------------------------------------------------- ;

bool CntZImage::read(void* fp,
                     Byte** ppByte,
                     double maxZError,
                     bool onlyHeader,
                     bool onlyZPart)
{
  const string fctName = "Error in CntZImage::read(...): ";

  if (!ppByte && !fp)
  {
    //cout << fctName << "input stream not ok" << endl;
    return false;
  }

  size_t len = getTypeString().length();
  string typeStr(len, '0');

  if (ppByte)
  {
    memcpy(&typeStr[0], *ppByte, len);
    *ppByte += len;
  }
  else
  {
    //fread(&typeStr[0], 1, len, fp);
  }

  if (typeStr != getTypeString())
  {
    //cout << fctName << "image is of type " + typeStr + ", not of type " + getTypeString() << endl;
    return false;
  }

  int version = 0, type = 0;
  long width = 0, height = 0;
  double maxZErrorInFile = 0;

  if (ppByte)
  {
    Byte* ptr = *ppByte;

    version = *((const int*)ptr);   ptr += sizeof(int);
    type    = *((const int*)ptr);   ptr += sizeof(int);
    height  = *((const long*)ptr);  ptr += sizeof(long);
    width   = *((const long*)ptr);  ptr += sizeof(long);
    maxZErrorInFile = *((const double*)ptr);  ptr += sizeof(double);

    *ppByte = ptr;
  }
  else
  {
    //fread(&version, sizeof(version), 1, fp);
    //fread(&type,    sizeof(type),    1, fp);
    //fread(&height,  sizeof(height),  1, fp);
    //fread(&width,   sizeof(width),   1, fp);
    //fread(&maxZErrorInFile, sizeof(maxZErrorInFile), 1, fp);

    //if (ferror(fp))
    //{
    //  cout << fctName << "read header failed" << endl;
    //  return false;
    //}

  }

  SWAP_4(version);
  SWAP_4(type);
  SWAP_4(height);
  SWAP_4(width);
  SWAP_8(maxZErrorInFile);

  if (version < 11)
  {
    //cout << fctName << "old file version no longer supported" << endl;
    return false;
  }
  if (type != type_)
  {
    //cout << fctName << "image type mismatch" << endl;
    return false;
  }
  if (width > 20000 || height > 20000)
  {
    //cout << fctName << "image too large, width or height > 20,000" << endl;
    return false;
  }
  if (maxZErrorInFile > maxZError)
  {
    //cout << fctName << "max z error larger than requested" << endl;
    return false;
  }

  if (onlyHeader)
    return true;

  if (!onlyZPart && !resizeFill0(width, height))    // init with (0,0), used below
  {
    //cout << fctName << "resize(...) failed" << endl;
    return false;
  }

  for (int iPart = 0; iPart < 2; iPart++)
  {
    bool zPart = iPart ? true : false;    // first cnt part, then z part

    if (!zPart && onlyZPart)
      continue;

    long numTilesVert = 0, numTilesHori = 0, numBytes = 0;
    float maxValInImg = 0;
    Byte* bArr = 0;

    if (ppByte)
    {
      Byte* ptr = *ppByte;
      numTilesVert = *((const long*)ptr);  ptr += sizeof(long);
      numTilesHori = *((const long*)ptr);  ptr += sizeof(long);
      numBytes     = *((const long*)ptr);  ptr += sizeof(long);
      maxValInImg  = *((const float*)ptr); ptr += sizeof(float);

      *ppByte = ptr;
      bArr = ptr;
    }
    else
    {
      //fread(&numTilesVert, sizeof(numTilesVert), 1, fp);
      //fread(&numTilesHori, sizeof(numTilesHori), 1, fp);
      //fread(&numBytes,     sizeof(numBytes),     1, fp);
      //fread(&maxValInImg,  sizeof(maxValInImg),  1, fp);
    }

    SWAP_4(numTilesVert);
    SWAP_4(numTilesHori);
    SWAP_4(numBytes);
    SWAP_4(maxValInImg);

    if (!ppByte)
    {
      //bArr = (Byte*) malloc(numBytes + numExtraBytesToAllocate());
      //if (!bArr)
      //{
      //  cout << fctName << "allocate buffer failed" << endl;
      //  return false;
      //}

      //size_t nBytesRead = fread(bArr, 1, numBytes, fp);
      //if (ferror(fp) || nBytesRead != numBytes)
      //{
      //  free(bArr);
      //  cout << fctName << "read data into buffer failed" << endl;
      //  return false;
      //}
    }

    if (!zPart && numTilesVert == 0 && numTilesHori == 0)    // no tiling for this cnt part
    {
      if (numBytes == 0)    // cnt part is const
      {
        CntZ* dstPtr = getData();
        for (long i = 0; i < height_; i++)
          for (long j = 0; j < width_; j++)
          {
            dstPtr->cnt = maxValInImg;
            dstPtr++;
          }
      }

      if (numBytes > 0)    // cnt part is binary mask, use fast RLE class
      {
        // decompress to bit mask
        BitMask bitMask(width_, height_);
        RLE rle;
        if (!rle.decompress(bArr, (Byte*)bitMask.Bits()))
        {
	  if (!ppByte)
	    free(bArr);
          return false;
        }

        CntZ* dstPtr = getData();
        long k = 0;
        for (long i = 0; i < height_; i++)
          for (long j = 0; j < width_; j++, k++)
          {
            if (bitMask.IsValid(k))
              dstPtr->cnt = 1;
            else
              dstPtr->cnt = 0;

            dstPtr++;
          }
      }
    }
    else if (!readTiles(zPart, maxZErrorInFile, numTilesVert, numTilesHori, maxValInImg, bArr))
    {
      //cout << fctName << "decode from buffer failed" << endl;
      return false;
    }

    if (ppByte)
      *ppByte += numBytes;
    //else
    //  free(bArr);
  }

  m_tmpDataVec.clear();
  return true;
}

// -------------------------------------------------------------------------- ;

bool CntZImage::findTiling(bool zPart, double maxZError, bool cntsNoInt,
                           long& numTilesVertA,
                           long& numTilesHoriA,
                           long& numBytesOptA,
                           float& maxValInImgA) const
{
  const string fctName = "Error in CntZImage::findTiling(...): ";

  const int tileWidthArr[] = {8, 11, 15, 20, 32, 64};
  const int numConfigs = 6;

  // first, do the entire image as 1 block
  numTilesVertA = 1;
  numTilesHoriA = 1;
  if (!writeTiles(zPart, maxZError, cntsNoInt, 1, 1, 0, numBytesOptA, maxValInImgA))
  {
//    cout << fctName << "write tiles failed" << endl;
    return false;
  }

  // if cnt part is constant 1 (all valid), or 0 or -1 (all invalid),
  // or if all is invalid so z part is empty, then we have to write the header only
  if (numBytesOptA == (zPart ? numBytesZTile(0, 0, 0, 0) : numBytesCntTile(0, 0, 0, false)))
  {
    //printf("block size = %d,  bpp = %f\n", height_ / numTilesVertA, (float)numBytesOptA * 8 / (height_ * width_));
    return true;
  }

  long numBytesPrev = 0;

  for (int k = 0; k < numConfigs; k++)
  {
    int tileWidth = tileWidthArr[k];

    long numTilesVert = height_ / tileWidth;
    long numTilesHori = width_  / tileWidth;

    if ((numTilesVert == 0 || numTilesHori == 0) ||
        (numTilesVert == 1 && numTilesHori == 1))
    {
      return true;
    }

    long numBytes = 0;
    float maxVal;
    if (!writeTiles(zPart, maxZError, cntsNoInt, numTilesVert, numTilesHori, 0, numBytes, maxVal))
    {
//      cout << fctName << "write tiles failed" << endl;
      return false;
    }

    if (numBytes < numBytesOptA)
    {
      numTilesVertA = numTilesVert;
      numTilesHoriA = numTilesHori;
      numBytesOptA = numBytes;
    }

    //printf("  block size = %d,  bpp = %f\n", height_ / numTilesVert, (float)numBytes * 8 / (height_ * width_));

    if (k > 0 && numBytes > numBytesPrev)    // we stop once things get worse by further increasing the block size
    {
      //printf("block size = %d,  bpp = %f\n", height_ / numTilesVertA, (float)numBytesOptA * 8 / (height_ * width_));
      return true;
    }

    numBytesPrev = numBytes;
  }

  //printf("block size = %d,  bpp = %f\n", height_ / numTilesVertA, (float)numBytesOptA * 8 / (height_ * width_));
  return true;
}

// -------------------------------------------------------------------------- ;

bool CntZImage::writeTiles(bool zPart, double maxZError, bool cntsNoInt,
                           long numTilesVert, long numTilesHori,
                           Byte* bArr, long& numBytes, float& maxValInImg) const
{
//  const string fctName = "Error in CntZImage::writeTiles(...): ";
  Byte* ptr = bArr;
  numBytes = 0;
  maxValInImg = -FLT_MAX;

  for (long iTile = 0; iTile <= numTilesVert; iTile++)
  {
    long tileH = height_ / numTilesVert;
    long i0 = iTile * tileH;
    if (iTile == numTilesVert)
      tileH = height_ % numTilesVert;

    if (tileH == 0)
      continue;

    for (long jTile = 0; jTile <= numTilesHori; jTile++)
    {
      long tileW = width_ / numTilesHori;
      long j0 = jTile * tileW;
      if (jTile == numTilesHori)
        tileW = width_ % numTilesHori;

      if (tileW == 0)
        continue;

      float cntMin = 0, cntMax = 0, zMin = 0, zMax = 0;
      long numValidPixel = 0;

      bool rv = zPart ? computeZStats(  i0, i0 + tileH, j0, j0 + tileW, zMin, zMax, numValidPixel) :
                        computeCntStats(i0, i0 + tileH, j0, j0 + tileW, cntMin, cntMax);
      if (!rv)
      {
//        cout << fctName << "compute stats failed" << endl;
        return false;
      }

      maxValInImg = zPart ? max(zMax, maxValInImg) : max(cntMax, maxValInImg);

      //if (cntMin < -1 || cntMax > 1)
      //  { cout << fctName << "cnt values outside [-1..1] detected" << endl;  return false; }

      long numBytesNeeded = zPart ? numBytesZTile(numValidPixel, zMin, zMax, maxZError) :
                                    numBytesCntTile(tileH * tileW, cntMin, cntMax, cntsNoInt);
      numBytes += numBytesNeeded;

      if (bArr)
      {
        long numBytesWritten;
        bool rv = zPart ? writeZTile(  &ptr, numBytesWritten, i0, i0 + tileH, j0, j0 + tileW, numValidPixel, zMin, zMax, maxZError) :
                          writeCntTile(&ptr, numBytesWritten, i0, i0 + tileH, j0, j0 + tileW, cntMin, cntMax, cntsNoInt);
        if (!rv)
        {
//          cout << fctName << "write tile failed" << endl;
          return false;

        }
        if (numBytesWritten != numBytesNeeded)
        {
//          cout << fctName << "num bytes written differs from num bytes computed" << endl;
          return false;
        }
      }
    }
  }

  return true;
}

// -------------------------------------------------------------------------- ;

bool CntZImage::readTiles(bool zPart, double maxZErrorInFile,
                          long numTilesVert, long numTilesHori, float maxValInImg,
                          Byte* bArr)
{
//  const string fctName = "Error in CntZImage::readTiles(...): ";
  Byte* ptr = bArr;

  for (long iTile = 0; iTile <= numTilesVert; iTile++)
  {
    long tileH = height_ / numTilesVert;
    long i0 = iTile * tileH;
    if (iTile == numTilesVert)
      tileH = height_ % numTilesVert;

    if (tileH == 0)
      continue;

    for (long jTile = 0; jTile <= numTilesHori; jTile++)
    {
      long tileW = width_ / numTilesHori;
      long j0 = jTile * tileW;
      if (jTile == numTilesHori)
        tileW = width_ % numTilesHori;

      if (tileW == 0)
        continue;

      bool rv = zPart ? readZTile(  &ptr, i0, i0 + tileH, j0, j0 + tileW, maxZErrorInFile, maxValInImg) :
                        readCntTile(&ptr, i0, i0 + tileH, j0, j0 + tileW);

      if (!rv)
      {
//        cout << fctName << "read tile failed" << endl;
        return false;
      }
    }
  }

  return true;
}

// -------------------------------------------------------------------------- ;

bool CntZImage::cntsNoInt() const
{
  float cntMaxErr = 0;
  for (long i = 0; i < height_; i++)
  {
    const CntZ* ptr = data_ + i * width_;
    for (long j = 0; j < width_; j++)
    {
      float cntErr = fabsf(ptr->cnt - (long)(ptr->cnt + 0.5f));
      cntMaxErr = max(cntErr, cntMaxErr);
      ptr++;
    }
  }
  return (cntMaxErr > 0.0001);
}

// -------------------------------------------------------------------------- ;

bool CntZImage::computeCntStats(long i0, long i1, long j0, long j1,
                                float& cntMinA, float& cntMaxA) const
{
//  const string fctName = "Error in CntZImage::computeCntStats(...): ";

  if (i0 < 0 || j0 < 0 || i1 > height_ || j1 > width_)
  {
//    cout << fctName << "input frame exceeds image boundaries" << endl;
    return false;
  }

  // determine cnt ranges
  float cntMin =  FLT_MAX;
  float cntMax = -FLT_MAX;

  for (long i = i0; i < i1; i++)
  {
    const CntZ* ptr = data_ + i * width_ + j0;
    for (long j = j0; j < j1; j++)
    {
      cntMin = min(ptr->cnt, cntMin);
      cntMax = max(ptr->cnt, cntMax);
      ptr++;
    }
  }

  cntMinA = cntMin;
  cntMaxA = cntMax;
  return true;
}

// -------------------------------------------------------------------------- ;

bool CntZImage::computeZStats(long i0, long i1, long j0, long j1,
                              float& zMinA, float& zMaxA, long& numValidPixelA) const
{
//  const string fctName = "Error in CntZImage::computeZStats(...): ";

  if (i0 < 0 || j0 < 0 || i1 > height_ || j1 > width_)
  {
//   cout << fctName << "input frame exceeds image boundaries" << endl;
    return false;
  }

  // determine z ranges
  float zMin =  FLT_MAX;
  float zMax = -FLT_MAX;
  long numValidPixel = 0;

  for (long i = i0; i < i1; i++)
  {
    const CntZ* ptr = data_ + i * width_ + j0;
    for (long j = j0; j < j1; j++)
    {
      if (ptr->cnt > 0)    // cnt <= 0 means ignore z
      {
        zMin = min(ptr->z, zMin);
        zMax = max(ptr->z, zMax);
        numValidPixel++;
      }
      ptr++;
    }
  }

  if (zMin > zMax)
  {
    zMin = 0;
    zMax = 0;
  }

  zMinA = zMin;
  zMaxA = zMax;
  numValidPixelA = numValidPixel;

  return true;
}

// -------------------------------------------------------------------------- ;

long CntZImage::numBytesCntTile(long numPixel, float cntMin, float cntMax, bool cntsNoInt) const
{
  if (cntMin == cntMax && (cntMin == 0 || cntMin == -1 || cntMin == 1))
    return 1;

  if (cntsNoInt || (cntMax - cntMin) > (1 << 28))
  {
    return(long)(1 + numPixel * sizeof(float));
  }
  else
  {
    unsigned long maxElem = (unsigned long)(cntMax - cntMin + 0.5f);
    return 1 + numBytesFlt(floorf(cntMin + 0.5f)) + BitStuffer::computeNumBytesNeeded(numPixel, maxElem);
  }
}

// -------------------------------------------------------------------------- ;

long CntZImage::numBytesZTile(long numValidPixel, float zMin, float zMax, double maxZError) const
{
  if (numValidPixel == 0 || (zMin == 0 && zMax == 0))
    return 1;

  if (maxZError == 0 || (double)(zMax - zMin) / (2 * maxZError) > (1 << 28))
  {
    return(long)(1 + numValidPixel * sizeof(float));
  }
  else
  {
    unsigned long maxElem = (unsigned long)((double)(zMax - zMin) / (2 * maxZError) + 0.5);
    if (maxElem == 0)
      return 1 + numBytesFlt(zMin);
    else
      return 1 + numBytesFlt(zMin) + BitStuffer::computeNumBytesNeeded(numValidPixel, maxElem);
  }
}

// -------------------------------------------------------------------------- ;

bool CntZImage::writeCntTile(Byte** ppByte, long& numBytes,
                             long i0, long i1, long j0, long j1,
                             float cntMin, float cntMax, bool cntsNoInt) const
{
//  const string fctName = "Error in CntZImage::writeCntTile(...): ";

  Byte* ptr = *ppByte;
  long numPixel = (i1 - i0) * (j1 - j0);

  if (cntMin == cntMax && (cntMin == 0 || cntMin == -1 || cntMin == 1))    // special case all constant 0, -1, or 1
  {
    if (cntMin == 0)
      *ptr++ = 2;
    else if (cntMin == -1)
      *ptr++ = 3;
    else if (cntMin == 1)
      *ptr++ = 4;

    numBytes = 1;
    *ppByte = ptr;
    return true;
  }

  if (cntsNoInt || cntMax - cntMin > (1 << 28))
  {
    // write cnt's as flt arr uncompressed
    *ptr++ = 0;
    float* dstPtr = (float*)ptr;

    for (long i = i0; i < i1; i++)
    {
      const CntZ* srcPtr = getData() + i * width_ + j0;
      for (long j = j0; j < j1; j++)
      {
        *dstPtr = srcPtr->cnt;
        SWAP_4(*dstPtr);
        srcPtr++;
        dstPtr++;
      }
    }

    ptr += numPixel * sizeof(float);
  }
  else
  {
    // write cnt's as long arr bit stuffed
    Byte flag = 1;
    float offset = floorf(cntMin + 0.5f);
    int n = numBytesFlt(offset);
    int bits67 = (n == 4) ? 0 : 3 - n;
    flag |= bits67 << 6;

    *ptr++ = flag;

    if (!writeFlt(&ptr, offset, n))
      return false;

    vector<unsigned long> dataVec(numPixel, 0);
    unsigned long* dstPtr = &dataVec[0];

    for (long i = i0; i < i1; i++)
    {
      const CntZ* srcPtr = getData() + i * width_ + j0;
      for (long j = j0; j < j1; j++)
      {
        *dstPtr++ = (long)(srcPtr->cnt - offset + 0.5f);
        srcPtr++;
      }
    }

    BitStuffer bitStuffer;
    if (!bitStuffer.write(&ptr, dataVec))
    {
//      cout << fctName << "BitStuffer::write(...) failed" << endl;
      return false;
    }
  }

  numBytes = (long)(ptr - *ppByte);
  *ppByte = ptr;
  return true;
}

// -------------------------------------------------------------------------- ;

bool CntZImage::writeZTile(Byte** ppByte, long& numBytes, 
                           long i0, long i1, long j0, long j1,
                           long numValidPixel,
                           float zMin, float zMax, double maxZError) const
{
//  const string fctName = "Error in CntZImage::writeZTile(...): ";

  Byte* ptr = *ppByte;
  long cntPixel = 0;

  if (numValidPixel == 0 || (zMin == 0 && zMax == 0))    // special cases
  {
    *ptr++ = 2;    // set compression flag to 2 to mark tile as constant 0
    numBytes = 1;
    *ppByte = ptr;
    return true;
  }

  if (maxZError == 0 ||                                       // user asks lossless OR
      (double)(zMax - zMin) / (2 * maxZError) > (1 << 28))    // we'd need > 28 bit
  {
    // write z's as flt arr uncompressed
    *ptr++ = 0;
    float* dstPtr = (float*)ptr;

    for (long i = i0; i < i1; i++)
    {
      const CntZ* srcPtr = getData() + i * width_ + j0;
      for (long j = j0; j < j1; j++)
      {
        if (srcPtr->cnt > 0)
        {
          *dstPtr = srcPtr->z;
          SWAP_4(*dstPtr);
          dstPtr++;
          cntPixel++;
        }
        srcPtr++;
      }
    }

    if (cntPixel != numValidPixel)
      return false;

    ptr += numValidPixel * sizeof(float);
  }
  else
  {
    // write z's as long arr bit stuffed
    Byte flag = 1;
    unsigned long maxElem = (unsigned long)((double)(zMax - zMin) / (2 * maxZError) + 0.5);
    if (maxElem == 0)
    {
      flag = 3;    // set compression flag to 3 to mark tile as constant zMin
    }

    int n = numBytesFlt(zMin);
    int bits67 = (n == 4) ? 0 : 3 - n;
    flag |= bits67 << 6;

    *ptr++ = flag;

    if (!writeFlt(&ptr, zMin, n))
      return false;

    if (maxElem > 0)
    {
      vector<unsigned long> dataVec(numValidPixel, 0);
      unsigned long* dstPtr = &dataVec[0];
      double scale = 1 / (2 * maxZError);

      for (long i = i0; i < i1; i++)
      {
        const CntZ* srcPtr = getData() + i * width_ + j0;
        for (long j = j0; j < j1; j++)
        {
          if (srcPtr->cnt > 0)
          {
            *dstPtr++ = (unsigned long)((srcPtr->z - zMin) * scale + 0.5);
            cntPixel++;
          }
          srcPtr++;
        }
      }

      if (cntPixel != numValidPixel)
        return false;

      BitStuffer bitStuffer;
      if (!bitStuffer.write(&ptr, dataVec))
      {
//        cout << fctName << "BitStuffer::write(...) failed" << endl;
        return false;
      }
    }
  }

  numBytes = (long)(ptr - *ppByte);
  *ppByte = ptr;
  return true;
}

// -------------------------------------------------------------------------- ;

bool CntZImage::readCntTile(Byte** ppByte, long i0, long i1, long j0, long j1)
{
//  const string fctName = "Error in CntZImage::readCntTile(...): ";

  Byte* ptr = *ppByte;
  long numPixel = (i1 - i0) * (j1 - j0);

  Byte comprFlag = *ptr++;

  if (comprFlag == 2)    // entire tile is constant 0 (invalid)
  {                      // here we depend on resizeFill0()
    *ppByte = ptr;
    return true;
  }

  if (comprFlag == 3 || comprFlag == 4)    // entire tile is constant -1 (invalid) or 1 (valid)
  {
    CntZ cz1m = {-1, 0};
    CntZ cz1p = { 1, 0};
    CntZ cz1 = (comprFlag == 3) ? cz1m : cz1p;

    for (long i = i0; i < i1; i++)
    {
      CntZ* dstPtr = getData() + i * width_ + j0;
      for (long j = j0; j < j1; j++)
        *dstPtr++ = cz1;
    }

    *ppByte = ptr;
    return true;
  }

  if ((comprFlag & 63) > 4)
  {
//    cout << fctName << "compression flag > 4 detected" << endl;
    return false;
  }

  if (comprFlag == 0)
  {
    // read cnt's as flt arr uncompressed
    const float* srcPtr = (const float*)ptr;

    for (long i = i0; i < i1; i++)
    {
      CntZ* dstPtr = getData() + i * width_ + j0;
      for (long j = j0; j < j1; j++)
      {
        dstPtr->cnt = *srcPtr++;
        SWAP_4(dstPtr->cnt);
        dstPtr++;
      }
    }

    ptr += numPixel * sizeof(float);
  }
  else
  {
    // read cnt's as long arr bit stuffed
    int bits67 = comprFlag >> 6;
    int n = (bits67 == 0) ? 4 : 3 - bits67;

    float offset = 0;
    if (!readFlt(&ptr, offset, n))
      return false;

    //vector<unsigned long> dataVec;
    vector<unsigned long>& dataVec = m_tmpDataVec;
    BitStuffer bitStuffer;
    if (!bitStuffer.read(&ptr, dataVec))
    {
//      cout << fctName << "BitStuffer::read(...) failed" << endl;
      return false;
    }

    unsigned long* srcPtr = &dataVec[0];

    for (long i = i0; i < i1; i++)
    {
      CntZ* dstPtr = getData() + i * width_ + j0;
      for (long j = j0; j < j1; j++)
      {
        dstPtr->cnt = offset + (float)(*srcPtr++);
        dstPtr++;
      }
    }
  }

  *ppByte = ptr;
  return true;
}

// -------------------------------------------------------------------------- ;

bool CntZImage::readZTile(Byte** ppByte, long i0, long i1, long j0, long j1,
                          double maxZErrorInFile, float maxZInImg)
{
//  const string fctName = "Error in CntZImage::readZTile(...): ";

  Byte* ptr = *ppByte;
  long numPixel = 0;

  Byte comprFlag = *ptr++;
  int bits67 = comprFlag >> 6;
  comprFlag &= 63;

  if (comprFlag == 2)    // entire zTile is constant 0 (if valid or invalid doesn't matter)
  {
    for (long i = i0; i < i1; i++)
    {
      CntZ* dstPtr = getData() + i * width_ + j0;
      for (long j = j0; j < j1; j++)
      {
        if (dstPtr->cnt > 0)
          dstPtr->z = 0;
        dstPtr++;
      }
    }

    *ppByte = ptr;
    return true;
  }

  if (comprFlag > 3)
  {
//    cout << fctName << "compression flag > 3 detected" << endl;
    return false;
  }

  if (comprFlag == 0)
  {
    // read z's as flt arr uncompressed
    const float* srcPtr = (const float*)ptr;

    for (long i = i0; i < i1; i++)
    {
      CntZ* dstPtr = getData() + i * width_ + j0;
      for (long j = j0; j < j1; j++)
      {
        if (dstPtr->cnt > 0)
        {
          dstPtr->z = *srcPtr++;
          SWAP_4(dstPtr->z);
          numPixel++;
        }
        dstPtr++;
      }
    }

    ptr += numPixel * sizeof(float);
  }
  else
  {
    // read z's as long arr bit stuffed
    int n = (bits67 == 0) ? 4 : 3 - bits67;
    float offset = 0;
    if (!readFlt(&ptr, offset, n))
      return false;

    if (comprFlag == 3)
    {
      for (long i = i0; i < i1; i++)
      {
        CntZ* dstPtr = getData() + i * width_ + j0;
        for (long j = j0; j < j1; j++)
        {
          if (dstPtr->cnt > 0)
            dstPtr->z = offset;
          dstPtr++;
        }
      }
    }
    else
    {
      //vector<unsigned long> dataVec;
      vector<unsigned long>& dataVec = m_tmpDataVec;
      BitStuffer bitStuffer;
      if (!bitStuffer.read(&ptr, dataVec))
      {
//        cout << fctName << "BitStuffer::read(...) failed" << endl;
        return false;
      }

      double invScale = 2 * maxZErrorInFile;
      unsigned long* srcPtr = &dataVec[0];

      for (long i = i0; i < i1; i++)
      {
        CntZ* dstPtr = getData() + i * width_ + j0;
        for (long j = j0; j < j1; j++)
        {
          if (dstPtr->cnt > 0)
          {
            //dstPtr->z = (float)(offset + *srcPtr++ * invScale);
            float z = (float)(offset + *srcPtr++ * invScale);
            dstPtr->z = min(z, maxZInImg);    // make sure we stay in the orig range
          }
          dstPtr++;
        }
      }
    }
  }

  *ppByte = ptr;
  return true;
}

// -------------------------------------------------------------------------- ;

int CntZImage::numBytesFlt(float z) const
{
  short s = (short)z;
  char c = (char)s;
  return ((float)c == z) ? 1 : ((float)s == z) ? 2 : 4;
}

// -------------------------------------------------------------------------- ;

bool CntZImage::writeFlt(Byte** ppByte, float z, int numBytes) const
{
  Byte* ptr = *ppByte;

  if (numBytes == 1)
  {
    char c = (char)z;
    *((char*)ptr) = c;
  }
  else if (numBytes == 2)
  {
    short s = (short)z;
    SWAP_2(s);
    *((short*)ptr) = s;
  }
  else if (numBytes == 4)
  {
    SWAP_4(z);
    *((float*)ptr) = z;
  }
  else
    return false;

  *ppByte = ptr + numBytes;
  return true;
}

// -------------------------------------------------------------------------- ;

bool CntZImage::readFlt(Byte** ppByte, float& z, int numBytes) const
{
  Byte* ptr = *ppByte;

  if (numBytes == 1)
  {
    char c = *((char*)ptr);
    z = c;
  }
  else if (numBytes == 2)
  {
    short s = *((short*)ptr);
    SWAP_2(s);
    z = s;
  }
  else if (numBytes == 4)
  {
    z = *((float*)ptr);
    SWAP_4(z);
  }
  else
    return false;

  *ppByte = ptr + numBytes;
  return true;
}

// -------------------------------------------------------------------------- ;

