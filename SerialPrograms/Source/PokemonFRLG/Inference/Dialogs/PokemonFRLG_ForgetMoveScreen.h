/*  Pokemon FRLG Forget-Move Screen
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Detector + reader for the "Which move should be forgotten?" screen that
 *  appears after the player accepts the move-learn prompt. The screen lists
 *  the four currently-known moves plus a Cancel option; this reader OCR's
 *  the four current moves so the smart-learn engine can pick which slot to
 *  replace.
 *
 *  Float-box constants are placeholder estimates and will need calibration
 *  on a real screenshot.
 */

#ifndef PokemonAutomation_PokemonFRLG_ForgetMoveScreen_H
#define PokemonAutomation_PokemonFRLG_ForgetMoveScreen_H

#include <array>
#include <chrono>
#include <string>
#include "Common/Cpp/Color.h"
#include "CommonFramework/ImageTools/ImageBoxes.h"
#include "CommonFramework/Language.h"
#include "CommonTools/VisualDetector.h"

namespace PokemonAutomation{

class Logger;
class ImageViewRGB32;
class VideoOverlaySet;

namespace NintendoSwitch{
namespace PokemonFRLG{


//  Detects the "Which move should be forgotten?" screen by sampling the
//  background colour band where the move list panel lives.
class ForgetMoveScreenDetector : public StaticScreenDetector{
public:
    ForgetMoveScreenDetector(Color color = COLOR_RED);

    virtual void make_overlays(VideoOverlaySet& items) const override;
    virtual bool detect(const ImageViewRGB32& screen) override;

private:
    Color m_color;
    //  TODO: calibrate against a real forget-move screenshot.
    ImageFloatBox m_box_panel;
    ImageFloatBox m_box_header;
};
class ForgetMoveScreenWatcher : public DetectorToFinder<ForgetMoveScreenDetector>{
public:
    ForgetMoveScreenWatcher(Color color = COLOR_RED)
        : DetectorToFinder("ForgetMoveScreenWatcher", std::chrono::milliseconds(250), color)
    {}
};


class ForgetMoveScreenReader{
public:
    ForgetMoveScreenReader(Color color = COLOR_RED);

    void make_overlays(VideoOverlaySet& items) const;

    //  Returns the four currently-known move slugs, top-to-bottom.
    //  Cancel option is ignored. Empty string in any slot means OCR failed.
    std::array<std::string, 4> read_moves(
        Logger& logger, Language language, const ImageViewRGB32& frame
    ) const;

private:
    Color m_color;
    //  TODO: calibrate against a real forget-move screenshot.
    std::array<ImageFloatBox, 4> m_box_moves;
};


}
}
}
#endif
