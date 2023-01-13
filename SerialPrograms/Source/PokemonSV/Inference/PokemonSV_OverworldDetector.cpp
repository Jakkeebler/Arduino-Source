/*  Overworld Detector
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include "Kernels/Waterfill/Kernels_Waterfill_Session.h"
#include "CommonFramework/Globals.h"
#include "CommonFramework/VideoPipeline/VideoOverlayScopes.h"
#include "CommonFramework/ImageMatch/ImageDiff.h"
#include "CommonFramework/ImageMatch/ExactImageMatcher.h"
#include "CommonFramework/ImageTools/BinaryImage_FilterRgb32.h"
#include "PokemonSV_OverworldDetector.h"

//#include <iostream>
//using std::cout;
//using std::endl;

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonSV{



const ImageMatch::ExactImageMatcher& RADAR_BALL(){
    static ImageMatch::ExactImageMatcher matcher(RESOURCE_PATH() + "PokemonSV/RadarBall.png");
    return matcher;
}




OverworldDetector::OverworldDetector(Color color)
    : m_color(color)
    , m_ball(0.890, 0.800, 0.030, 0.060)
    , m_radar(0.815, 0.680, 0.180, 0.310)
    , m_radar_inside(0.865, 0.750, 0.080, 0.170)
{}
void OverworldDetector::make_overlays(VideoOverlaySet& items) const{
    items.add(m_color, m_ball);
    items.add(m_color, m_radar);
    items.add(m_color, m_radar_inside);
}
bool OverworldDetector::detect(const ImageViewRGB32& screen) const{
    if (!detect_ball(screen)){
        return false;
    }

    //  TODO: Detect the directions.

    return true;
}

bool OverworldDetector::detect_ball(const ImageViewRGB32& screen) const{
    using namespace Kernels::Waterfill;

    std::unique_ptr<WaterfillSession> session = make_WaterfillSession();
    ImageViewRGB32 image = extract_box_reference(screen, m_ball);
    std::vector<PackedBinaryMatrix> matrices = compress_rgb32_to_binary_range(
        image,
        {
            {0xffc0c000, 0xffffff1f},
            {0xffd0d000, 0xffffff1f},
            {0xffe0e000, 0xffffff1f},
            {0xfff0f000, 0xffffff1f},
            {0xfff8f800, 0xffffff1f},

            {0xffc0c000, 0xffffff3f},
            {0xffd0d000, 0xffffff3f},
            {0xffe0e000, 0xffffff3f},
            {0xfff0f000, 0xffffff3f},
            {0xfff8f800, 0xffffff3f},

            {0xffc0c000, 0xffffff5f},
            {0xffd0d000, 0xffffff5f},
            {0xffe0e000, 0xffffff5f},
            {0xfff0f000, 0xffffff5f},
            {0xfff8f800, 0xffffff5f},

            {0xffc0c000, 0xffffff7f},
            {0xffd0d000, 0xffffff7f},
            {0xffe0e000, 0xffffff7f},
            {0xfff0f000, 0xffffff7f},
            {0xfff8f800, 0xffffff7f},
        }
    );

//    size_t c = 0;
    for (PackedBinaryMatrix& matrix : matrices){
        session->set_source(matrix);
        auto iter = session->make_iterator(50);
        WaterfillObject object;
        while (iter->find_next(object, false)){
//             c++;
//             extract_box_reference(image, object).save("object-" + std::to_string(c) + ".png");

            //  Exclude if it touches the borders.
            if (object.min_x == 0 || object.min_y == 0 ||
                object.max_x == image.width() || object.max_y == image.height()
            ){
                continue;
            }
            double aspect_ratio = object.aspect_ratio();
            // cout << "object " << c << " ratio " << aspect_ratio << " area " << object.area_ratio() << endl;
            if (!(0.8 < aspect_ratio && aspect_ratio < 1.2)){
                continue;
            }
            double area_ratio = object.area_ratio();
            if (!(0.4 < area_ratio && area_ratio < 0.9)){
                continue;
            }

            // extract_box_reference(image, object).save("ball-" + std::to_string(c) + ".png");
            double rmsd = RADAR_BALL().rmsd(extract_box_reference(image, object));
            // cout << "rmsd = " << rmsd << endl;
            if (rmsd < 50){
                return true;
            }
        }
    }
    return false;
}



OverworldWatcher::OverworldWatcher(
    Color color,
    std::chrono::milliseconds hold_duration
)
     : OverworldDetector(color)
     , VisualInferenceCallback("OverworldWatcher")
     , m_hold_duration(hold_duration)
{}

void OverworldWatcher::make_overlays(VideoOverlaySet& items) const{
    OverworldDetector::make_overlays(items);
}
bool OverworldWatcher::process_frame(const VideoSnapshot& frame){
    //  Return true only if the detection is held for the entire duration and
    //  the radar map has not changed for the duration.

    //  No detection.
    if (!detect(frame)){
        m_start_of_detection.clear();
        return false;
    }

    //  First detection.
    if (!m_start_of_detection){
        m_start_of_detection = frame;
        return false;
    }

    //  Mismatching image sizes.
    ImageViewRGB32 start = extract_box_reference(m_start_of_detection, m_radar_inside);
    ImageViewRGB32 current = extract_box_reference(frame, m_radar_inside);
    if (start.width() != current.width() || start.height() != current.height()){
        m_start_of_detection = frame;
        return false;
    }

    //  Image has changed too much.
    double rmsd = ImageMatch::pixel_RMSD(start, current);
//    cout << "rmsd = " << rmsd << endl;
    if (rmsd > 2.0){
        m_start_of_detection = frame;
        return false;
    }

    //  Make sure it's held for long enough.
    return frame.timestamp - m_start_of_detection.timestamp >= m_hold_duration;
}





}
}
}
