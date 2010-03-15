#ifndef __INCLUDE_SEGMENT_CPCIDSKAPMODEL_H
#define __INCLUDE_SEGMENT_CPCIDSKAPMODEL_H

#include "pcidsk_airphoto.h"
#include "segment/cpcidsksegment.h"

#include <string>
#include <vector>

namespace PCIDSK {

    class CPCIDSKAPModelSegment : public CPCIDSKSegment,
                                  public PCIDSKAPModelSegment
    {
    public:
        CPCIDSKAPModelSegment(PCIDSKFile *file, int segment,
            const char *segment_pointer);
            
        ~CPCIDSKAPModelSegment();

        unsigned int GetWidth(void) const;
        unsigned int GetHeight(void) const;
        unsigned int GetDownsampleFactor(void) const;

        // Interior Orientation Parameters
        PCIDSKAPModelIOParams const& GetInteriorOrientationParams(void) const;
        
        // Exterior Orientation Parameters
        PCIDSKAPModelEOParams const& GetExteriorOrientationParams(void) const;

        // ProjInfo
        PCIDSKAPModelMiscParams const& GetAdditionalParams(void) const;
        
        std::string GetMapUnitsString(void) const;
        std::string GetUTMUnitsString(void) const;
        std::vector<double> const& GetProjParams(void) const;

    private:
        void UpdateFromDisk();
        
        PCIDSKBuffer buf;
        std::string map_units_, utm_units_;
        std::vector<double> proj_parms_;
        PCIDSKAPModelIOParams* io_params_;
        PCIDSKAPModelEOParams* eo_params_;
        PCIDSKAPModelMiscParams* misc_params_;
        unsigned int width_, height_, downsample_;
        bool filled_;
    };

} // end namespace PCIDSK

#endif // __INCLUDE_SEGMENT_CPCIDSKAPMODEL_H

