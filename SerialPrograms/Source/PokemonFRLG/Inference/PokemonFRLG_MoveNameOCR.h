/*  Pokemon FRLG Move Name OCR
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Singleton dictionary matcher whose candidate set is built at construction
 *  time from the FRLG move database (Resources/PokemonFRLG/Data/Moves.json).
 *
 *  Mirrors PokemonSV::PokemonMovesOCR but sources its dictionary from the
 *  runtime move table instead of a side OCR JSON, so there is one source of
 *  truth for move names.
 *
 */

#ifndef PokemonAutomation_PokemonFRLG_MoveNameOCR_H
#define PokemonAutomation_PokemonFRLG_MoveNameOCR_H

#include "CommonFramework/Language.h"
#include "CommonTools/OCR/OCR_SmallDictionaryMatcher.h"

namespace PokemonAutomation{
    class ImageViewRGB32;
    class Logger;
namespace NintendoSwitch{
namespace PokemonFRLG{


class MoveNameOCR : public OCR::SmallDictionaryMatcher{
    static constexpr double MAX_LOG10P = -1.30;
    static constexpr double MAX_LOG10P_SPREAD = 0.50;

public:
    MoveNameOCR();

    static MoveNameOCR& instance();

    OCR::StringMatchResult read_substring(
        Logger& logger,
        Language language,
        const ImageViewRGB32& image,
        const std::vector<OCR::TextColorRange>& text_color_ranges,
        double min_text_ratio = 0.01, double max_text_ratio = 0.50
    ) const;
};


}
}
}
#endif
