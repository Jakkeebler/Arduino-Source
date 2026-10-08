/*  Pokemon FRLG AutoStory - Segment B7a
 *
 *  From: https://github.com/PokemonAutomation/
 */

#include "Common/Cpp/Color.h"
#include "CommonFramework/Exceptions/OperationFailedException.h"
#include "CommonTools/Async/InferenceRoutines.h"
#include "NintendoSwitch/Commands/NintendoSwitch_Commands_PushButtons.h"
#include "PokemonFRLG/PokemonFRLG_Navigation.h"
#include "PokemonFRLG/Inference/Dialogs/PokemonFRLG_DialogDetector.h"
#include "PokemonFRLG/Inference/Dialogs/PokemonFRLG_BattleDialogs.h"
#include "PokemonFRLG/Programs/PokemonFRLG_KantoMapNavigator.h"
#include "PokemonFRLG_AutoStory_Segment_B7a.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

using namespace std::chrono_literals;

namespace{

//  Fight off a single wild-encounter interruption while walking through the
//  Mansion (same shape as Segment_B5b's tower-floor wild Ghost handling).
//  Returns true if a battle was fought and won, false if no encounter
//  occurred (watcher never fired, nothing to do).
bool handle_possible_wild_encounter(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    AutoStoryStats& stats
){
    BattleMenuWatcher battle_ready(COLOR_RED);
    context.wait_for_all_requests();
    int ret = wait_until(env.console, context, 3s, {battle_ready});
    if (ret < 0){
        return false;
    }
    BattleResult result = spam_first_move(env.console, context);
    if (result == BattleResult::playerfainted){
        OperationFailedException::fire(
            ErrorReport::SEND_ERROR_REPORT,
            "run_B7a_cinnabar_mansion(): lost a wild encounter inside Pokemon Mansion.",
            env.console
        );
    }
    track_battle_won(stats);
    exit_wild_battle(env.console, context, false, true);
    return true;
}

}  //  anonymous namespace


void run_B7a_cinnabar_mansion(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
){
    (void)options;
    env.log("B7a: Cinnabar Gym -> Pokemon Mansion (Secret Key) -> Cinnabar Gym", COLOR_BLUE);

    //  ---- Walk from the Gym door to the Mansion entrance ----
    //  Both buildings are on the same small island screen, so no long-range
    //  overworld hop is required; a direct navigate_to() between the two
    //  known goals covers it.
    env.log("Heading to Pokemon Mansion...");
    navigate_to(env, context, stats, KantoGoals::PokemonMansionEntrance, &KantoGoals::CinnabarGymEntrance);

    //  ---- Enter the Mansion ----
    //  enter_building()/BuildingType only has POKECENTER/GYM cases wired up
    //  today (see PokemonFRLG_AutoStoryTools.h), but enter_pokecenter() is
    //  itself just a generic "walk through the door, wait for the fade"
    //  helper with no PokeCenter-specific logic, so reusing it here for the
    //  Mansion's door is intentional, matching the same reuse already done
    //  for the Game Corner door in Segment_B5a.
    enter_building(env, context, BuildingType::POKECENTER);

    //  ---- Traverse the Mansion floors to the Secret Key ----
    //  NOTE: best-effort / not hardware-verified. The Mansion is a 4-floor
    //  dungeon in the original game (plus a basement) with the Secret Key
    //  on a middle floor; the exact tile path has not been captured here.
    //  This just walks generally forward each floor, fighting off any wild
    //  encounter met along the way, and mashes through the "found Secret
    //  Key" dialog once in the right spot. Flagging as the segment's
    //  highest-risk section, same caveat as the Rocket Hideout trapdoors in
    //  Segment_B5a.
    for (int floor = 0; floor < 3; floor++){
        env.log("Pokemon Mansion: walking floor " + std::to_string(floor + 1) + "...");
        pbf_move_left_joystick(context, {128, 0}, 1500ms, 300ms);
        pbf_move_left_joystick(context, {128, 0}, 1000ms, 300ms);
        context.wait_for_all_requests();
        handle_possible_wild_encounter(env, context, stats);
    }

    //  Pick up the Secret Key (simple "found item" dialog, no choices).
    env.log("Picking up the Secret Key...");
    pbf_mash_button(context, BUTTON_A, 8s);
    context.wait_for_all_requests();

    //  ---- Leave the Mansion the way we came ----
    //  Best-effort retrace: mash B (cancel any menu) then walk back toward
    //  the entrance door. Like the walk-in above, this is not
    //  hardware-verified.
    pbf_mash_button(context, BUTTON_B, 3s);
    for (int floor = 0; floor < 3; floor++){
        pbf_move_left_joystick(context, {128, 255}, 2000ms, 300ms);
    }
    {
        BlackScreenWatcher exit_fade;
        context.wait_for_all_requests();
        int ret = wait_until(env.console, context, 10s, {exit_fade});
        if (ret < 0){
            env.log("run_B7a_cinnabar_mansion(): mansion exit fade not detected. Continuing blind.", COLOR_ORANGE);
        }
    }
    exit_building(env, context, BuildingType::POKECENTER);

    //  ---- Walk back to the Cinnabar Gym door ----
    env.log("Heading back to the Cinnabar Gym...");
    navigate_to(env, context, stats, KantoGoals::CinnabarGymEntrance, &KantoGoals::PokemonMansionEntrance);

    context.wait_for_all_requests();
    env.log("Secret Key obtained. Standing outside the Cinnabar Gym.");
}

}
}
}
