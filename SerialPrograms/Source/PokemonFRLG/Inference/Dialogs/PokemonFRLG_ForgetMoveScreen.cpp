/*  Pokemon FRLG Forget-Move Screen
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include "Common/Cpp/Color.h"
#include "CommonFramework/ImageTypes/ImageRGB32.h"
#include "CommonFramework/ImageTypes/ImageViewRGB32.h"
#include "CommonFramework/ImageTools/ImageBoxes.h"
#include "CommonFramework/ImageTools/ImageStats.h"
#include "CommonFramework/Logging/Logger.h"
#include "CommonFramework/VideoPipeline/VideoOverlayScopes.h"
#include "CommonTools/Images/ImageFilter.h"
#include "PokemonFRLG/PokemonFRLG_Settings.h"
#include "PokemonFRLG/Inference/PokemonFRLG_MoveNameOCR.h"
#include "PokemonFRLG_ForgetMoveScreen.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


namespace{

//  Forget-move move list is dark text on a near-white panel.
const std::vector<OCR::TextColorRange>& dark_text_filters(){
    static const std::vector<OCR::TextColorRange> filters{
        {combine_rgb(0, 0, 0), combine_rgb(80, 80, 80)},
        {combine_rgb(0, 0, 0), combine_rgb(120, 120, 120)},
    };
    return filters;
}

}  //  namespace


//  ----- Detector -----

ForgetMoveScreenDetector::ForgetMoveScreenDetector(Color color)
    : m_color(color)
    //  TODO: calibrate against a real forget-move screenshot.
    , m_box_panel  (0.060, 0.150, 0.880, 0.700)
    , m_box_header (0.060, 0.110, 0.880, 0.080)
{}

void ForgetMoveScreenDetector::make_overlays(VideoOverlaySet& items) const{
    const BoxOption& GAME_BOX = GameSettings::instance().GAME_BOX;
    items.add(m_color, GAME_BOX.inner_to_outer(m_box_panel));
    items.add(m_color, GAME_BOX.inner_to_outer(m_box_header));
}

bool ForgetMoveScreenDetector::detect(const ImageViewRGB32& screen){
    ImageViewRGB32 game_screen = extract_box_reference(screen, GameSettings::instance().GAME_BOX);
    ImageViewRGB32 panel = extract_box_reference(game_screen, m_box_panel);

    //  Heuristic: forget-move screen shows a near-white panel filling the
    //  bulk of the screen. Check the white-pixel density across the panel.
    const bool replace_color_within_range = false;
    const ImageRGB32 white_region = filter_rgb32_range(
        panel,
        combine_rgb(220, 220, 220), combine_rgb(255, 255, 255),
        Color(0), replace_color_within_range
    );
    const size_t white_pixels = image_stats(white_region).count;
    const double ratio = (double)white_pixels / (double)(panel.width() * panel.height());
    return ratio > 0.60;
}


//  ----- Reader -----

ForgetMoveScreenReader::ForgetMoveScreenReader(Color color)
    : m_color(color)
{
    //  TODO: calibrate against a real forget-move screenshot.
    //  Five-row vertical list (4 moves + Cancel). We only read the 4 moves.
    const double x = 0.150;
    const double w = 0.700;
    const double h = 0.110;
    const double y0 = 0.180;
    const double dy = 0.130;
    for (int i = 0; i < 4; i++){
        m_box_moves[i] = ImageFloatBox(x, y0 + dy * i, w, h);
    }
}

void ForgetMoveScreenReader::make_overlays(VideoOverlaySet& items) const{
    const BoxOption& GAME_BOX = GameSettings::instance().GAME_BOX;
    for (const ImageFloatBox& b : m_box_moves){
        items.add(m_color, GAME_BOX.inner_to_outer(b));
    }
}

std::array<std::string, 4> ForgetMoveScreenReader::read_moves(
    Logger& logger, Language language, const ImageViewRGB32& frame
) const{
    std::array<std::string, 4> result;
    ImageViewRGB32 game_screen = extract_box_reference(frame, GameSettings::instance().GAME_BOX);
    for (int i = 0; i < 4; i++){
        ImageViewRGB32 region = extract_box_reference(game_screen, m_box_moves[i]);
        OCR::StringMatchResult ocr = MoveNameOCR::instance().read_substring(
            logger, language, region, dark_text_filters()
        );
        if (!ocr.results.empty()){
            result[i] = ocr.results.begin()->second.token;
        }
    }
    return result;
}


}
}
}
