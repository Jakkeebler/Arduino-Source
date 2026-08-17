/*  Pokemon FRLG Learn-Move Dialog Reader
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include "Common/Cpp/Color.h"
#include "CommonFramework/ImageTypes/ImageViewRGB32.h"
#include "CommonFramework/ImageTools/ImageBoxes.h"
#include "CommonFramework/Logging/Logger.h"
#include "CommonFramework/VideoPipeline/VideoOverlayScopes.h"
#include "PokemonFRLG/PokemonFRLG_Settings.h"
#include "PokemonFRLG/Inference/PokemonFRLG_MoveNameOCR.h"
#include "PokemonFRLG_LearnMoveDialogReader.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


namespace{

//  Battle dialog text on FRLG is white-on-blue.
const std::vector<OCR::TextColorRange>& dialog_text_filters(){
    static const std::vector<OCR::TextColorRange> filters{
        {combine_rgb(224, 224, 224), combine_rgb(255, 255, 255)},
        {combine_rgb(208, 208, 208), combine_rgb(255, 255, 255)},
        {combine_rgb(192, 192, 192), combine_rgb(255, 255, 255)},
    };
    return filters;
}

}  //  namespace


LearnMoveDialogReader::LearnMoveDialogReader(Color color)
    : m_color(color)
    //  TODO: calibrate against a real "wants to learn <Move>" dialog screenshot.
    //  Covers the typical 2-line dialog box at the bottom of the battle screen.
    , m_box_dialog_text(0.060, 0.770, 0.880, 0.180)
{}

void LearnMoveDialogReader::make_overlays(VideoOverlaySet& items) const{
    const BoxOption& GAME_BOX = GameSettings::instance().GAME_BOX;
    items.add(m_color, GAME_BOX.inner_to_outer(m_box_dialog_text));
}

std::string LearnMoveDialogReader::read_new_move(
    Logger& logger, Language language, const ImageViewRGB32& frame
) const{
    ImageViewRGB32 game_screen = extract_box_reference(frame, GameSettings::instance().GAME_BOX);
    ImageViewRGB32 region = extract_box_reference(game_screen, m_box_dialog_text);

    //  read_move_slug applies the confidence floor, returns "" rather than
    //  guessing between look-alike short move names, and retries through the GBA
    //  pixel-font preprocessor on a miss. A wrong slug here makes the decider
    //  replace the wrong move, so an honest "" is much cheaper than a guess.
    return MoveNameOCR::instance().read_move_slug(
        logger, language, region, dialog_text_filters()
    );
}


}
}
}
