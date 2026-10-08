/*  Pokemon FRLG AutoStory - Segment B0_04
 *
 *  From: https://github.com/PokemonAutomation/
 */

#include "Common/Cpp/Color.h"
#include "CommonFramework/Exceptions/OperationFailedException.h"
#include "CommonTools/Async/InferenceRoutines.h"
#include "NintendoSwitch/Commands/NintendoSwitch_Commands_PushButtons.h"
#include "PokemonFRLG/Inference/Dialogs/PokemonFRLG_DialogDetector.h"
#include "PokemonFRLG_AutoStory_Segment_B0_04.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

using namespace std::chrono_literals;

void run_B0_04_route1_viridian_forest(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
){
    env.log("B0_04: Route 1/2 -> Viridian Forest entrance", COLOR_BLUE);

    //  Walk from Viridian City (just healed) to the known grass goal just
    //  south of the forest entrance.
    navigate_to(env, context, stats, KantoGoals::Route2SouthGrass);

    //  Short scripted walk north into the forest's door sprite. Best-effort
    //  -- not hardware-verified. Gated on the black-screen fade so a short
    //  mistimed walk still ends at the right scene instead of silently
    //  failing.
    BlackScreenWatcher forest_fade(COLOR_RED);
    context.wait_for_all_requests();
    int ret = run_until<ProControllerContext>(
        env.console, context,
        [](ProControllerContext& context){
            for (int i = 0; i < 8; i++){
                pbf_move_left_joystick(context, {128, 0}, 1000ms, 200ms);
            }
        },
        { forest_fade }
    );
    if (ret < 0){
        OperationFailedException::fire(
            ErrorReport::SEND_ERROR_REPORT,
            "run_B0_04_route1_viridian_forest(): did not detect the fade into Viridian "
            "Forest after walking north from the Route 2 south grass. The forest "
            "entrance tile has not been hardware-verified -- check positioning.",
            env.console
        );
    }

    pbf_wait(context, 1000ms);
    context.wait_for_all_requests();
    env.log("Entered Viridian Forest.");
}

}
}
}
