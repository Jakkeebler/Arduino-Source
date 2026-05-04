/*  Pokemon FRLG Learn-Move Dialog Reader
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  OCR reader for the in-battle / post-battle "<Pokemon> wants to learn
 *  <Move>!" dialog. Returns the new move's slug.
 *
 *  The dialog text region float-box is a placeholder estimate and will need
 *  calibration on a real screenshot.
 */

#ifndef PokemonAutomation_PokemonFRLG_LearnMoveDialogReader_H
#define PokemonAutomation_PokemonFRLG_LearnMoveDialogReader_H

#include <string>
#include "Common/Cpp/Color.h"
#include "CommonFramework/ImageTools/ImageBoxes.h"
#include "CommonFramework/Language.h"

namespace PokemonAutomation{

class Logger;
class ImageViewRGB32;
class VideoOverlaySet;

namespace NintendoSwitch{
namespace PokemonFRLG{


class LearnMoveDialogReader{
public:
    LearnMoveDialogReader(Color color = COLOR_RED);

    void make_overlays(VideoOverlaySet& items) const;

    //  Reads the new move name from the "wants to learn" dialog.
    //  Returns the move slug, or an empty string on failure.
    std::string read_new_move(
        Logger& logger, Language language, const ImageViewRGB32& frame
    ) const;

private:
    Color m_color;
    //  TODO: calibrate against a real "wants to learn" dialog screenshot.
    ImageFloatBox m_box_dialog_text;
};


}
}
}
#endif
