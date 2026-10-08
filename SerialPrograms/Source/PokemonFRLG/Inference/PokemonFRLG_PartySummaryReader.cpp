/*  Pokemon FRLG Party Summary Reader
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include <set>
#include "Common/Cpp/Color.h"
#include "CommonFramework/ImageTypes/ImageViewRGB32.h"
#include "CommonFramework/ImageTools/ImageBoxes.h"
#include "CommonFramework/Logging/Logger.h"
#include "CommonFramework/VideoPipeline/VideoOverlayScopes.h"
#include "Pokemon/Inference/Pokemon_NameReader.h"
#include "PokemonFRLG/PokemonFRLG_Settings.h"
#include "PokemonFRLG/Resources/PokemonFRLG_SpeciesData.h"
#include "PokemonFRLG_DigitReader.h"
#include "PokemonFRLG_MoveNameOCR.h"
#include "PokemonFRLG_PartySummaryReader.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


namespace{

//  FRLG Summary text colour bands. Page 1 nickname is white-on-blue; moves on
//  page 3 are dark-on-light. Two filter sets cover both cases.
const std::vector<OCR::TextColorRange>& white_text_filters(){
    static const std::vector<OCR::TextColorRange> filters{
        {combine_rgb(224, 224, 224), combine_rgb(255, 255, 255)},
        {combine_rgb(208, 208, 208), combine_rgb(255, 255, 255)},
        {combine_rgb(192, 192, 192), combine_rgb(255, 255, 255)},
    };
    return filters;
}
const std::vector<OCR::TextColorRange>& dark_text_filters(){
    static const std::vector<OCR::TextColorRange> filters{
        {combine_rgb(0, 0, 0), combine_rgb(80, 80, 80)},
        {combine_rgb(0, 0, 0), combine_rgb(120, 120, 120)},
    };
    return filters;
}

std::string best_token(const OCR::StringMatchResult& result){
    if (result.results.empty()){
        return std::string();
    }
    return result.results.begin()->second.token;
}

//  Cached PokemonNameReader scoped to FRLG species only. Constraining the
//  OCR dictionary to ~150 Kanto slugs (vs the global ~1000+ multi-gen list)
//  fixes the fuzzy-match misreads we hit with the global instance — e.g. an
//  un-nicknamed "WEEDLE" no longer matches "meditite" because meditite isn't
//  in the FRLG subset to begin with.
const Pokemon::PokemonNameReader& frlg_name_reader(){
    static const Pokemon::PokemonNameReader reader = []{
        std::set<std::string> subset;
        for (const SpeciesData& s : all_species()){
            subset.insert(s.slug);
        }
        return Pokemon::PokemonNameReader(subset);
    }();
    return reader;
}

}  //  namespace


PartySummaryReader::PartySummaryReader(Color color)
    : m_color(color)
    //  Lilac left-panel region: nickname/species text spans the wider area;
    //  level digits live to the right of the "Lv" prefix glyphs. We OCR only
    //  the digit cells — including the "Lv" prefix in the level box causes
    //  the waterfill matcher to template-match "L"/"v" glyphs against digits
    //  (e.g. "L" reads as "7"), so this box is tight around just the digits.
    , m_box_level    (0.055, 0.110, 0.080, 0.080)
    , m_box_nickname (0.150, 0.105, 0.260, 0.080)
    //  Page-1-only: the "No XXX" digits in the right-side info panel.
    //  Tightened to skip the "No" badge curve and the "NAME" row below,
    //  but loose enough to comfortably hold all 3 digits.
    , m_box_dex_no   (0.690, 0.135, 0.085, 0.075)
{
    //  4 vertically-stacked move-name regions on the right half of the screen.
    //  Row spacing on FRLG Summary page 3 is ~0.17. Boxes are uniform width
    //  and height across all 4 rows, sized generously to fully cover the
    //  move-name text including longer names like "POISONPOWDER".
    const double x = 0.535;
    const double w = 0.450;
    const double h = 0.130;
    m_box_moves[0] = ImageFloatBox(x, 0.150, w, h);
    m_box_moves[1] = ImageFloatBox(x, 0.320, w, h);
    m_box_moves[2] = ImageFloatBox(x, 0.490, w, h);
    m_box_moves[3] = ImageFloatBox(x, 0.660, w, h);
}

void PartySummaryReader::make_overlays(VideoOverlaySet& items) const{
    const BoxOption& GAME_BOX = GameSettings::instance().GAME_BOX;
    items.add(m_color, GAME_BOX.inner_to_outer(m_box_level));
    items.add(m_color, GAME_BOX.inner_to_outer(m_box_nickname));
    items.add(m_color, GAME_BOX.inner_to_outer(m_box_dex_no));
    for (const ImageFloatBox& b : m_box_moves){
        items.add(m_color, GAME_BOX.inner_to_outer(b));
    }
}

void PartySummaryReader::read_page1(
    Logger& logger, Language language,
    const ImageViewRGB32& frame, PartySummaryRead& out
) const{
    ImageViewRGB32 game_screen = extract_box_reference(frame, GameSettings::instance().GAME_BOX);

    //  Level digits in the lilac left-side panel.
    ImageViewRGB32 level_region = extract_box_reference(game_screen, m_box_level);
    out.level = read_digits_waterfill_template(
        logger, level_region,
        /*rmsd_threshold*/ 175.0,
        DigitTemplateType::LevelBox,
        "summary_level",
        /*binarize_high*/ 0x7F
    );

    //  Actual Pokedex No. on the right-side info panel (page 1 only).
    //  The badge has a gray label tile + digits on a near-white background;
    //  StatBox template + default binarize covers this.
    ImageViewRGB32 dex_region = extract_box_reference(game_screen, m_box_dex_no);
    out.dex_no = read_digits_waterfill_template(
        logger, dex_region,
        /*rmsd_threshold*/ 175.0,
        DigitTemplateType::StatBox,
        "summary_dex_no"
    );

    //  Nickname: FRLG-subset Pokemon name OCR matcher (constrained dictionary
    //  reduces fuzzy mismatches against gen 5+ species names).
    auto name_result = frlg_name_reader().read_substring(
        logger, language,
        extract_box_reference(game_screen, m_box_nickname),
        white_text_filters()
    );
    out.nickname = best_token(name_result);
}

