/*  Summary Calibrator
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Draws all PartySummaryReader OCR boxes on the live video feed and waits.
 *  Use this to calibrate float-box positions: navigate to whichever Summary
 *  page you want to inspect (page 1 POKEMON INFO, page 3 KNOWN MOVES, etc.),
 *  then run this program. Boxes stay visible for HOLD_SECONDS so you can
 *  screenshot.
 *
 *  No console interaction beyond drawing overlays.
 */

#ifndef PokemonAutomation_PokemonFRLG_SummaryCalibrator_H
#define PokemonAutomation_PokemonFRLG_SummaryCalibrator_H

#include "Common/Cpp/Options/SimpleIntegerOption.h"
#include "CommonFramework/Tools/VideoStream.h"
#include "NintendoSwitch/Controllers/Procon/NintendoSwitch_ProController.h"
#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


class SummaryCalibrator_Descriptor : public SingleSwitchProgramDescriptor{
public:
    SummaryCalibrator_Descriptor();
};

class SummaryCalibrator : public SingleSwitchProgramInstance{
public:
    SummaryCalibrator();
    virtual void program(
        SingleSwitchProgramEnvironment& env,
        ProControllerContext& context
    ) override;
    virtual void start_program_border_check(
        VideoStream& stream, FeedbackType feedback_type
    ) override{}

private:
    SimpleIntegerOption<uint64_t> HOLD_SECONDS;
};


}
}
}
#endif
