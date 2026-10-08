/*  Pokemon FRLG AutoStory - Segment B0_00
 *
 *  From: https://github.com/PokemonAutomation/
 */

#include "Common/Cpp/Color.h"
#include "CommonTools/Async/InferenceRoutines.h"
#include "NintendoSwitch/Commands/NintendoSwitch_Commands_PushButtons.h"
#include "PokemonFRLG/Inference/Dialogs/PokemonFRLG_DialogDetector.h"
#include "PokemonFRLG_AutoStory_Segment_B0_00.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

using namespace std::chrono_literals;

void run_B0_00_new_game_oak_intro(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
){
    env.log("B0_00: New Game -> Oak Intro", COLOR_BLUE);

    //  Title screen: A opens the file-select menu ("NEW GAME" is the only/
    //  default-highlighted entry on a blank cartridge), A again confirms it.
    pbf_press_button(context, BUTTON_A, 200ms, 2000ms);
    pbf_press_button(context, BUTTON_A, 200ms, 2000ms);

    //  Professor Oak's opening speech + logo animation. Mash A through the
    //  dialog boxes, then gate on the fade into the gender-select scene.
    pbf_mash_button(context, BUTTON_A, 15s);
    {
        BlackScreenWatcher black_screen;
        context.wait_for_all_requests();
        int ret = wait_until(env.console, context, 15s, {black_screen});
        if (ret < 0){
            env.log("run_B0_00_new_game_oak_intro(): gender-select fade not detected within 15s. Continuing blind.", COLOR_ORANGE);
        }
    }

    //  Gender select: BOY is the default cursor position. A confirms it.
    //  (AutoStory does not currently expose a gender option; this can be
    //  extended with a left d-pad press + A to pick GIRL if that's wanted.)
    pbf_press_button(context, BUTTON_A, 200ms, 2000ms);

    //  More Oak dialog before the naming screen.
    pbf_mash_button(context, BUTTON_A, 8s);

    //  Player name entry. Pressing PLUS (GBA START, via the Pro Controller
    //  mapping) on the naming keyboard accepts the pre-filled default name
    //  without navigating the letter grid.
    pbf_press_button(context, BUTTON_PLUS, 200ms, 1000ms);
    pbf_press_button(context, BUTTON_A, 200ms, 2000ms);  //  confirm "is this name OK?"

    //  Oak dialog, rival introduction.
    pbf_mash_button(context, BUTTON_A, 10s);

    //  Rival name entry (same default-name shortcut).
    pbf_press_button(context, BUTTON_PLUS, 200ms, 1000ms);
    pbf_press_button(context, BUTTON_A, 200ms, 2000ms);

    //  Remaining Oak dialog, then the fade into the player's bedroom.
    pbf_mash_button(context, BUTTON_A, 10s);
    {
        BlackScreenWatcher bedroom_fade;
        context.wait_for_all_requests();
        int ret = wait_until(env.console, context, 15s, {bedroom_fade});
        if (ret < 0){
            env.log("run_B0_00_new_game_oak_intro(): bedroom fade not detected within 15s. Continuing blind.", COLOR_ORANGE);
        }
    }
    pbf_wait(context, 2000ms);
    context.wait_for_all_requests();
}

}
}
}
