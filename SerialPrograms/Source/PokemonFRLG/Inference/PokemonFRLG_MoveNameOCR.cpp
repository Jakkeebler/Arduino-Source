/*  Pokemon FRLG Move Name OCR
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include "Common/Cpp/Color.h"
#include "Common/Cpp/Json/JsonValue.h"
#include "Common/Cpp/Json/JsonArray.h"
#include "Common/Cpp/Json/JsonObject.h"
#include "Common/Cpp/Strings/Unicode.h"
#include "CommonFramework/ImageTypes/ImageRGB32.h"
#include "CommonFramework/Logging/Logger.h"
#include "PokemonFRLG/Inference/PokemonFRLG_DigitReader.h"
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

namespace{

//  Filters for a region that has already been run through preprocess_for_ocr,
//  which emits black text on a white background regardless of the source colours.
const std::vector<OCR::TextColorRange>& preprocessed_text_filters(){
    static const std::vector<OCR::TextColorRange> filters{
        {combine_rgb(0, 0, 0), combine_rgb(64, 64, 64)},
        {combine_rgb(0, 0, 0), combine_rgb(128, 128, 128)},
    };
    return filters;
}

//  Collapse a match result to a single trustworthy slug, or "" if there isn't
//  one. Ambiguity (two or more candidates inside the spread) is treated as a
//  failed read, not as a reason to guess.
std::string resolve_unique(Logger& logger, const OCR::StringMatchResult& result){
    if (result.results.empty()){
        return std::string();
    }
    if (result.results.size() > 1){
        std::string candidates;
        for (const auto& item : result.results){
            if (!candidates.empty()){
                candidates += ", ";
            }
            candidates += item.second.token;
        }
        logger.log(
            "MoveNameOCR: ambiguous read - " + std::to_string(result.results.size()) +
            " candidates within the confidence spread [" + candidates +
            "]. Treating as unknown rather than guessing.",
            COLOR_RED
        );
        return std::string();
    }
    return result.results.begin()->second.token;
}

}  //  namespace

std::string MoveNameOCR::read_move_slug(
    Logger& logger,
    Language language,
    const ImageViewRGB32& image,
    const std::vector<OCR::TextColorRange>& text_color_ranges,
    double min_text_ratio, double max_text_ratio
) const{
    //  Pass 1: the region as captured, with the caller's colour filters.
    std::string slug = resolve_unique(
        logger,
        read_substring(logger, language, image, text_color_ranges, min_text_ratio, max_text_ratio)
    );
    if (!slug.empty()){
        return slug;
    }

    //  Pass 2: run the GBA pixel-font preprocessor and try again. FRLG text is a
    //  small seven-segment-style font with 1px gaps between segments that
    //  Tesseract reads poorly at native scale; this pipeline (native-res blur to
    //  bridge the gaps -> 4x upscale -> threshold -> close -> pad) was written for
    //  exactly this problem but had never been wired into the move readers.
    //
    //  in_range_black = true with a high band maps light text to black, giving
    //  black-on-white output whichever way round the source was.
    try{
        ImageRGB32 cleaned = preprocess_for_ocr(
            image, "movename",
            /*blur_kernel_size=*/5, /*blur_passes=*/2,
            /*in_range_black=*/true,
            /*bw_min=*/combine_rgb(150, 150, 150),
            /*bw_max=*/combine_rgb(255, 255, 255)
        );
        slug = resolve_unique(
            logger,
            read_substring(
                logger, language, cleaned, preprocessed_text_filters(),
                min_text_ratio, max_text_ratio
            )
        );
        if (!slug.empty()){
            logger.log("MoveNameOCR: recovered '" + slug + "' via GBA preprocessing pass.");
            return slug;
        }
    }catch (...){
        //  Preprocessing is a bonus path. If it throws, fall through and report
        //  the honest "unknown" that pass 1 already produced.
        logger.log("MoveNameOCR: GBA preprocessing pass failed; reporting unknown.", COLOR_RED);
    }

    return std::string();
}


}
}
}
