/******************************************************************************
 *
 * Purpose: Implementation of the Segment Factory. Segment types are loaded,
 *          either dynamically or statically, and registered with this factory
 *          to make them available to the system
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
 
#include "segment/segmentfactory.h"
#include "segment/pcidsksegmentbuilder.h"

#include <iostream>
#include <string>
#include <vector>
#include <map>

using namespace PCIDSK;
using namespace PCIDSK::priv;
// Built-in segment classes

CPCIDSKSegmentFactory *CPCIDSKSegmentFactory::mpoInstance = NULL;

CPCIDSKSegmentFactory::CPCIDSKSegmentFactory()
{
   
}

CPCIDSKSegmentFactory::~CPCIDSKSegmentFactory()
{
    
}

CPCIDSKSegmentFactory *CPCIDSKSegmentFactory::GetInstance(void)
{
    
}

void CPCIDSKSegmentFactory::RegisterSegmentBuilderInstance(IPCIDSKSegmentBuilder *poBuilder)
{
    moAllSegmentBuilders.push_back(poBuilder);
    
    std::vector<std::string> oSegmentStrings = poBuilder->GetSupportedSegments();
    
    for (unsigned int i = 0; i < oSegmentStrings.size(); i++) {
        moMapSegmentNameToBuilder.insert(
            std::pair<std::string, IPCIDSKSegmentBuilder *>(oSegmentStrings[i], poBuilder));
    }
}

void CPCIDSKSegmentFactory::RegisterSegmentFactory(const std::string &sPath)
{
    if (LoadSegmentLibrary(sPath) != true) {
        std::cout << "Failed to load library " << sPath << std::endl;
    }
}
