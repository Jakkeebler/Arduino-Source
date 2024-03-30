/*  Fast Travel Detector
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include "Common/Cpp/Containers/FixedLimitVector.tpp"
#include "CommonFramework/ImageMatch/WaterfillTemplateMatcher.h"
#include "CommonFramework/ImageTypes/ImageViewRGB32.h"
#include "CommonFramework/ImageTools/WaterfillUtilities.h"
#include "CommonFramework/VideoPipeline/VideoOverlayScopes.h"
#include "Kernels/Waterfill/Kernels_Waterfill_Types.h"
#include "PokemonSV_FastTravelDetector.h"

// #include <iostream>
//using std::cout;
//using std::endl;

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonSV{

ImageFloatBox MINIMAP_AREA(0.815, 0.680, 0.180, 0.310);


class FastTravelMatcher : public ImageMatch::WaterfillTemplateMatcher{
public:
    FastTravelMatcher() : WaterfillTemplateMatcher(
        "PokemonSV/Map/FastTravelIcon-Template.png", Color(0,0,0), Color(255, 255, 255), 450
    ) {
        m_aspect_ratio_lower = 0.9;
        m_aspect_ratio_upper = 1.2;
        m_area_ratio_lower = 0.7;
        m_area_ratio_upper = 1.1;
    }

    static const ImageMatch::WaterfillTemplateMatcher& instance() {
        static FastTravelMatcher matcher;
        return matcher;
    }
};


FastTravelDetector::~FastTravelDetector() = default;

FastTravelDetector::FastTravelDetector(Color color, const ImageFloatBox& box)
    : m_color(color)
    , m_box(box)
{}

void FastTravelDetector::make_overlays(VideoOverlaySet& items) const{
    items.add(m_color, m_box);
}

bool FastTravelDetector::detect(const ImageViewRGB32& screen) const{
    std::vector<ImageFloatBox> hits = detect_all(screen);
    return !hits.empty();
}

std::vector<ImageFloatBox> FastTravelDetector::detect_all(const ImageViewRGB32& screen) const{
    const std::vector<std::pair<uint32_t, uint32_t>> filters = {
        {combine_rgb(0, 0, 80), combine_rgb(140, 140, 255)}
    };

    const double min_object_size = 400.0;
    const double rmsd_threshold = 60.0;

    const double screen_rel_size = (screen.height() / 1080.0);
    const size_t min_size = size_t(screen_rel_size * screen_rel_size * min_object_size);

    std::vector<ImageFloatBox> found_locations;

    ImagePixelBox pixel_search_area = floatbox_to_pixelbox(screen.width(), screen.height(), m_box);    
    match_template_by_waterfill(
        extract_box_reference(screen, m_box), 
        FastTravelMatcher::instance(),
        filters,
        {min_size, SIZE_MAX},
        rmsd_threshold,
        [&](Kernels::Waterfill::WaterfillObject& object) -> bool {
            ImagePixelBox found_box(
                object.min_x + pixel_search_area.min_x, object.min_y + pixel_search_area.min_y,
                object.max_x + pixel_search_area.min_x, object.max_y + pixel_search_area.min_y);
            found_locations.emplace_back(pixelbox_to_floatbox(screen.width(), screen.height(), found_box));
            return false;
        }
    );

    return found_locations;
}



FastTravelWatcher::~FastTravelWatcher() = default;

FastTravelWatcher::FastTravelWatcher(Color color, VideoOverlay& overlay, const ImageFloatBox& box)
    : VisualInferenceCallback("FastTravelWatcher")
    , m_overlay(overlay)
    , m_detector(color, box)
{}

void FastTravelWatcher::make_overlays(VideoOverlaySet& items) const{
    m_detector.make_overlays(items);
}

bool FastTravelWatcher::process_frame(const ImageViewRGB32& screen, WallClock timestamp){
    std::vector<ImageFloatBox> hits = m_detector.detect_all(screen);

    m_hits.reset(hits.size());
    for (const ImageFloatBox& hit : hits){
        m_hits.emplace_back(m_overlay, hit, COLOR_MAGENTA);
    }

    return !hits.empty();
}








}
}
}
