/*  Pokemon Champions Menu Detectors
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#ifndef PokemonAutomation_PokemonChampions_MenuDetectors_H
#define PokemonAutomation_PokemonChampions_MenuDetectors_H

#include <chrono>
#include "Common/Cpp/Color.h"
#include "CommonFramework/ImageTools/ImageBoxes.h"
#include "CommonTools/VisualDetector.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonChampions{


//  NOTE: Every detector in this file is UNCALIBRATED. See the calibration block
//  at the top of PokemonChampions_MenuDetectors.cpp. Until real frames of the
//  game are measured, detect() always returns false, so every caller must have
//  a timeout / fallback path rather than blocking on these forever.


//  The main menu / hub screen. This is the "known good" anchor state that all
//  recovery paths steer back to.
class MainMenuDetector : public StaticScreenDetector{
public:
    MainMenuDetector(Color color = COLOR_RED);

    virtual void make_overlays(VideoOverlaySet& items) const override;
    virtual bool detect(const ImageViewRGB32& screen) override;

private:
    Color m_color;
};
class MainMenuWatcher : public DetectorToFinder<MainMenuDetector>{
public:
    MainMenuWatcher(
        Color color = COLOR_RED,
        std::chrono::milliseconds hold_duration = std::chrono::milliseconds(250)
    )
        : DetectorToFinder("MainMenuWatcher", hold_duration, color)
    {}
};


//  The "searching for an opponent" matchmaking screen. Used to tell a genuine
//  long queue apart from a hung screen, so we don't kill a healthy 4 minute
//  matchmaking wait.
class MatchmakingDetector : public StaticScreenDetector{
public:
    MatchmakingDetector(Color color = COLOR_RED);

    virtual void make_overlays(VideoOverlaySet& items) const override;
    virtual bool detect(const ImageViewRGB32& screen) override;

private:
    Color m_color;
};
class MatchmakingWatcher : public DetectorToFinder<MatchmakingDetector>{
public:
    MatchmakingWatcher(
        Color color = COLOR_RED,
        std::chrono::milliseconds hold_duration = std::chrono::milliseconds(250)
    )
        : DetectorToFinder("MatchmakingWatcher", hold_duration, color)
    {}
};



}
}
}
#endif
