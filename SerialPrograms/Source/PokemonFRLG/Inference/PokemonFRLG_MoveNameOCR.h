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

#include <string>
#include <vector>
#include "CommonFramework/Language.h"
#include "CommonTools/OCR/OCR_SmallDictionaryMatcher.h"

namespace PokemonAutomation{
    class ImageViewRGB32;
    class Logger;
namespace NintendoSwitch{
namespace PokemonFRLG{


class MoveNameOCR : public OCR::SmallDictionaryMatcher{
    //  Confidence floor for accepting a dictionary match, and how close a
    //  runner-up may be before the read counts as ambiguous.
    //
    //  These were -1.30 / 0.50, looser than the rest of the project uses. On the
    //  GBA's small pixel font that was loose enough for garbage Tesseract output
    //  to still land in range of some short move name ("Cut", "Bide", "Dig",
    //  "Rest"), so a blurred glyph yielded a *confidently wrong* slug instead of
    //  an honest "unknown" -- and the wrong move then got pinned, or the wrong
    //  slot forgotten.
    static constexpr double MAX_LOG10P = -1.60;
    static constexpr double MAX_LOG10P_SPREAD = 0.30;

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

    //  Preferred entry point: read a move name, snap it to the closed 355-move
    //  vocabulary, and return the slug -- or "" for "could not read this".
    //
    //  Unlike taking results.begin()->second.token directly, this returns ""
    //  when two or more candidates survive the spread, because picking one of
    //  those is a coin flip. Every consumer downstream already handles ""
    //  correctly as "unknown" (see MoveLearnDecider::decide_accept_or_decline).
    //
    //  If the first pass finds nothing, the region is re-read through the GBA
    //  pixel-font preprocessor (preprocess_for_ocr) and matched again against
    //  black-on-white filters. That second pass can only recover reads the first
    //  pass missed -- it never overrides a confident first-pass result.
    std::string read_move_slug(
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
