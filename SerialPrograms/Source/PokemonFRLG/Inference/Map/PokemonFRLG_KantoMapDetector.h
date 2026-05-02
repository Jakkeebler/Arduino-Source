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

#include <optional>
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
    OffMap,  //  Outside any of the three sub-maps (e.g. tree-fill border).
};

const char* kanto_region_name(KantoRegion r);
KantoRegion kanto_region_at(int tile_x, int tile_y);

struct KantoPosition{
    int tile_x;          //  Combined-map column. 0..47
    int tile_y;          //  Combined-map row.    0..99
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

private:
    KantoMapDetector();

    cv::Mat m_combined;   //  BGR.
    int m_width_tiles = 0;
    int m_height_tiles = 0;
};


}
}
}
#endif
