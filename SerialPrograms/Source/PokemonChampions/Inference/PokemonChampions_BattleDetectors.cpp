/*  Pokemon Champions Battle Detectors
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include "CommonFramework/ImageTools/ImageStats.h"
#include "CommonFramework/VideoPipeline/VideoOverlayScopes.h"
#include "CommonTools/Images/SolidColorTest.h"
#include "PokemonChampions_BattleDetectors.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonChampions{


namespace{

//  ===========================================================================
//  CALIBRATION BLOCK  --  the only thing in this file that needs to change
//  ===========================================================================
//
//  See the equivalent block in PokemonChampions_MenuDetectors.cpp for the full
//  capture and measurement procedure. Same rules apply:
//    -  ImageFloatBox is (x, y, width, height), normalized, origin top-left.
//    -  is_solid() colour ratios are normalized channel ratios, not raw RGB.
//    -  Flip DETECTORS_CALIBRATED once every constant below is measured.
//
//  Frames needed here, in priority order:
//    1.  6-8 frames of the move select UI, cursor on each move slot, plus one
//        with a disabled / 0-PP move. Needed for BATTLE_MENU_* and, later, for
//        per-slot cursor detection.
//    2.  One frame of the win banner AND one of the loss banner. A single box
//        that cannot tell them apart makes the win/loss stats useless.
//    3.  4 frames of the forced switch flow: the "X fainted" message, the
//        switch selection screen, and the confirm step.
constexpr bool DETECTORS_CALIBRATED = false;

//  TODO: calibrate against move select frames.
const ImageFloatBox BATTLE_MENU_PANEL{0.500, 0.740, 0.480, 0.230};
const ImageFloatBox BATTLE_MENU_EDGE {0.020, 0.740, 0.200, 0.060};

//  TODO: calibrate against the forced switch screen.
const ImageFloatBox FORCED_SWITCH_PANEL{0.050, 0.150, 0.400, 0.700};

//  TODO: calibrate against win and loss banners. The two colour ratios below
//  are what separates a win from a loss -- they must come from real frames.
const ImageFloatBox RESULT_BANNER{0.250, 0.300, 0.500, 0.200};
const double        RESULT_WIN_RATIO[3] {0.333, 0.333, 0.334};
const double        RESULT_LOSS_RATIO[3]{0.333, 0.333, 0.334};

}


const char* battle_result_name(BattleResult result){
    switch (result){
    case BattleResult::Win:     return "Win";
    case BattleResult::Loss:    return "Loss";
    default:                    return "Unknown";
    }
}



BattleMenuDetector::BattleMenuDetector(Color color)
    : m_color(color)
{}
void BattleMenuDetector::make_overlays(VideoOverlaySet& items) const{
    items.add(m_color, BATTLE_MENU_PANEL);
    items.add(m_color, BATTLE_MENU_EDGE);
}
bool BattleMenuDetector::detect(const ImageViewRGB32& screen){
    if (!DETECTORS_CALIBRATED){
        return false;
    }
    //  TODO: replace both colour ratios with measured values.
    const ImageStats panel = image_stats(extract_box_reference(screen, BATTLE_MENU_PANEL));
    if (!is_solid(panel, {0.333, 0.333, 0.334})){
        return false;
    }
    const ImageStats edge = image_stats(extract_box_reference(screen, BATTLE_MENU_EDGE));
    if (!is_solid(edge, {0.333, 0.333, 0.334})){
        return false;
    }
    return true;
}



ForcedSwitchDetector::ForcedSwitchDetector(Color color)
    : m_color(color)
{}
void ForcedSwitchDetector::make_overlays(VideoOverlaySet& items) const{
    items.add(m_color, FORCED_SWITCH_PANEL);
}
bool ForcedSwitchDetector::detect(const ImageViewRGB32& screen){
    if (!DETECTORS_CALIBRATED){
        return false;
    }
    //  TODO: replace with a measured value.
    const ImageStats panel = image_stats(extract_box_reference(screen, FORCED_SWITCH_PANEL));
    return is_solid(panel, {0.333, 0.333, 0.334});
}



BattleResultDetector::BattleResultDetector(Color color)
    : m_color(color)
{}
void BattleResultDetector::make_overlays(VideoOverlaySet& items) const{
    items.add(m_color, RESULT_BANNER);
}
bool BattleResultDetector::detect(const ImageViewRGB32& screen){
    m_current = BattleResult::Unknown;
    if (!DETECTORS_CALIBRATED){
        return false;
    }
    const ImageStats banner = image_stats(extract_box_reference(screen, RESULT_BANNER));
    if (is_solid(banner, {RESULT_WIN_RATIO[0], RESULT_WIN_RATIO[1], RESULT_WIN_RATIO[2]})){
        m_current = BattleResult::Win;
        return true;
    }
    if (is_solid(banner, {RESULT_LOSS_RATIO[0], RESULT_LOSS_RATIO[1], RESULT_LOSS_RATIO[2]})){
        m_current = BattleResult::Loss;
        return true;
    }
    return false;
}
void BattleResultDetector::commit_state(){
    m_committed = m_current;
}
void BattleResultDetector::reset_state(){
    m_current = BattleResult::Unknown;
    m_committed = BattleResult::Unknown;
}



}
}
}
