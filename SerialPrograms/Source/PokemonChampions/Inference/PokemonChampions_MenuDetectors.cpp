/*  Pokemon Champions Menu Detectors
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include "CommonFramework/ImageTools/ImageStats.h"
#include "CommonFramework/VideoPipeline/VideoOverlayScopes.h"
#include "CommonTools/Images/SolidColorTest.h"
#include "PokemonChampions_MenuDetectors.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonChampions{


namespace{

//  ===========================================================================
//  CALIBRATION BLOCK  --  the only thing in this file that needs to change
//  ===========================================================================
//
//  Everything below is a placeholder measured off nothing. To calibrate:
//
//    1.  Capture PNG frames at 1920x1080, 16:9, TV output at 100% (no black
//        border) using: Nintendo Switch -> "---- Testing ----" -> Snapshot
//        Dumper. Set "Image Format" to png -- the default jpg introduces
//        compression artifacts that wreck colour thresholds.
//    2.  Read the boxes off those frames. ImageFloatBox is
//        (x, y, width, height), normalized 0.0-1.0, origin top-left.
//        The Box Draw dev program (same Testing menu) draws live boxes over the
//        feed and is much faster than eyeballing a PNG.
//    3.  Colour ratios passed to is_solid() are NORMALIZED channel ratios, not
//        raw RGB: raw (127, 127, 254) becomes {0.25, 0.25, 0.50}.
//        ImageSolidCheck::debug_string() prints the measured ratio for a box,
//        which is the easiest way to get these numbers.
//    4.  Flip DETECTORS_CALIBRATED to true.
//
//  Until then every detector here returns false on purpose. A placeholder box
//  would otherwise fire on unrelated screens, and a confidently wrong detector
//  is far worse than one that abstains: the Autobattle program is written to
//  run on generic watchers plus timeouts alone, so nothing deadlocks.
constexpr bool DETECTORS_CALIBRATED = false;

//  TODO: calibrate against a main menu / hub frame.
const ImageFloatBox MAIN_MENU_HEADER{0.020, 0.020, 0.300, 0.070};
const ImageFloatBox MAIN_MENU_FOOTER{0.700, 0.930, 0.280, 0.050};

//  TODO: calibrate against a "searching for opponent" frame.
const ImageFloatBox MATCHMAKING_BANNER{0.300, 0.400, 0.400, 0.120};

}



MainMenuDetector::MainMenuDetector(Color color)
    : m_color(color)
{}
void MainMenuDetector::make_overlays(VideoOverlaySet& items) const{
    items.add(m_color, MAIN_MENU_HEADER);
    items.add(m_color, MAIN_MENU_FOOTER);
}
bool MainMenuDetector::detect(const ImageViewRGB32& screen){
    if (!DETECTORS_CALIBRATED){
        return false;
    }
    //  TODO: replace both colour ratios with measured values.
    const ImageStats header = image_stats(extract_box_reference(screen, MAIN_MENU_HEADER));
    if (!is_solid(header, {0.333, 0.333, 0.334})){
        return false;
    }
    const ImageStats footer = image_stats(extract_box_reference(screen, MAIN_MENU_FOOTER));
    if (!is_solid(footer, {0.333, 0.333, 0.334})){
        return false;
    }
    return true;
}



MatchmakingDetector::MatchmakingDetector(Color color)
    : m_color(color)
{}
void MatchmakingDetector::make_overlays(VideoOverlaySet& items) const{
    items.add(m_color, MATCHMAKING_BANNER);
}
bool MatchmakingDetector::detect(const ImageViewRGB32& screen){
    if (!DETECTORS_CALIBRATED){
        return false;
    }
    //  TODO: replace with a measured value.
    const ImageStats banner = image_stats(extract_box_reference(screen, MATCHMAKING_BANNER));
    return is_solid(banner, {0.333, 0.333, 0.334});
}



}
}
}
