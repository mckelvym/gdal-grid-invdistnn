// msgcommand.h: interface for the msgcommand class.
//
//////////////////////////////////////////////////////////////////////

/******************************************************************************
 *
 * Purpose:  Parse the src_dataset string that is meant for the MSG driver.
 * Author:   Bas Retsios, retsios@itc.nl
 *
 ******************************************************************************
 * Copyright (c) 2004, ITC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ******************************************************************************/

#if !defined(AFX_MSGCOMMAND_H__64E1CE65_4F49_4198_9C4C_13625CF54EC5__INCLUDED_)
#define AFX_MSGCOMMAND_H__64E1CE65_4F49_4198_9C4C_13625CF54EC5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <string>

class MSGCommand  
{
public:
  MSGCommand();
  virtual ~MSGCommand();

  std::string parse(std::string command_line);
  std::string sFileName(int iSequence, int iStrip);
  std::string sPrologueFileName(int iCycle);
  int iNrChannels();
  int iChannel(int iNr);

  static int iNrStrips(int iChannel);

  char cDataConversion;
  int iNrCycles;
  int channel[12];

private:
  std::string sTrimSpaces(std::string str);
  std::string sNextTerm(std::string str, int & iPos);
  int iDaysInMonth(int iMonth, int iYear);
  std::string sCycle(int iCycle);
  static std::string sChannel(int iChannel);
  static int iChannel(std::string sChannel);
  static std::string sTimeStampToFolder(std::string & sTimeStamp);
  std::string sRootFolder;
  std::string sTimeStamp;
  int iStep;
  bool fUseTimestampFolder;
};

#endif // !defined(AFX_MSGCOMMAND_H__64E1CE65_4F49_4198_9C4C_13625CF54EC5__INCLUDED_)
