/******************************************************************************
 *
 * Purpose: Class to contain and manipulate a UUID.
 * 
 ******************************************************************************
 * Copyright (c) 2009
 * PCI Geomatics, 50 West Wilmot Street, Richmond Hill, Ont, Canada
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
 ****************************************************************************/
 
#ifndef __INCLUDE_SEGMENT_UUID_H
#define __INCLUDE_SEGMENT_UUID_H

#include <cassert>
#include <iostream>
#include <iomanip>

namespace PCIDSK
{
namespace priv
{
    class PCIDSKUUID
    {
        friend std::ostream &operator<<(std::ostream &out, PCIDSKUUID &oUUID);
    public:
        PCIDSKUUID(unsigned char *mabyUUID)
        {
            CopyBytes(mabyUUID);
        }
        
        PCIDSKUUID(unsigned int nG1, unsigned short nG2, 
            unsigned short nG3, unsigned short nG4, unsigned char *aG5)
        {
            unsigned int *naUUID = (unsigned int *)mabyUUID;
            unsigned short *nsUUID = (unsigned short *)mabyUUID;
            
            naUUID[0] = nG1;
            
            nsUUID[2] = nG2;
            nsUUID[3] = nG3;
            nsUUID[4] = nG4;
            
            for (unsigned int i = 10; i < 16; i++) {
                mabyUUID[i] = aG5[i - 10];
            }
        }
        
        PCIDSKUUID &operator=(const PCIDSKUUID *poRight)
        {
            CopyBytes(poRight->mabyUUID);
            return *this;
        }
        
        PCIDSKUUID &operator=(const PCIDSKUUID &oRight)
        {
            CopyBytes(oRight.mabyUUID);
            return *this;
        }
        
        bool operator==(const PCIDSKUUID *poRight) const
        {
            return CompareBytes(poRight->mabyUUID);
        }
        
        bool operator==(const PCIDSKUUID &oRight) const
        {
            return CompareBytes(oRight.mabyUUID);
        }
        
        unsigned char &operator[](unsigned int nByteID)
        {
            assert(nByteID < 16);
            
            return mabyUUID[nByteID];
        }
        
        unsigned int GetUUIDVersion(void) const
        {
            return (unsigned int)((mabyUUID[6] >> 4) & 0xa);
        }
        
    private:
        void CopyBytes(const unsigned char abyUUID[16])
        {
            for (unsigned int i = 0; i < 16; i++) {
                mabyUUID[i] = abyUUID[i];
            }
        }
        
        bool CompareBytes(const unsigned char abyUUID[16]) const
        {
            bool bEqual = true;
            
            for (unsigned int i = 0; i < 16; i++) {
                if (mabyUUID[i] != abyUUID[i]) {
                    bEqual = false;
                    break;
                }
            }
            
            return bEqual;
        }
        
        unsigned char mabyUUID[16];
    };
    
    std::ostream &operator<<(std::ostream &out, PCIDSKUUID &oUUID)
    {
        unsigned char *abyUUID = oUUID.mabyUUID;
        unsigned short *nsUUID = (unsigned short *)abyUUID;
        unsigned int *naUUID = (unsigned int *)abyUUID;
        
        out << "{ " << std::setw(8) << std::hex << naUUID[0] << "-";
        out << std::setw(2) << std::hex << nsUUID[2] << "-" << std::setw(2) << std::hex << nsUUID[3] << std::setw(2) << std::hex << nsUUID[4] << "-";
        for (unsigned int i = 10; i < 16; i++) {
            out << std::setw(2) << std::hex << abyUUID[i];
        }
        out << " }";

		return out;
    }
}; // end namespace priv
}; // end namespace PCIDSK

#endif // __INCLUDE_SEGMENT_UUID_H