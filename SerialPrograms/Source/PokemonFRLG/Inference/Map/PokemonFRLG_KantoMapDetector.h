/*  Kanto Map Detector
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Locates the player on the combined Kanto map (Viridian + Route 1 + Pallet
 *  stitched into one image) by template-matching the live game viewport.
 *  Returns global tile coordinates and a confidence score.
 */

#ifndef PokemonAutomation_PokemonFRLG_KantoMapDetector_H
#define PokemonAutomation_PokemonFRLG_KantoMapDetector_H

#include <cstdint>
#include <optional>
#include <vector>
#include <opencv2/core.hpp>
#include "CommonFramework/ImageTypes/ImageViewRGB32.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

//  Logical sub-region of the combined Kanto map. Used only for log messages.
enum class KantoRegion{
    ViridianCity,
    Route1,
    PalletTown,
    ViridianForest,  //  Interior, stitched into the bottom-right corner of
                     //  the combined map at tile cols 354..407, rows 331..399.
    OffMap,          //  Outside any of the named sub-maps (tree-fill border etc.).
};

const char* kanto_region_name(KantoRegion r);
KantoRegion kanto_region_at(int tile_x, int tile_y);

struct KantoPosition{
    //  Tile the PLAYER is standing on, in combined-map coordinates. The map is
    //  408x400 tiles (6528x6400 px at 16 px/tile), so 0..407 and 0..399.
    int tile_x;          //  Combined-map column.
    int tile_y;          //  Combined-map row.
    double confidence;   //  TM_CCOEFF_NORMED score in [-1, 1].
};


class KantoMapDetector{
public:
    //  Singleton. The combined map is loaded once at first access.
    static const KantoMapDetector& instance();

    //  Locate the player. Returns nullopt if the best score is below
    //  min_confidence. Default 0.40 catches real overworld matches; off-map
    //  content (battle, dialogs, fades) tends to score < 0.4.
    //
    //  If `hint_x` and `hint_y` are >= 0 and `hint_radius_tiles` > 0, only
    //  the (2*radius)x(2*radius) square of the map centered on (hint_x,
    //  hint_y) is searched. This cuts matchTemplate time from ~1-2 s on
    //  the full Kanto map to ~50 ms. Returns nullopt if the constrained
    //  search fails OR if best is below min_confidence; caller can fall
    //  back to a full-map search.
    std::optional<KantoPosition> locate(
        const ImageViewRGB32& screen,
        double min_confidence = 0.40,
        int hint_x = -1, int hint_y = -1,
        int hint_radius_tiles = 0
    ) const;

    int map_width_tiles() const { return m_width_tiles; }
    int map_height_tiles() const { return m_height_tiles; }

    //  --- Unrendered ("void") regions of the combined map -------------------
    //
    //  Kanto-Combined.png is a stitch of several sub-maps, and the areas
    //  between and beyond them are left pure white. The game, standing at the
    //  edge of a sub-map, renders its border block there instead -- so any
    //  viewport that overlaps a void is being compared against a template that
    //  is partly meaningless, and its correlation score is depressed in
    //  proportion.
    //
    //  This is not hypothetical. Route1SouthGrass{78,235} sits 6 tiles from the
    //  void that starts at x=84 on that row, so its viewport always included
    //  2 white columns and never scored better than ~0.76 (vs 0.98 on interior
    //  ground). A couple of tiles of drift east took it under the 0.40 floor
    //  and localization stopped working entirely, mid-run, on 2026-08-18.

    //  True if this tile has real map content; false if it is unrendered.
    bool tile_rendered(int tile_x, int tile_y) const;

    //  Fraction (0..1) of the 15x10-tile viewport centred on this player tile
    //  that is unrendered. 0.0 means the template is entirely valid.
    double viewport_void_fraction(int tile_x, int tile_y) const;

private:
    KantoMapDetector();
    void build_rendered_mask();

    cv::Mat m_combined;   //  BGR.
    int m_width_tiles = 0;
    int m_height_tiles = 0;

    //  One byte per tile, row-major, m_width_tiles * m_height_tiles.
    //  1 = rendered, 0 = void. Built once at construction.
    std::vector<uint8_t> m_rendered;
};


}
}
}
#endif
