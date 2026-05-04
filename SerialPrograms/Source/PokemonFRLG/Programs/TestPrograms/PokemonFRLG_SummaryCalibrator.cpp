/*  Summary Calibrator
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include <chrono>
#include "Common/Cpp/Color.h"
#include "CommonFramework/VideoPipeline/VideoOverlayScopes.h"
#include "NintendoSwitch/Commands/NintendoSwitch_Commands_PushButtons.h"
#include "Pokemon/Pokemon_Strings.h"
#include "PokemonFRLG/Inference/PokemonFRLG_PartySummaryReader.h"
#include "PokemonFRLG_SummaryCalibrator.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

using namespace std::chrono_literals;


SummaryCalibrator_Descriptor::SummaryCalibrator_Descriptor()
    : SingleSwitchProgramDescriptor(
        "PokemonFRLG:SummaryCalibrator",
        Pokemon::STRING_POKEMON + " FRLG",
        "Summary Calibrator", "",
        "Draws all PartySummaryReader OCR boxes on the live feed and waits. "
        "Navigate to whichever Summary page you want to inspect BEFORE starting "
        "the program (POKEMON INFO for dex#, KNOWN MOVES for moves). Boxes "
        "stay visible for the configured duration; screenshot to send back for "
        "calibration.",
        ProgramControllerClass::StandardController_NoRestrictions,
        FeedbackType::REQUIRED,
        AllowCommandsWhenRunning::DISABLE_COMMANDS
    )
{}

SummaryCalibrator::SummaryCalibrator()
    : HOLD_SECONDS(
        "<b>Hold Duration (seconds):</b><br>How long to keep the overlays on screen so you can screenshot.",
        LockMode::LOCK_WHILE_RUNNING,
        30, 5, 600
    )
{
    PA_ADD_OPTION(HOLD_SECONDS);
}

void SummaryCalibrator::program(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context
){
    env.log("Summary Calibrator: drawing PartySummaryReader overlays. Waiting " +
            std::to_string((uint64_t)HOLD_SECONDS) + "s.", COLOR_BLUE);

    PartySummaryReader reader;
    VideoOverlaySet overlays(env.console.overlay());
    reader.make_overlays(overlays);

    pbf_wait(context, std::chrono::seconds((uint64_t)HOLD_SECONDS));
    context.wait_for_all_requests();

    env.log("Summary Calibrator: done.", COLOR_BLUE);
}


}
}
}
