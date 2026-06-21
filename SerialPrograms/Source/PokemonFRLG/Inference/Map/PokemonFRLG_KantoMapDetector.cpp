/*  Kanto Map Detector
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include <atomic>
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
        }
    }
    if (search_image.empty()){
        search_image = m_combined;
    }

    cv::Mat result;
    cv::matchTemplate(search_image, templ, result, cv::TM_CCOEFF_NORMED);

    double max_val = 0.0;
    cv::Point max_loc;
    cv::minMaxLoc(result, nullptr, &max_val, nullptr, &max_loc);

    if (max_val < min_confidence){
        return std::nullopt;
    }

    int center_px_x = max_loc.x + search_offset_x + templ.cols / 2;
    int center_px_y = max_loc.y + search_offset_y + templ.rows / 2;

    KantoPosition pos;
    pos.tile_x = center_px_x / TILE_PX;
    pos.tile_y = center_px_y / TILE_PX;
    pos.confidence = max_val;
    return pos;
}


}
}
}