std::string PartySummaryReader::read_page3_single_move(
    Logger& logger, Language language,
    const ImageViewRGB32& frame, int move_index_0_to_3
) const{
    if (move_index_0_to_3 < 0 || move_index_0_to_3 >= 4){
        return std::string();
    }
    ImageViewRGB32 game_screen = extract_box_reference(frame, GameSettings::instance().GAME_BOX);
    ImageViewRGB32 region = extract_box_reference(game_screen, m_box_moves[move_index_0_to_3]);

    //  Try dark-text filters first (Summary page 3 is typically light-on-dark).
    //  read_move_slug returns "" on an ambiguous match rather than picking between
    //  look-alike short move names, and retries through the GBA pixel-font
    //  preprocessor before giving up -- so an empty result here really does mean
    //  "unreadable", which is what the caller's move cache should record.
    std::string slug = MoveNameOCR::instance().read_move_slug(
        logger, language, region, dark_text_filters()
    );
    if (slug.empty()){
        //  Fall back to white-text in case the screen variant differs.
        slug = MoveNameOCR::instance().read_move_slug(
            logger, language, region, white_text_filters()
        );
    }
    return slug;
}

void PartySummaryReader::read_page3_moves(
    Logger& logger, Language language,
    const ImageViewRGB32& frame, PartySummaryRead& out
) const{
    for (int i = 0; i < 4; i++){
        out.move_slugs[i] = read_page3_single_move(logger, language, frame, i);
    }
}


}
}
}
