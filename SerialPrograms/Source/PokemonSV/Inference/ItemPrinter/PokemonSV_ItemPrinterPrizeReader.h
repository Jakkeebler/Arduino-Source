/*  Item Printer - Prize Reader
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#ifndef PokemonAutomation_PokemonSV_ItemPrinterPrizeReader_H
#define PokemonAutomation_PokemonSV_ItemPrinterPrizeReader_H

#include <array>
#include "CommonFramework/Logging/Logger.h"
#include "CommonFramework/Language.h"
#include "CommonFramework/ImageTools/ImageBoxes.h"
#include "CommonFramework/Inference/VisualDetector.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonSV{




class ItemPrinterPrizeReader{
    static constexpr double MAX_LOG10P = -1.40;
    static constexpr double MAX_LOG10P_SPREAD = 0.50;

public:
    ItemPrinterPrizeReader(Language language);

    void make_overlays(VideoOverlaySet& items) const;
    std::array<std::string, 10> read(Logger& logger, const ImageViewRGB32& screen) const;

private:
    Language m_language;
    ImageFloatBox m_boxes[10];
};




}
}
}
#endif
