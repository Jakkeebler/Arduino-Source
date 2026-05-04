/*  Pokemon FRLG Move Name OCR
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include "Common/Cpp/Json/JsonValue.h"
#include "Common/Cpp/Json/JsonArray.h"
#include "Common/Cpp/Json/JsonObject.h"
#include "Common/Cpp/Strings/Unicode.h"
#include "PokemonFRLG/Resources/PokemonFRLG_MoveData.h"
#include "PokemonFRLG_MoveNameOCR.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


namespace{

//  Build a JsonObject in the layout SmallDictionaryMatcher expects:
//      { "<lang_code>": { "<token>": ["<candidate1>", ...] } }
//  Sourced from the FRLG move data so that Moves.json is the single source of
//  truth — adding a move there automatically makes it OCR-recognisable.
JsonObject build_move_ocr_json(){
    JsonObject eng_entries;
    for (const MoveData& m : all_moves()){
        JsonArray candidates;
        candidates.push_back(JsonValue(m.display_eng));
        eng_entries[m.slug] = JsonValue(std::move(candidates));
    }
    JsonObject root;
    root["eng"] = JsonValue(std::move(eng_entries));
    return root;
}

}  //  namespace


MoveNameOCR::MoveNameOCR()
    : OCR::SmallDictionaryMatcher(build_move_ocr_json())
{}

MoveNameOCR& MoveNameOCR::instance(){
    static MoveNameOCR matcher;
    return matcher;
}

OCR::StringMatchResult MoveNameOCR::read_substring(
    Logger& logger,
    Language language,
    const ImageViewRGB32& image,
    const std::vector<OCR::TextColorRange>& text_color_ranges,
    double min_text_ratio, double max_text_ratio
) const{
    return match_substring_from_image_multifiltered(
        &logger, language, image, text_color_ranges,
        MAX_LOG10P, MAX_LOG10P_SPREAD, min_text_ratio, max_text_ratio
    );
}


}
}
}
