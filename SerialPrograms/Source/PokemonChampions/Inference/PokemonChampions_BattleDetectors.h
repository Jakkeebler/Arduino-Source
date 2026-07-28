/*  Pokemon Champions Battle Detectors
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#ifndef PokemonAutomation_PokemonChampions_BattleDetectors_H
#define PokemonAutomation_PokemonChampions_BattleDetectors_H

#include <chrono>
#include "Common/Cpp/Color.h"
#include "CommonFramework/ImageTools/ImageBoxes.h"
#include "CommonTools/VisualDetector.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonChampions{


//  NOTE: Every detector in this file is UNCALIBRATED. See the calibration block
//  at the top of PokemonChampions_BattleDetectors.cpp.


enum class BattleResult{
    Unknown,    //  Battle ended but the result could not be read.
    Win,
    Loss,
};
const char* battle_result_name(BattleResult result);


//  The in-battle move select UI -- the signal that it is our turn and the game
//  is waiting on input. This is the single highest value detector in the module:
//  without it the program can only mash A, and cannot tell "my turn" apart from
//  "waiting on the opponent".
class BattleMenuDetector : public StaticScreenDetector{
public:
    BattleMenuDetector(Color color = COLOR_RED);

    virtual void make_overlays(VideoOverlaySet& items) const override;
    virtual bool detect(const ImageViewRGB32& screen) override;

private:
    Color m_color;
};
class BattleMenuWatcher : public DetectorToFinder<BattleMenuDetector>{
public:
    BattleMenuWatcher(
        Color color = COLOR_RED,
        std::chrono::milliseconds hold_duration = std::chrono::milliseconds(250)
    )
        : DetectorToFinder("BattleMenuWatcher", hold_duration, color)
    {}
};


//  The forced switch prompt shown after our active Pokemon faints.
//
//  WARNING: do not guess which button is safe here. In FRLG the equivalent
//  prompt forfeits the whole battle if B is pressed (see the "Mid-battle forced
//  switch" section of CLAUDE.md). The Autobattle program only ever presses A on
//  this screen until the real button mapping is confirmed against a capture.
class ForcedSwitchDetector : public StaticScreenDetector{
public:
    ForcedSwitchDetector(Color color = COLOR_RED);

    virtual void make_overlays(VideoOverlaySet& items) const override;
    virtual bool detect(const ImageViewRGB32& screen) override;

private:
    Color m_color;
};
class ForcedSwitchWatcher : public DetectorToFinder<ForcedSwitchDetector>{
public:
    ForcedSwitchWatcher(
        Color color = COLOR_RED,
        std::chrono::milliseconds hold_duration = std::chrono::milliseconds(250)
    )
        : DetectorToFinder("ForcedSwitchWatcher", hold_duration, color)
    {}
};


//  The end-of-battle win/loss banner. Both results are read from the same box;
//  they are told apart by colour, so calibration needs a frame of each.
class BattleResultDetector : public StaticScreenDetector{
public:
    BattleResultDetector(Color color = COLOR_RED);

    virtual void make_overlays(VideoOverlaySet& items) const override;
    virtual bool detect(const ImageViewRGB32& screen) override;
    virtual void commit_state() override;
    virtual void reset_state() override;

    //  Valid only after the watcher has fired (i.e. after commit_state()).
    BattleResult result() const{ return m_committed; }

private:
    Color m_color;
    BattleResult m_current = BattleResult::Unknown;
    BattleResult m_committed = BattleResult::Unknown;
};
class BattleResultWatcher : public DetectorToFinder<BattleResultDetector>{
public:
    BattleResultWatcher(
        Color color = COLOR_RED,
        std::chrono::milliseconds hold_duration = std::chrono::milliseconds(250)
    )
        : DetectorToFinder("BattleResultWatcher", hold_duration, color)
    {}
};



}
}
}
#endif
