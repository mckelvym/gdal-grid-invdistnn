/******************************************************************************
 *
 * Purpose: Implementation of access to a PCIDSK GCP2 Segment
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
#include "segment/cpcidskgcp2segment.h"

#include "pcidsk_gcp.h"
#include "pcidsk_exception.h"

#include <cstring>
#include <iostream>
#include <vector>
#include <string>

using namespace PCIDSK;

struct CPCIDSKGCP2Segment::PCIDSKGCP2SegInfo
{
    std::vector<PCIDSK::GCP> gcps;
    unsigned int num_gcps;
    PCIDSKBuffer seg_data;
    
    std::string map_units;
    unsigned int num_proj;
};

CPCIDSKGCP2Segment::CPCIDSKGCP2Segment(PCIDSKFile *file, int segment, const char *segment_pointer)
    : CPCIDSKSegment(file, segment, segment_pointer), loaded_(false)
{
    pimpl_ = new PCIDSKGCP2SegInfo;
    pimpl_->gcps.clear();
    Load();
}
 
CPCIDSKGCP2Segment::~CPCIDSKGCP2Segment()
{
    delete pimpl_;
}

void CPCIDSKGCP2Segment::Load()
{
    if (loaded_) {
        return;
    }
    
    // Read the the segment in. The first block has information about
    // the structure of the GCP segment (how many, the projection, etc.)
    pimpl_->seg_data.SetSize(data_size - 1024);
    ReadFromFile(pimpl_->seg_data.buffer, 0, data_size - 1024);
    
    // check for 'GCP2    ' in the first 8 bytes
    if (std::strncmp(pimpl_->seg_data.buffer, "GCP2    ", 8) != 0) {
        char magic[9];
        std::strncpy(magic, pimpl_->seg_data.buffer, 8);
        magic[8] = '\0';
        ThrowPCIDSKException("GCP2 segment magic is corrupt. Found '%s'", magic);
    }
    
    // Check the number of blocks field's validity
    unsigned int num_blocks = pimpl_->seg_data.GetInt(8, 8);
    
    if (((data_size - 1024 - 512) / 512) != num_blocks) {
        //ThrowPCIDSKException("Calculated number of blocks (%d) does not match "
        //    "the value encoded in the GCP2 segment (%d).", ((data_size - 1024 - 512)/512), 
        //    num_blocks);
        // Something is messed up with how GDB generates these segments... nice.
    }
    
    pimpl_->num_gcps = pimpl_->seg_data.GetInt(16, 8);
    
    std::cout << "Found GCP segment with " << pimpl_->num_gcps << " GCPs." << std::endl;
    
    // Extract the map units string:
    pimpl_->map_units = std::string(pimpl_->seg_data.buffer + 24, 16);
    
    // Get the number of alternative projections (should be 0!)
    pimpl_->num_proj = pimpl_->seg_data.GetInt(40, 8);
    if (pimpl_->num_proj != 0) {
        ThrowPCIDSKException("There are alternative projections contained in this "
            "GCP2 segment. This functionality is not supported in libpcidsk.");
    }
    
    // Load the GCPs into the vector of PCIDSK::GCPs
    for (unsigned int i = 0; i < pimpl_->num_gcps; i++)
    {
        unsigned int offset = 512 + i * 256;
        bool is_cp = pimpl_->seg_data.buffer[0] == 'C';
        double pixel = pimpl_->seg_data.GetDouble(offset + 6, 14);
        double line = pimpl_->seg_data.GetDouble(offset + 20, 14);
        
        double elev = pimpl_->seg_data.GetDouble(offset + 34, 12);
        double x = pimpl_->seg_data.GetDouble(offset + 48, 22);
        double y = pimpl_->seg_data.GetDouble(offset + 70, 22);
        
        PCIDSK::GCP::EElevationDatum elev_datum = pimpl_->seg_data.buffer[47] != 'M' ? 
            GCP::EEllipsoidal : GCP::EMeanSeaLevel;
        
        char elev_unit_c = pimpl_->seg_data.buffer[46];
        PCIDSK::GCP::EElevationUnit elev_unit = elev_unit_c == 'M' ? GCP::EMetres :
            elev_unit_c == 'F' ? GCP::EInternationalFeet :
            elev_unit_c == 'A' ? GCP::EAmericanFeet : GCP::EUnknown;
        
        double pix_err = pimpl_->seg_data.GetDouble(offset + 92, 10);
        double line_err = pimpl_->seg_data.GetDouble(offset + 102, 10);
        double elev_err = pimpl_->seg_data.GetDouble(offset + 112, 10);
        
        double x_err = pimpl_->seg_data.GetDouble(offset + 122, 14);
        double y_err = pimpl_->seg_data.GetDouble(offset + 136, 14);
        
        std::string gcp_id(pimpl_->seg_data.buffer + 192, 64);
        
        PCIDSK::GCP gcp(x, y, elev,
                        line, pixel, gcp_id, pimpl_->map_units, 
                        x_err, y_err, elev_err,
                        line_err, pix_err);
        gcp.SetElevationUnit(elev_unit);
        gcp.SetElevationDatum(elev_datum);  
        gcp.SetCheckpoint(is_cp);          
        
        pimpl_->gcps.push_back(gcp);
    } 
    
    loaded_ = true;
}

 // Return all GCPs in the segment
std::vector<PCIDSK::GCP> const& CPCIDSKGCP2Segment::GetGCPs(void) const
{
    return pimpl_->gcps;
}
 
// Write the given GCPs to the segment. If the segment already
// exists, it will be replaced with this one.
void CPCIDSKGCP2Segment::SetGCPs(std::vector<PCIDSK::GCP> const& gcps)
{
    pimpl_->num_gcps = gcps.size();
    pimpl_->gcps = gcps; // copy them in
    
    // TODO: regenerate GCP segment data
}
 
// Return the count of GCPs in the segment
unsigned int  CPCIDSKGCP2Segment::GetGCPCount(void) const
{
    return pimpl_->num_gcps;
}
 
// Clear a GCP Segment
void  CPCIDSKGCP2Segment::ClearGCPs(void)
{
    pimpl_->num_gcps = 0;
    pimpl_->gcps.clear();
    
    // TODO: regenerate GCP segment data
}

