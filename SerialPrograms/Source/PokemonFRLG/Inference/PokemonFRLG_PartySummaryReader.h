/*  Pokemon FRLG Party Summary Reader
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  OCR readers for the in-game Summary screen (the screen reached via
 *  PARTY -> A -> SUMMARY). Reads:
 *
 *      - Page 1: National Pokedex number (digit OCR) + nickname (text OCR).
 *      - Page 3: the Pokemon's four current moves by name.
 *
 *  Float-box constants are placeholder estimates and will need calibration
 *  on real screenshots. See the FRLG "Read Party" developer program.
 */

#ifndef PokemonAutomation_PokemonFRLG_PartySummaryReader_H
#define PokemonAutomation_PokemonFRLG_PartySummaryReader_H

#include <array>
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


struct PartySummaryRead{
    int level  = -1;                          //  Level digits (left-side lilac panel). -1 if not detected.
    int dex_no = -1;                          //  Actual National Pokedex No. from page-1 info panel. -1 if not detected.
    std::string nickname;                     //  empty if not detected
    std::array<std::string, 4> move_slugs;    //  empty string for any failed read
};


class PartySummaryReader{
public:
    PartySummaryReader(Color color = COLOR_RED);

    void make_overlays(VideoOverlaySet& items) const;

    //  Page 1 (Pokemon Info): nickname + dex#.
    //  Sets `dex_no` and `nickname` on `out`. Other fields untouched.
    void read_page1(
        Logger& logger, Language language,
        const ImageViewRGB32& frame, PartySummaryRead& out
    ) const;

    //  Page 3 (Skills/Moves): the four current move names.
    //  Sets `move_slugs` on `out`. Other fields untouched.
    void read_page3_moves(
        Logger& logger, Language language,
        const ImageViewRGB32& frame, PartySummaryRead& out
    ) const;

    //  Single-move helper if you only need one slot's slug.
    //  Returns an empty string on failure.
    std::string read_page3_single_move(
        Logger& logger, Language language,
        const ImageViewRGB32& frame, int move_index_0_to_3
    ) const;

private:
    Color m_color;

    //  Page 1 / page 3 left-panel regions (level + species/nickname text are
    //  visible at the top of both pages, in the lilac panel).
    ImageFloatBox m_box_level;
    ImageFloatBox m_box_nickname;

    //  Page 1 only: the "No XXX" Pokedex number in the right-side info panel.
    //  Read this to identify species reliably regardless of nickname.
    ImageFloatBox m_box_dex_no;

    //  Page 3 regions: 4 vertically-stacked move name boxes.
    std::array<ImageFloatBox, 4> m_box_moves;
};


}
}
}
#endif
