/******************************************************************************
 *
 * Purpose:  Declaration of the Segment Factory class.
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
#ifndef __INCLUDE_SEGMENT_SEGMENTFACTORY_H
#define __INCLUDE_SEGMENT_SEGMENTFACTORY_H

#include <string>
#include <map>
#include <vector>

namespace PCIDSK 
{
    struct IPCIDSKSegmentBuilder;
    class PCIDSKSegment;
    class PCIDSKFile;

namespace priv
{

    /**
     * The private PCIDSK Segment factory. This class implements the
     * functionality required to register new classes of segments and
     * construct them from prototypes or abstract factories.
     *
     * The CPCIDSKSegmentFactory is a singleton.
     */
    class CPCIDSKSegmentFactory
    {
    public:
        ~CPCIDSKSegmentFactory();
        
        // TODO: Figure out optimal set of arguments to be passed to the segment factory
        PCIDSKSegment *ConstructSegment(PCIDSKFile *poFile, unsigned int nSegmentNum,
            const char *psSegmentPointer, const std::string &sSegmentMnemonic,
            unsigned int nSegmentType);
        
        // Check out/construct an instance of the segment factory singleton
        static CPCIDSKSegmentFactory *GetInstance(void);
        
        // Register an arbitrary PCIDSK Segment Builder
        void RegisterSegmentBuilderInstance(IPCIDSKSegmentBuilder *poBuilder);
        
        // Register a new class of PCIDSK Segment, contained in the dynamic library
        // in the path provided
        void RegisterSegmentFactory(const std::string &sPath);
    private:
        CPCIDSKSegmentFactory(void);
        void RegisterBuiltInSegmentTypes(void);
        static CPCIDSKSegmentFactory *mpoInstance;
        /**
         * Vector of all the segment builders registered. Typically used
         * to ensure no duplicates are registered and for other housekeeping
         * functions.
         */
        std::vector<IPCIDSKSegmentBuilder *> moAllSegmentBuilders;
        
        /**
         * A tree that maps a segment title (i.e. textual name) to the instance
         * of the segment builder. This tree is then mapped to the segment type
         * number.
         * This is used during the construction of a segment.
         */
        std::map<unsigned int, std::map<std::string, IPCIDSKSegmentBuilder *> > moMapSegmentNameToBuilder;
        
        /**
         * The default segment builder
         */
        IPCIDSKSegmentBuilder *mpoDefaultBuilder;
    };
    
    IPCIDSKSegmentBuilder *LoadSegmentLibrary(const std::string &sLibraryName);

}; // end namespace priv
}; // end namespace PCIDSK

#endif // __INCLUDE_SEGMENT_SEGMENTFACTORY_H
