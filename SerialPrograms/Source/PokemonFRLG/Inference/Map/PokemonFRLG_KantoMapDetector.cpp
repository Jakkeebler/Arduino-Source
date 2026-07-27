/*  Kanto Map Detector
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include <atomic>
#include <algorithm>
#include <cmath>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include "Common/Cpp/Filesystem.h"
#include "CommonFramework/Globals.h"
#include "CommonFramework/Logging/Logger.h"
#include "PokemonFRLG_KantoMapDetector.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

const char* kanto_region_name(KantoRegion r){
    switch (r){
    case KantoRegion::ViridianCity:   return "Viridian City";
    case KantoRegion::Route1:         return "Route 1";
    case KantoRegion::PalletTown:     return "Pallet Town";
    case KantoRegion::ViridianForest: return "Viridian Forest";
    case KantoRegion::OffMap:         return "off-map";
    }
    return "?";
}

//  Approximate sub-region boundaries on the full Kanto map.
//  Used only for log labels - navigation operates on global tile coords.
KantoRegion kanto_region_at(int tile_x, int tile_y){
    if (tile_y >= 180 && tile_y < 215 && tile_x >= 50 && tile_x < 100){
        return KantoRegion::ViridianCity;
    }
    if (tile_y >= 215 && tile_y < 260 && tile_x >= 55 && tile_x < 85){
        return KantoRegion::Route1;
    }
    if (tile_y >= 260 && tile_y < 285 && tile_x >= 55 && tile_x < 85){
        return KantoRegion::PalletTown;
    }
    //  Stitched-in interior region (bottom-right corner of the combined map).
    if (tile_y >= 331 && tile_y < 400 && tile_x >= 354 && tile_x < 408){
        return KantoRegion::ViridianForest;
    }
    return KantoRegion::OffMap;
}

namespace{

constexpr int TILE_PX = 16;
constexpr int VIEWPORT_W_TILES = 15;
constexpr int VIEWPORT_H_TILES = 10;
constexpr const char* MAP_RELATIVE_PATH = "PokemonFRLG/Maps/Kanto-Combined.png";

//  Ambiguity rejection: when measuring the second-best correlation peak, ignore
//  everything within this radius of the best peak (that area is the same peak's
//  shoulder, not a distinct location).
constexpr int SUPPRESS_RADIUS_PX = 4 * TILE_PX;
//  The best peak must beat the next *distinct* peak by at least this much
//  (TM_CCOEFF_NORMED units) to be accepted. Guards against repetitive terrain
//  where several map locations look near-identical.
constexpr double AMBIGUITY_MARGIN = 0.06;

cv::Rect detect_letterbox_roi(const cv::Mat& bgr, int threshold = 10){
    cv::Mat gray;
    cv::cvtColor(bgr, gray, cv::COLOR_BGR2GRAY);

    int x0 = 0, x1 = bgr.cols - 1;
    int y0 = 0, y1 = bgr.rows - 1;

    auto col_is_black = [&](int x){
        for (int y = 0; y < gray.rows; ++y){
            if (gray.at<uint8_t>(y, x) > threshold) return false;
        }
        return true;
    };
    auto row_is_black = [&](int y){
        for (int x = 0; x < gray.cols; ++x){
            if (gray.at<uint8_t>(y, x) > threshold) return false;
        }
        return true;
    };
    while (x0 < x1 && col_is_black(x0)) ++x0;
    while (x1 > x0 && col_is_black(x1)) --x1;
    while (y0 < y1 && row_is_black(y0)) ++y0;
    while (y1 > y0 && row_is_black(y1)) --y1;

    return cv::Rect(x0, y0, x1 - x0 + 1, y1 - y0 + 1);
}

}  // namespace


const KantoMapDetector& KantoMapDetector::instance(){
    static KantoMapDetector singleton;
    return singleton;
}

KantoMapDetector::KantoMapDetector(){
    std::string full_path = RESOURCE_PATH() + MAP_RELATIVE_PATH;
    m_combined = cv::imread(full_path, cv::IMREAD_COLOR);
    if (m_combined.empty()){
        global_logger_tagged().log(
            std::string("KantoMapDetector: failed to load combined map at ") + full_path,
            COLOR_RED
        );
        return;
    }
    m_width_tiles = m_combined.cols / TILE_PX;
    m_height_tiles = m_combined.rows / TILE_PX;
}

std::optional<KantoPosition> KantoMapDetector::locate(
    const ImageViewRGB32& screen, double min_confidence,
    int hint_x, int hint_y, int hint_radius_tiles
) const{
    if (m_combined.empty() || !screen){
        return std::nullopt;
    }

    cv::Mat bgra = screen.to_opencv_Mat();
    if (bgra.empty()){
        return std::nullopt;
    }
    cv::Mat bgr;
    cv::cvtColor(bgra, bgr, cv::COLOR_BGRA2BGR);

    cv::Rect crop_roi = detect_letterbox_roi(bgr);
    if (crop_roi.width < 100 || crop_roi.height < 60){
        return std::nullopt;
    }
    cv::Mat content = bgr(crop_roi);

    cv::Mat templ;
    cv::resize(
        content, templ,
        cv::Size(VIEWPORT_W_TILES * TILE_PX, VIEWPORT_H_TILES * TILE_PX),
        0, 0, cv::INTER_AREA
    );

    if (templ.cols > m_combined.cols || templ.rows > m_combined.rows){
        return std::nullopt;
    }

    //  Constrain the search to a window around the hint, if given. Window is
    //  expressed in tile units; convert to a pixel ROI, clamped to the map
    //  bounds and large enough to fit the template.
    cv::Mat search_image;
    int search_offset_x = 0, search_offset_y = 0;
    bool used_hint = false;
    if (hint_radius_tiles > 0 && hint_x >= 0 && hint_y >= 0){
        int radius_px = hint_radius_tiles * TILE_PX;
        int sx = hint_x * TILE_PX - radius_px;
        int sy = hint_y * TILE_PX - radius_px;
        int sw = 2 * radius_px;
        int sh = 2 * radius_px;
        //  Clamp to map bounds.
        if (sx < 0){ sw += sx; sx = 0; }
        if (sy < 0){ sh += sy; sy = 0; }
        if (sx + sw > m_combined.cols){ sw = m_combined.cols - sx; }
        if (sy + sh > m_combined.rows){ sh = m_combined.rows - sy; }
        if (sw >= templ.cols && sh >= templ.rows){
            search_image = m_combined(cv::Rect(sx, sy, sw, sh));
            search_offset_x = sx;
            search_offset_y = sy;
            used_hint = true;
        }
    }
    if (search_image.empty()){
        search_image = m_combined;
    }

    //  Run matchTemplate over `search` (with global pixel offset off_x/off_y),
    //  returning the best peak's confidence, the best *distinct* competing peak
    //  (for ambiguity rejection), and the sub-pixel-refined global center.
    struct MatchResult{
        double best;
        double second;
        double center_px_x;
        double center_px_y;
    };
    auto run_match = [&](const cv::Mat& search, int off_x, int off_y) -> MatchResult{
        cv::Mat result;
        cv::matchTemplate(search, templ, result, cv::TM_CCOEFF_NORMED);

        double max_val = 0.0;
        cv::Point max_loc;
        cv::minMaxLoc(result, nullptr, &max_val, nullptr, &max_loc);

        //  Sub-tile refinement: fit a parabola through the peak and its two
        //  neighbors on each axis to recover a fractional offset in [-1, 1].
        double dx = 0.0, dy = 0.0;
        if (max_loc.x > 0 && max_loc.x < result.cols - 1){
            float l = result.at<float>(max_loc.y, max_loc.x - 1);
            float c = result.at<float>(max_loc.y, max_loc.x);
            float r = result.at<float>(max_loc.y, max_loc.x + 1);
            double denom = double(l) - 2.0 * c + r;
            if (std::abs(denom) > 1e-6){ dx = 0.5 * (double(l) - r) / denom; }
        }
        if (max_loc.y > 0 && max_loc.y < result.rows - 1){
            float u = result.at<float>(max_loc.y - 1, max_loc.x);
            float c = result.at<float>(max_loc.y, max_loc.x);
            float d = result.at<float>(max_loc.y + 1, max_loc.x);
            double denom = double(u) - 2.0 * c + d;
            if (std::abs(denom) > 1e-6){ dy = 0.5 * (double(u) - d) / denom; }
        }
        dx = std::clamp(dx, -1.0, 1.0);
        dy = std::clamp(dy, -1.0, 1.0);

        //  Second-best distinct peak: blank a neighborhood around the best,
        //  then take the next maximum. If the window is small enough that the
        //  neighborhood covers it entirely, `second` stays -1 (no competitor).
        double second_val = -1.0;
        cv::Rect zero(
            std::max(0, max_loc.x - SUPPRESS_RADIUS_PX),
            std::max(0, max_loc.y - SUPPRESS_RADIUS_PX),
            0, 0
        );
        zero.width  = std::min(result.cols, max_loc.x + SUPPRESS_RADIUS_PX + 1) - zero.x;
        zero.height = std::min(result.rows, max_loc.y + SUPPRESS_RADIUS_PX + 1) - zero.y;
        if (zero.width < result.cols || zero.height < result.rows){
            cv::Mat suppressed = result.clone();
            suppressed(zero).setTo(cv::Scalar(-1.0));
            cv::minMaxLoc(suppressed, nullptr, &second_val, nullptr, nullptr);
        }

        MatchResult m;
        m.best = max_val;
        m.second = second_val;
        m.center_px_x = (max_loc.x + dx) + off_x + templ.cols / 2.0;
        m.center_px_y = (max_loc.y + dy) + off_y + templ.rows / 2.0;
        return m;
    };

    //  Diagnostic callers pass a negative min_confidence to force the raw best
    //  guess; only gate on ambiguity for real (positive-threshold) lookups.
    const bool apply_ambiguity = min_confidence > 0.0;

    MatchResult m = run_match(search_image, search_offset_x, search_offset_y);
    bool from_full_map = !used_hint;

    //  Auto-expand: if a hinted (windowed) search found no confident match, the
    //  player may have moved outside the window. Retry on the full map and keep
    //  whichever is stronger, so a single missed hint doesn't drop a poll.
    if (used_hint && m.best < min_confidence){
        MatchResult full = run_match(m_combined, 0, 0);
        if (full.best > m.best){ m = full; from_full_map = true; }
    }

    if (m.best < min_confidence){
        return std::nullopt;
    }
    //  Ambiguity gate only on global (unhinted) fixes: a hinted window already
    //  pins down location, so a nearby look-alike inside it is safe to accept.
    //  A cold-start full-map fix must clearly beat any other map location.
    if (apply_ambiguity && from_full_map && (m.best - m.second) < AMBIGUITY_MARGIN){
        return std::nullopt;
    }

    KantoPosition pos;
    pos.tile_x = int(m.center_px_x / TILE_PX);
    pos.tile_y = int(m.center_px_y / TILE_PX);
    pos.confidence = m.best;
    return pos;
}


}
}
}
