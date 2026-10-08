/*  Pokemon FRLG AutoStory - Segment B0_01
 *
 *  From: https://github.com/PokemonAutomation/
 */

#include "Common/Cpp/Color.h"
#include "CommonFramework/Exceptions/OperationFailedException.h"
#include "CommonTools/Async/InferenceRoutines.h"
#include "NintendoSwitch/Commands/NintendoSwitch_Commands_PushButtons.h"
#include "PokemonFRLG/Inference/Dialogs/PokemonFRLG_DialogDetector.h"
#include "PokemonFRLG_AutoStory_Segment_B0_01.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

using namespace std::chrono_literals;

namespace{

//  Number of right d-pad presses from the leftmost pokeball (Bulbasaur) to
//  reach the configured starter's ball on the table.
int steps_right_to_starter(StarterChoice starter){
    switch (starter){
    case StarterChoice::Bulbasaur:  return 0;
    case StarterChoice::Squirtle:   return 1;
    case StarterChoice::Charmander: return 2;
    }
    return 0;
}

}  //  anonymous namespace

void run_B0_01_starter_selection(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
){
    env.log("B0_01: Starter Selection", COLOR_BLUE);

    //  Walk down out of the bedroom, downstairs, and out of the house. This
    //  is a short, fixed path in vanilla Pallet Town.
    pbf_move_left_joystick(context, {128, 255}, 1000ms, 500ms);   //  down to the stairs
    pbf_move_left_joystick(context, {128, 255}, 1000ms, 500ms);   //  down the stairs
    pbf_move_left_joystick(context, {128, 255}, 500ms, 500ms);    //  out the front door

    {
        BlackScreenWatcher door_fade;
        context.wait_for_all_requests();
        int ret = wait_until(env.console, context, 10s, {door_fade});
        if (ret < 0){
            env.log("run_B0_01_starter_selection(): house-exit fade not detected within 10s. Continuing blind.", COLOR_ORANGE);
        }
    }

    //  Mom/rival's sister dialog just outside the house.
    pbf_mash_button(context, BUTTON_A, 5s);

    //  Walk to Oak's lab (south-west of the player's house in Pallet Town).
    pbf_move_left_joystick(context, {128, 255}, 1500ms, 500ms);
    pbf_move_left_joystick(context, {0, 128}, 1500ms, 500ms);    //  left
    pbf_move_left_joystick(context, {128, 255}, 500ms, 500ms);   //  into the lab doorway

    {
        BlackScreenWatcher lab_fade;
        context.wait_for_all_requests();
        int ret = wait_until(env.console, context, 10s, {lab_fade});
        if (ret < 0){
            env.log("run_B0_01_starter_selection(): lab-entry fade not detected within 10s. Continuing blind.", COLOR_ORANGE);
        }
    }

    //  Oak's aide/dialog before letting you approach the table.
    pbf_mash_button(context, BUTTON_A, 6s);

    //  Walk up to the pokeball table and select the configured starter.
    pbf_move_left_joystick(context, {128, 0}, 500ms, 500ms);    //  approach table (north)
    int right_steps = steps_right_to_starter(options.starter_choice);
    for (int i = 0; i < right_steps; i++){
        pbf_move_left_joystick(context, {255, 128}, 300ms, 300ms);
    }
    pbf_press_button(context, BUTTON_A, 200ms, 1500ms);  //  interact with the ball

    //  "Do you want <species>?" Yes/No prompt -- cursor defaults to YES.
    pbf_mash_button(context, BUTTON_A, 3s);

    //  Receive the Pokemon + Oak's dialog, then the nickname prompt. The
    //  nickname Yes/No prompt defaults to NO on a single down-press.
    pbf_mash_button(context, BUTTON_A, 4s);
    pbf_press_button(context, BUTTON_DOWN, 200ms, 500ms);
    pbf_press_button(context, BUTTON_A, 200ms, 1500ms);  //  confirm "No nickname"

    //  Remaining Oak dialog back in the lab, overworld control restored.
    pbf_mash_button(context, BUTTON_A, 8s);
    context.wait_for_all_requests();
}

}
}
}
