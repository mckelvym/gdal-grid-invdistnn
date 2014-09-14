
#include <tchar.h>
#include <iostream>
#include <math.h>
#include "CntZImage.h"
#include "PerfTimer.h"

using namespace std;

//------------------------------------------------------------------------------

int _tmain(int argc, _TCHAR* argv[])
{
  // create sample z image

  long h = 512;
  long w = 512;

  CntZImage zImg;
  zImg.resize(w, h);

  for (long i = 0; i < h; i++)
  {
    for (long j = 0; j < w; j++)
    {
	  CntZ cz;
      cz.z = sqrt((float)(i * i + j * j));    // smooth surface
      cz.z += rand() % 20;    // add some small amplitude noise
	  cz.cnt = ((j % 100 == 0 || i % 100 == 0) ? 0:1);    // set some void points
	  zImg(i, j) = cz;
    }
  }

  // LP Write the buffer so I can play with it
// #define WR_BUFFER
#if defined(WR_BUFFER)
  FILE *f = fopen("testimg.raw","wb");
  fwrite(zImg.getData(),2*sizeof(float),h*w,f);
  fclose(f);
#endif

  PerfTimer pt;
  pt.start();

  // compress CntZImage into byte arr

  double maxZErrorWanted = 0.1;
  double eps = 0.0001;    // safety margin, to account for finite floating point accuracy
  double maxZError = maxZErrorWanted - eps;
  unsigned long numBytesNeeded = zImg.computeNumBytesNeededToWrite(maxZError) + CntZImage::numExtraBytesToAllocate();
  Byte* bufferPtr = (Byte*)malloc(numBytesNeeded);
  Byte* ptr = bufferPtr;
  if (!zImg.write(&ptr, maxZError, true))
  {
    cout << "CntZImage::write(...) failed." << endl;
    return 0;
  }

  pt.stop();
  cout << "time for compress = " << pt.ms() << " ms" << endl;

  double ratio = w * h * (0.125 + sizeof(float)) / numBytesNeeded;
  cout << "compression ratio = " << ratio << endl;

  pt.start();

  // decompress

  CntZImage zImgOut;
  ptr = bufferPtr;
  if (!zImgOut.read(&ptr, 1e12))    // no limit on max z error, take whatever was encoded
  {
    cout << "CntZImage::read(...) failed." << endl;
    return 0;
  }

  pt.stop();
  cout << "time for decompress = " << pt.ms() << " ms" << endl;

  // compare to orig

  float maxDelta = 0;
  for (long i = 0; i < h; i++)
  {
    for (long j = 0; j < w; j++)
    {
      if (zImgOut(i, j).cnt != zImg(i, j).cnt)
        cout << "Error in main: decoded image has different valid / invalid mask compared to original." << endl;

      float delta = abs(zImgOut(i, j).z - zImg(i, j).z);
      maxDelta = max(delta, maxDelta);
    }
  }

  cout << "max z error per pixel = " << maxDelta << endl;

  free(bufferPtr);
	return 0;
}

//------------------------------------------------------------------------------

