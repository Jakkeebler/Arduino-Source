/*  Pokemon FRLG Navigation
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Soft reset, menus, etc.
 *
 */

#include <array>
#include "CommonFramework/Exceptions/OperationFailedException.h"
#include "CommonFramework/VideoPipeline/VideoFeed.h"
#include "CommonTools/Random.h"
#include "CommonTools/Async/InferenceRoutines.h"
#include "CommonTools/StartupChecks/StartProgramChecks.h"
#include "CommonTools/StartupChecks/VideoResolutionCheck.h"
#include "NintendoSwitch/Programs/NintendoSwitch_GameEntry.h"
#include "NintendoSwitch/Commands/NintendoSwitch_Commands_PushButtons.h"
#include "NintendoSwitch/Commands/NintendoSwitch_Commands_Superscalar.h"
#include "NintendoSwitch/Controllers/Procon/NintendoSwitch_ProController.h"
#include "NintendoSwitch/NintendoSwitch_ConsoleHandle.h"
#include "NintendoSwitch/Inference/NintendoSwitch_HomeMenuDetector.h"
#include "Pokemon/Pokemon_Strings.h"
#include "PokemonFRLG/PokemonFRLG_Settings.h"
#include "PokemonFRLG/Inference/Dialogs/PokemonFRLG_DialogDetector.h"
#include "PokemonFRLG/Inference/Dialogs/PokemonFRLG_BattleDialogs.h"
#include "PokemonFRLG/Inference/Dialogs/PokemonFRLG_PartyDialogs.h"
#include "PokemonFRLG/Inference/Sounds/PokemonFRLG_ShinySoundDetector.h"
#include "PokemonFRLG/Inference/Menus/PokemonFRLG_BagDetector.h"
#include "PokemonFRLG/Inference/Menus/PokemonFRLG_StartMenuDetector.h"
#include "PokemonFRLG/Inference/Menus/PokemonFRLG_LoadMenuDetector.h"
#include "PokemonFRLG/Inference/Menus/PokemonFRLG_SummaryDetector.h"
#include "PokemonFRLG/Inference/Menus/PokemonFRLG_PartyEmptySlotDetector.h"
#include "PokemonFRLG/Inference/Menus/PokemonFRLG_PartyMenuDetector.h"
#include "PokemonFRLG/Inference/Map/PokemonFRLG_MapDetector.h"
#include "PokemonFRLG/Inference/PokemonFRLG_BattlePokemonDetector.h"
#include "PokemonFRLG/Programs/PokemonFRLG_StartMenuNavigation.h"
#include "PokemonFRLG/Programs/PokemonFRLG_BattleMenuNavigation.h"
#include "PokemonFRLG/Programs/Farming/PokemonFRLG_MoveLearnDecider.h"
#include "PokemonFRLG/Inference/Dialogs/PokemonFRLG_LearnMoveDialogReader.h"
#include "PokemonFRLG/Inference/Dialogs/PokemonFRLG_ForgetMoveScreen.h"
#include "PokemonFRLG_Navigation.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


void home_black_border_check(ConsoleHandle& console, ProControllerContext& context){
    if (GameSettings::instance().DEVICE == GameSettings::Device::switch_1_2){
        console.log("Switch 1 or 2 selected in Settings.");

        console.log("Checking for min 720p and 16:9.");
        assert_16_9_720p_min(console, console);

        console.log("Going to home to check for black border.");

        //  Connect the controller.
        require_player(console, context, BUTTON_ZL);

        pbf_press_button(context, BUTTON_HOME, 120ms, 880ms);
        try{
            ensure_at_home(console, context, 2);
        }catch (OperationFailedException&){
            ControllerPlayerNumber current = context->get_player_number(context);
            if (current == ControllerPlayerNumber::UNKNOWN){
                throw UserSetupError(
                    console,
                    "Unable to find Home menu.\n\n"
                    "Either your controller isn't connected or your screen size to not "
                    "set to 100% in the TV Settings on your Nintendo Switch.\n\n"
                    "If your Switch entered the Home screen and re-entered the game, then your "
                    "controller is connected but your screen size is not set to 100%.\n\n"
                    "If nothing happened at all, then your controller is not connected. "
                    "Please disconnect all other controllers and try again.\n\n"
                    "We recommend changing the controller to \"NS1: Wired Pro Controller\" "
                    "as that will be able self-diagnose controller connection issues."
                );
            }else{
                throw UserSetupError(
                    console,
                    "Unable to find Home menu.\n\n"
                    "It is likely your screen size to not set to 100% in the TV Settings on your Nintendo Switch."
                );
            }
        }

//        context.wait_for_all_requests();
        StartProgramChecks::check_border(console);
        console.log("Returning to game.");
        resume_game_from_home(console, context);
        context.wait_for_all_requests();
        console.log("Entered game.");
    }else{
        console.log("Non-Switch device selected in Settings.");
        console.log("Skipping black border check.", COLOR_BLUE);
    }
}




bool try_soft_reset(ConsoleHandle& console, ProControllerContext& context){
    // A + B + Select + Start
    pbf_press_button(context, BUTTON_B | BUTTON_A | BUTTON_MINUS | BUTTON_PLUS, 360ms, 1440ms);

    pbf_mash_button(context, BUTTON_MINUS, GameSettings::instance().SELECT_BUTTON_MASH0);
    context.wait_for_all_requests();

    //Random wait before pressing start/A
    console.log("Randomly waiting...");
    Milliseconds rng_wait = std::chrono::milliseconds(random_u32(0, 5000));
    pbf_wait(context, rng_wait);
    context.wait_for_all_requests();

    //Mash A until white screen to game load menu
    WhiteScreenOverWatcher whitescreen(COLOR_RED);
    LoadMenuWatcher load_menu(COLOR_BLUE);

    int ls = run_until<ProControllerContext>(
        console, context,
        [](ProControllerContext& context){
            pbf_mash_button(context, BUTTON_A, 1000ms);
            pbf_wait(context, 5000ms);
            context.wait_for_all_requests();
        },
        { whitescreen, load_menu }
    );
    context.wait_for_all_requests();
    if (ls == 0){
        console.log("Entered load menu. (WhiteScreenOver)");
    }else if (ls == 1){
        console.log("Entered load menu. (LoadMenu)");
    }else{
        console.log("soft_reset(): Unable to enter load menu.", COLOR_RED);
        return false;
    }
    //Let the animation finish
    pbf_wait(context, 500ms);
    context.wait_for_all_requests();

    //Load game
    pbf_press_button(context, BUTTON_A, 160ms, 320ms);

    //Wait for game to load in
    BlackScreenOverWatcher detector(COLOR_RED);
    int ret = wait_until(
        console, context,
        GameSettings::instance().ENTER_GAME_WAIT0,
        {detector}
    );
    if (ret == 0){
        console.log("Entered game!");
    }else{
        console.log("soft_reset(): Timed out waiting to enter game.", COLOR_RED);
        return false;
    }

    //Mash past "previously on..."
    pbf_mash_button(context, BUTTON_B, GameSettings::instance().ENTER_GAME_MASH0);
    context.wait_for_all_requests();

    //Random wait no.2
    console.log("Randomly waiting...");
    Milliseconds rng_wait2 = std::chrono::milliseconds(random_u32(0, 5000));
    pbf_wait(context, rng_wait2);
    context.wait_for_all_requests();

    return true;
}

uint64_t soft_reset(ConsoleHandle& console, ProControllerContext& context){
    uint64_t errors = 0;
    for (; errors < 5; errors++){
        if (try_soft_reset(console, context)){
            console.log("Soft reset completed.");
            return errors;
        }
    }
    OperationFailedException::fire(
        ErrorReport::SEND_ERROR_REPORT,
        "soft_reset(): Failed to reset after 5 attempts.",
        console
    );
}

bool try_open_slot_six(ConsoleHandle& console, ProControllerContext& context){
    //  Attempt to exit any dialog and open the start menu
    StartMenuWatcher start_menu(COLOR_RED);

    int ret = run_until<ProControllerContext>(
        console, context,
        [](ProControllerContext& context){
            for (int i = 0; i < 10; i++){
                pbf_press_button(context, BUTTON_B, 320ms, 640ms);
                pbf_wait(context, 100ms);
                context.wait_for_all_requests();
                pbf_press_button(context, BUTTON_PLUS, 320ms, 640ms);
                pbf_wait(context, 100ms);
                context.wait_for_all_requests();
            }
        },
        { start_menu }
    );
    context.wait_for_all_requests();
    if (ret < 0){
        console.log("open_slot_six(): Unable to open Start menu.", COLOR_RED);
        return false;
    }

    if (!move_cursor_to_position(console, context, SelectionArrowPositionStartMenu::POKEMON)){
        console.log("open_slot_six(): Unable to move menu cursor to: " + Pokemon::STRING_POKEMON, COLOR_RED);
        return false;
    }

    console.log("Navigating to party menu.");
    PartyMenuWatcher blk1(COLOR_RED);

    int pm = run_until<ProControllerContext>(
        console, context,
        [](ProControllerContext& context){
            pbf_press_button(context, BUTTON_A, 320ms, 5640ms);
            context.wait_for_all_requests();
        },
        { blk1 }
    );
    if (pm == 0){
        console.log("Entered party menu.");
    }else{
        console.log("open_slot_six(): Unable to enter Party menu.", COLOR_RED);
        return false;
    }
    context.wait_for_all_requests();

    //Press up twice to get to the last slot
    PartySlotWatcher last_slot(COLOR_RED, PartySlot::SIX);
    int ps = run_until<ProControllerContext>(
        console, context,
        [](ProControllerContext& context){
            for (int i = 0; i < 15; i++){ //Enough to cycle through 6pty+cxl twice
                pbf_wait(context, 320ms);
                context.wait_for_all_requests();
                pbf_press_dpad(context, DPAD_UP, 320ms, 320ms);
            }
        },
        { last_slot }
        );
    context.wait_for_all_requests();
    if (ps == 0){
        console.log("Moved selection to slot six.");
    }else{
        console.log("open_slot_six(): Unable to move selection to slot six.", COLOR_RED);
        return false;
    }

    //Two presses to open summary
    BlackScreenOverWatcher blk2(COLOR_RED);
    int sm = run_until<ProControllerContext>(
        console, context,
        [](ProControllerContext& context){
            pbf_press_button(context, BUTTON_A, 320ms, 640ms);
            pbf_press_button(context, BUTTON_A, 320ms, 640ms);
            pbf_wait(context, 5000ms);
            context.wait_for_all_requests();
        },
        { blk2 }
    );
    if (sm == 0){
        console.log("Entered summary.");
    }else{
        console.log("open_slot_six(): Unable to enter summary.", COLOR_RED);
        return false;
    }

    //Double check that we are on summary
    SummaryWatcher sum1(COLOR_RED);
    int sm1 = wait_until(
        console, context,
        std::chrono::seconds(5),
        {{ sum1 }}
    );
    if (sm1 == 0){
        console.log("Summary page dots detected.");
    }else{
        console.log("open_slot_six(): Unable to detect summary screen.", COLOR_RED);
        return false;
    }

    pbf_wait(context, 1000ms);
    context.wait_for_all_requests();
    return true;
}

uint64_t open_slot_six(ConsoleHandle& console, ProControllerContext& context){
    uint64_t errors = 0;
    for (; errors < 5; errors++){
        if (try_open_slot_six(console, context)){
            return errors;
        }else{
            console.log("Mashing B to return to overworld and retry...");
            pbf_mash_button(context, BUTTON_B, 10000ms);
        }
    }
    OperationFailedException::fire(
        ErrorReport::SEND_ERROR_REPORT,
        "open_slot_six(): Failed to open party summary after 5 attempts.",
        console
    );
}

bool handle_encounter(ConsoleHandle& console, ProControllerContext& context, bool send_out_lead){
    float shiny_coefficient = 1.0;
    ShinySoundDetector shiny_detector(console.logger(), [&](float error_coefficient) -> bool{
        shiny_coefficient = error_coefficient;
        return true;
    });
    AdvanceBattleDialogWatcher battle_dialog(COLOR_YELLOW);
    
    int res = run_until<ProControllerContext>(
        console, context,
        [&](ProControllerContext& context){
            int ret = wait_until(
                console, context,
                std::chrono::seconds(30), //More than enough time for shiny sound
                {{battle_dialog}}
            );
            if (ret == 0){
                console.log("Battle Advance arrow detected.");
            }else{
                OperationFailedException::fire(
                    ErrorReport::SEND_ERROR_REPORT,
                    "handle_encounter(): Did not detect battle advance arrow.",
                    console
                );
            }
            pbf_wait(context, 1000ms);
            context.wait_for_all_requests();

            /* 
            //Send out shiny lead to test detection
            BattleMenuWatcher battle_menu(COLOR_RED);
            console.log("Sending out lead Pokemon.");
            pbf_press_button(context, BUTTON_A, 320ms, 320ms);

            int ret2 = wait_until(
                console, context,
                std::chrono::seconds(15),
                { {battle_menu} }
            );
            if (ret2 == 0){
                console.log("Battle menu detecteed!");
            }else{
                OperationFailedException::fire(
                    ErrorReport::SEND_ERROR_REPORT,
                    "handle_encounter(): Did not detect battle menu.",
                    console
                );
            }
            pbf_wait(context, 100000ms); //extreme audio delay on my cheap test device
            context.wait_for_all_requests();
            */

        },
        {{shiny_detector}}
    );
    shiny_detector.throw_if_no_sound(std::chrono::milliseconds(1000));
    if (res == 0){
        console.log("Shiny detected!");
        return true;
    }
    console.log("No shiny detected.");

    if (send_out_lead){
        //Send out lead, no shiny detection needed. (Or wanted.)
        BattleMenuWatcher battle_menu(COLOR_RED);
        console.log("Sending out lead Pokemon.");
        WallClock start = current_time();
        
        while (true){
            if (current_time() - start > 60s){
                OperationFailedException::fire(
                    ErrorReport::SEND_ERROR_REPORT,
                    "handle_encounter(): No battle menu detected after sixty seconds.",
                    console
                );
            }
            pbf_press_button(context, BUTTON_B, 320ms, 320ms);

            int ret = wait_until(
                console, context,
                std::chrono::seconds(15),
                { {battle_menu, battle_dialog} }
            );

            switch (ret){
            case 0:
                console.log("Battle menu detecteed!");
                break;
            case 1:
                console.log("Battle Advance arrow detected. This is likely due to an ability triggering at the start of battle.");
                pbf_press_button(context, BUTTON_B, 320ms, 320ms);
                context.wait_for_all_requests();
                continue;
            default:
                console.log("Did not detect battle menu or battle dialog.");
                continue;
            }

            pbf_wait(context, 1000ms);
            context.wait_for_all_requests();
            break;
        }
    }

    return false;
}

BattleResult spam_first_move(
    ConsoleHandle& console, ProControllerContext& context,
    const std::vector<size_t>& move_priority
){
    //  Effective priority list: drop out-of-range entries; if empty, default to slot 0.
    std::vector<size_t> priority;
    priority.reserve(move_priority.size());
    for (size_t s : move_priority){
        if (s < 4){
            priority.push_back(s);
        }
    }
    if (priority.empty()){
        priority.push_back(0);
    }
    const bool single_default_slot = (priority.size() == 1 && priority[0] == 0);

    uint16_t errors = 0;
    uint16_t times_moved = 0;
    while (true){
        if (errors > 5) {
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "spam_first_move(): Failed to use move 5 times.",
                console
            );
        } else if (times_moved > 50){
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "spam_first_move(): More than 50 move uses detected.",
                console
            );
        }

        BattleMenuWatcher battle_menu(COLOR_RED);
        BattleFaintWatcher pokemon_fainted(COLOR_RED);
        BattleOpponentFaintWatcher opponent_fainted(COLOR_RED);
        BlackScreenWatcher battle_ended(COLOR_RED);

        int ret = run_until<ProControllerContext>(
            console, context,
            [](ProControllerContext& context){
                pbf_wait(context, 20000ms);
                context.wait_for_all_requests();
            },
            { battle_menu, pokemon_fainted, opponent_fainted, battle_ended }
        );

        switch (ret){
        case 0: {
            context.wait_for_all_requests();
            //  Press A on FIGHT to open the move list, then probe each priority
            //  slot in order. The BattleOutOfPpWatcher reads the currently
            //  highlighted slot's PP region, so we re-check it after each
            //  cursor move.
            int initial_pp_ret;
            {
                BattleOutOfPpWatcher pp_watch(COLOR_RED);
                initial_pp_ret = run_until<ProControllerContext>(
                    console, context,
                    [](ProControllerContext& ctx){
                        pbf_press_button(ctx, BUTTON_A, 200ms, 400ms);
                    },
                    { pp_watch }
                );
            }

            //  Fast path: legacy default of "use slot 1 only", no cursor navigation,
            //  no PP re-checks. Matches pre-priority-list behaviour exactly.
            if (single_default_slot){
                if (initial_pp_ret >= 0){
                    console.log("Out of PP, fleeing battle.");
                    pbf_mash_button(context, BUTTON_B, 2000ms);
                    flee_battle(console, context);
                    context.wait_for_all_requests();
                    return BattleResult::outofpp;
                }
                console.log("Using move slot 1.");
                pbf_press_button(context, BUTTON_A, 200ms, 300ms);
                pbf_mash_button(context, BUTTON_A, 500ms);
                context.wait_for_all_requests();
                times_moved++;
                continue;
            }

            //  Multi-slot path: walk the priority list, using arrow detection
            //  to navigate the move list and re-checking PP per slot.
            bool selected = false;
            for (size_t i = 0; i < priority.size(); i++){
                size_t slot = priority[i];
                console.log("Trying move slot " + std::to_string(slot + 1) + ".");
                if (!move_cursor_to_move_slot(console, context, static_cast<MoveSlot>(slot))){
                    console.log("Failed to position cursor on move slot " + std::to_string(slot + 1) + "; trying next.", COLOR_RED);
                    continue;
                }
                //  Let the PP/type info panel redraw before sampling the watcher.
                pbf_wait(context, 300ms);
                context.wait_for_all_requests();

                BattleOutOfPpWatcher pp_watch_slot(COLOR_RED);
                int pp_ret = wait_until(
                    console, context,
                    std::chrono::milliseconds(600),
                    { pp_watch_slot }
                );
                if (pp_ret >= 0){
                    console.log("Move slot " + std::to_string(slot + 1) + " is out of PP.");
                    continue;
                }

                console.log("Using move slot " + std::to_string(slot + 1) + ".");
                pbf_press_button(context, BUTTON_A, 200ms, 300ms);
                pbf_mash_button(context, BUTTON_A, 500ms);
                context.wait_for_all_requests();
                times_moved++;
                selected = true;
                break;
            }
            if (selected){
                continue;
            }
            console.log("All priority moves out of PP, fleeing battle.");
            pbf_mash_button(context, BUTTON_B, 2000ms);
            flee_battle(console, context);
            context.wait_for_all_requests();
            return BattleResult::outofpp;
        }
        case 1:
            console.log("Player Pokemon fainted.");
            return BattleResult::playerfainted;
        case 2:
            console.log("Opponent fainted.");
            return BattleResult::opponentfainted;
        case 3:
            console.log("Battle ended"); // the opponent probably fled
            pbf_wait(context, 2000ms);
            context.wait_for_all_requests();
            return BattleResult::unknown;
        default:
            console.log("Failed to detect move use.");
            pbf_mash_button(context, BUTTON_B, 2000ms); // get back to the top-level battle menu
            context.wait_for_all_requests();
            continue;
        }
    }
}

void flee_battle(ConsoleHandle& console, ProControllerContext& context){
    uint16_t errors = 0;

    BattleMenuWatcher battle_menu(COLOR_RED);
    AdvanceBattleDialogWatcher ran_away(COLOR_YELLOW);
    BlackScreenOverWatcher battle_over(COLOR_RED);

    while (true)
    {
        if (errors > 5){
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "flee_battle(): Failed to flee battle after 5 attempts.",
                console
            );
        }

        context.wait_for_all_requests();
        int ret = run_until<ProControllerContext>(
            console, context,
            [](ProControllerContext& context) {
                pbf_wait(context, 1000ms);
                pbf_mash_button(context, BUTTON_B, 9000ms);
            },
            { battle_menu }
        );
        if (ret < 0) {
            errors++;
            console.log("flee_battle(): Failed to detect battle menu. Attempt " + std::to_string(errors) + "/5", COLOR_RED);
            continue;
        }

        console.log("Navigate to Run.");
        pbf_press_dpad(context, DPAD_RIGHT, 160ms, 160ms);
        pbf_press_dpad(context, DPAD_DOWN, 160ms, 160ms);
        pbf_press_button(context, BUTTON_A, 160ms, 320ms);

        int ret2 = wait_until(
            console, context,
            std::chrono::seconds(5),
            { {ran_away} }
        );

        if (ret2 == 0) {
            console.log("Running away...");
        }
        else {
            // Even though we failed to detect the "ran away" dialog, we might have still successfully fled. 
            // Attempt the next detections as a possible recovery.
            console.log("flee_battle(): Failed to detect flee dialog.", COLOR_RED);
        }

        int ret3 = run_until<ProControllerContext>(
            console, context,
            [](ProControllerContext& context) {
                pbf_press_button(context, BUTTON_A, 320ms, 640ms);
                pbf_wait(context, 5000ms);
                context.wait_for_all_requests();
            },
            { battle_over, battle_menu }
        );
        if (ret3 == 0) {
            console.log("Successfully fled the battle.");
            return;
        }
        else if (ret3 == 1){
            errors++;
            console.log("flee_battle(): Detected battle menu after attempting to flee. Attempt " + std::to_string(errors) + "/5", COLOR_RED);
            continue;
        }
        else {
            console.log("flee_battle(): failed to detect transition to overworld. Attempting to open start menu to verify successful flee..", COLOR_RED);
            
            open_start_menu(console, context);
            close_start_menu(console, context);
            context.wait_for_all_requests();
            return;
        }
    }
}

WildBattleExit exit_wild_battle(
    ConsoleHandle& console, ProControllerContext& context,
    bool stop_on_move_learn, bool prevent_evolution,
    const MoveLearnDecider* decider,
    Language language,
    bool* evolved_out
){
    // For move learning, there are two dialog selection boxes in a row.
    // Decline path: press B on the first, then A on the second (don't learn).
    // Replace path: press A on the first, wait for the "Forget which move?"
    //   screen, OCR the 4 current moves, ask the decider which slot to forget,
    //   navigate (DPAD_DOWN x N), press A to confirm.
    // Stop: return StopBattleStuck immediately — caller must halt and not
    //   navigate, since the dialog is still on screen.

    auto exit_normally = [&](bool move_learned){
        return move_learned ? WildBattleExit::LearnHandled : WildBattleExit::NoLearn;
    };

    uint16_t errors = 0;
    uint16_t loops = 0;
    bool first_attempt = true;
    bool rejected_first_box = false;
    bool move_learned = false;
    while (true){
        if (errors > 5 || loops > 5){
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "exit_wild_battle(): Failed to exit battle.",
                console
            );
        }

        BlackScreenWatcher battle_exited(COLOR_RED);    
        AdvanceBattleDialogWatcher advance_dialog(COLOR_RED);
        BattleLearnDialogWatcher move_learn_select(COLOR_RED);

        context.wait_for_all_requests();
        WallClock deadline = current_time() + 30s;
        int ret;
        if (first_attempt){
            ret = run_until<ProControllerContext>(
                console, context,
                [](ProControllerContext& context) {
                    pbf_mash_button(context, BUTTON_B, 20000ms);
                },
                { battle_exited }
            );
        }else{
            ret = run_until<ProControllerContext>(
                console, context,
                [deadline, rejected_first_box](ProControllerContext& context) {
                    pbf_wait(context, 1000ms); // give the watchers a chance to detect something
                    while (current_time() < deadline){
                        pbf_press_button(context, rejected_first_box ? BUTTON_A : BUTTON_B, 200ms, 1800ms);
                    }
                },
                { battle_exited, advance_dialog, move_learn_select }
            );
        }

        BattleDialogWatcher evolution_started(COLOR_RED);
        StartMenuWatcher start_menu_open(COLOR_RED);
        int ret2;

        switch (ret){
        case 0:
            // check for the evolution screen
            context.wait_for_all_requests(); 
            ret2 = run_until<ProControllerContext>(
                console, context,
                [](ProControllerContext& context) {
                    pbf_wait(context, 2000ms);
                },
                { evolution_started }
            );

            if (ret2 == 0){
                console.log("Evolution detected.");
                if (!prevent_evolution){
                    // make sure B isn't pressed too soon, which would cancel the evolution
                    pbf_wait(context, 20000ms);
                    //  Only report an evolution that we actually allowed to
                    //  complete. When prevent_evolution is set we cancel it,
                    //  so the species is unchanged and no rescan is needed.
                    if (evolved_out != nullptr){
                        *evolved_out = true;
                    }
                }
                rejected_first_box = false;
                continue; // press B as in other cases, and handle any move learning loops that might come up
            }
            console.log("Battle exited.");
            return exit_normally(move_learned);
        case 1:
            console.log("Battle Advance arrow detected.");
            pbf_press_button(context, BUTTON_B, 200ms, 800ms);
            rejected_first_box = false;
            continue;
        case 2:
            if (stop_on_move_learn){
                console.log("Move learn detected. Stopping per stop_on_move_learn (battle dialog still active).");
                return WildBattleExit::StopBattleStuck;
            }
            //  Second iteration of the learn dialog (after a previous Decline):
            //  this is the "Give up on learning Y?" prompt — press A to confirm.
            if (rejected_first_box){
                loops++;
                console.log("Declined to learn new move (second prompt).");
                pbf_press_button(context, BUTTON_A, 200ms, 0ms);
                continue;
            }
            //  First iteration of the learn dialog. Decide accept vs decline.
            {
                MoveLearnDecider::FirstAction action = MoveLearnDecider::FirstAction::Decline;
                std::string new_move_slug;
                if (decider != nullptr){
                    LearnMoveDialogReader dialog_reader(COLOR_RED);
                    VideoSnapshot snap = console.video().snapshot();
                    new_move_slug = dialog_reader.read_new_move(console.logger(), language, snap);
                    action = decider->decide_accept_or_decline(new_move_slug);
                    console.log(
                        "Move learn dialog: new move OCR='" + new_move_slug +
                        "', decision=" + (
                            action == MoveLearnDecider::FirstAction::Stop ? "Stop" :
                            action == MoveLearnDecider::FirstAction::Replace ? "Replace" : "Decline"
                        )
                    );
                }
                if (action == MoveLearnDecider::FirstAction::Stop){
                    console.log("Decider returned Stop (battle dialog still active).");
                    return WildBattleExit::StopBattleStuck;
                }
                if (action == MoveLearnDecider::FirstAction::Replace){
                    //  Accept the prompt and walk the forget-move screen.
                    pbf_press_button(context, BUTTON_A, 200ms, 0ms);
                    context.wait_for_all_requests();

                    ForgetMoveScreenWatcher forget_screen(COLOR_RED);
                    int waited = wait_until(
                        console, context,
                        std::chrono::milliseconds(5000),
                        { forget_screen }
                    );
                    if (waited < 0){
                        console.log("Forget-move screen not detected after 5s; falling back to fixed wait.", COLOR_RED);
                        pbf_wait(context, 1500ms);
                        context.wait_for_all_requests();
                    }

                    ForgetMoveScreenReader forget_reader(COLOR_RED);
                    VideoSnapshot forget_snap = console.video().snapshot();
                    auto current_moves = forget_reader.read_moves(console.logger(), language, forget_snap);
                    console.log(
                        std::string("Forget-screen current moves: [") +
                        current_moves[0] + "|" + current_moves[1] + "|" +
                        current_moves[2] + "|" + current_moves[3] + "]"
                    );
                    int forget_slot = decider->pick_forget_slot(new_move_slug, current_moves);
                    console.log("Forgetting slot " + std::to_string(forget_slot + 1) + ".");

                    //  Cursor starts on the top move (slot 0). Step down to target.
                    for (int i = 0; i < forget_slot; i++){
                        pbf_press_dpad(context, DPAD_DOWN, 160ms, 320ms);
                    }
                    pbf_press_button(context, BUTTON_A, 200ms, 0ms);
                    context.wait_for_all_requests();
                    move_learned = true;
                    //  Post-replace dialogs ("1, 2, and... poof!", "X learned Y!")
                    //  advance with B in subsequent iterations.
                    continue;
                }
                //  Decline path: press B on the first prompt.
                pbf_press_button(context, BUTTON_B, 200ms, 0ms);
                rejected_first_box = true;
                move_learned = true;
            }
            continue;
        default:
            if (first_attempt){
                console.log("Loop detected.");
                first_attempt = false;
                continue;
            }
            console.log("Failed to detect expected battle dialogs.");
            errors++;
            // attempt to exit any screen that might be open (party, bag, etc)
            pbf_mash_button(context, BUTTON_B, 500ms);
            // overworld detection: look for the start menu
            context.wait_for_all_requests();
            ret2 = run_until<ProControllerContext>(
                console, context,
                [](ProControllerContext& context) {
                    pbf_press_button(context, BUTTON_PLUS, 200ms, 800ms);
                    pbf_press_button(context, BUTTON_PLUS, 200ms, 800ms);
                    pbf_press_button(context, BUTTON_PLUS, 200ms, 800ms);
                },
                { start_menu_open }
            );
            if (ret2 == 0){
                pbf_mash_button(context, BUTTON_B, 500ms);
                context.wait_for_all_requests();
                console.log("Battle exited.");
                return exit_normally(move_learned);
            }
            context.wait_for_all_requests();
            rejected_first_box = false;
            continue;
        }
    }
}

void open_party_menu_from_overworld(ConsoleHandle& console, ProControllerContext& context, StartMenuContext menu_context){
    uint16_t errors = 0;
    bool start_menu_is_open = false;
    while (true){
        if (errors > 5){
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "open_party_menu_from_overworld(): Failed to open party menu 5 times in a row.",
                console
            );
        }

        context.wait_for_all_requests();
        if (!start_menu_is_open){
            open_start_menu(console, context); // This is unavoidable since we cannot detect the overworld.
            start_menu_is_open = true;
        }

        StartMenuWatcher start_menu(COLOR_RED);
        PartyMenuWatcher party_menu(COLOR_RED);

        int ret = wait_until(
            console, context, 10000ms,
            { start_menu, party_menu }
        );

        switch (ret){
        case 0:
            if (menu_context == StartMenuContext::SAFARI_ZONE){
                ret = move_cursor_to_position(console, context, SelectionArrowPositionSafariMenu::POKEMON);
            } else if (menu_context == StartMenuContext::NO_DEX){
                ret = move_cursor_to_position(console, context, SelectionArrowPositionNoDexMenu::POKEMON);
            } else {
                ret = move_cursor_to_position(console, context, SelectionArrowPositionStartMenu::POKEMON);
            }

            if (ret < 0){
                console.log("Failed to navigate to POKEMON on the start menu.");
                errors++;
                context.wait_for_all_requests();
                pbf_mash_button(context, BUTTON_B, 2000ms);
                start_menu_is_open = false;
            } else {
                console.log("Navigated to POKEMON on the start menu");
                context.wait_for_all_requests();
                pbf_press_button(context, BUTTON_A, 200ms, 1300ms);
            }
            continue;
        case 1:
            console.log("Party menu opened.");
            return;
        default:
            console.log("Failed to open party menu.");
            errors++;
            pbf_mash_button(context, BUTTON_B, 2000ms);
            start_menu_is_open = false;
            continue;
        }
    }
}

PartySlot detect_last_occupied_party_slot(ConsoleHandle& console){
    const auto snapshot = console.video().snapshot();
    constexpr std::array slots{
        PartySlot::SIX,
        PartySlot::FIVE,
        PartySlot::FOUR,
        PartySlot::THREE,
        PartySlot::TWO,
    };

    for (PartySlot slot : slots){
        PartyEmptySlotDetector empty_slot_detector(COLOR_RED, slot);
        if (!empty_slot_detector.detect(snapshot)){
            return slot;
        }
    }

    return PartySlot::ONE;
}

void open_bag_from_overworld(ConsoleHandle& console, ProControllerContext& context, StartMenuContext menu_context){
    uint16_t errors = 0;
    bool start_menu_is_open = false;
    while (true){
        if (errors > 5){
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "open_party_menu_from_overworld(): Failed to open party menu 5 times in a row.",
                console
            );
        }

        context.wait_for_all_requests();
        if (!start_menu_is_open){
            open_start_menu(console, context); // This is unavoidable since we cannot detect the overworld.
            start_menu_is_open = true;
        }

        StartMenuWatcher start_menu(COLOR_RED);
        BagWatcher bag(COLOR_RED);

        int ret = wait_until(
            console, context, 10000ms,
            { start_menu, bag }
        );

        switch (ret){
        case 0:
            if (menu_context == StartMenuContext::SAFARI_ZONE){
                ret = move_cursor_to_position(console, context, SelectionArrowPositionSafariMenu::BAG);
            } else if (menu_context == StartMenuContext::NO_DEX){
                ret = move_cursor_to_position(console, context, SelectionArrowPositionNoDexMenu::BAG);
            } else {
                ret = move_cursor_to_position(console, context, SelectionArrowPositionStartMenu::BAG);
            }

            if (ret < 0){
                console.log("Failed to navigate to BAG on the start menu.");
                errors++;
                context.wait_for_all_requests();
                pbf_mash_button(context, BUTTON_B, 2000ms);
                start_menu_is_open = false;
            } else {
                console.log("Navigated to BAG on the start menu");
                context.wait_for_all_requests();
                pbf_press_button(context, BUTTON_A, 200ms, 1300ms);
            }
            continue;
        case 1:
            console.log("Bag opened.");
            return;
        default:
            console.log("Failed to open bag.");
            errors++;
            pbf_mash_button(context, BUTTON_B, 2000ms);
            start_menu_is_open = false;
            continue;
        }
    }
}

void use_sweet_scent_from_overworld(ConsoleHandle& console, ProControllerContext& context, int from_last){
    uint16_t errors = 0;
    
    while (true){
        if (errors > 5){
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "use_teleport_from_overworld(): Failed to use Teleport 5 times in a row.",
                console
            );
        }

        open_party_menu_from_overworld(console, context);
        // navigate to last party slot
        for (int i=0; i<(2+from_last); i++){
            pbf_move_left_joystick(context, {0, +1}, 200ms, 300ms);
        }

        PartySelectionWatcher sweetscent_selected(COLOR_RED);

        context.wait_for_all_requests();
        int ret = run_until<ProControllerContext>(
            console, context,
            [](ProControllerContext& context){
                pbf_press_button(context, BUTTON_A, 200ms, 1800ms);
            },
            { sweetscent_selected }
        );

        if (ret < 0){
            console.log("Failed to select Sweet Scent user.");
            errors++;
            pbf_mash_button(context, BUTTON_B, 3000ms);
            continue;
        }
        
        // select Sweet Scent (2nd option, but maybe HMs could change this)
        pbf_move_left_joystick(context, {0, -1}, 200ms, 300ms);
        pbf_press_button(context, BUTTON_A, 200ms, 1800ms);
        pbf_press_button(context, BUTTON_A, 200ms, 800ms);

        context.wait_for_all_requests();
        console.log("Used Sweet Scent.");
        return;
    }
}

void use_teleport_from_overworld(ConsoleHandle& console, ProControllerContext& context){
    uint16_t errors = 0;
    
    while (true){
        if (errors > 5){
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "use_teleport_from_overworld(): Failed to use Teleport 5 times in a row.",
                console
            );
        }

        open_party_menu_from_overworld(console, context);
        // navigate to last party slot
        pbf_move_left_joystick(context, {0, +1}, 200ms, 300ms);
        pbf_move_left_joystick(context, {0, +1}, 200ms, 300ms);

        PartySelectionWatcher teleporter_selected(COLOR_RED);

        context.wait_for_all_requests();
        int ret = run_until<ProControllerContext>(
            console, context,
            [](ProControllerContext& context){
                pbf_press_button(context, BUTTON_A, 200ms, 1800ms);
            },
            { teleporter_selected }
        );

        if (ret < 0){
            console.log("Failed to select Teleport user.");
            errors++;
            pbf_mash_button(context, BUTTON_B, 3000ms);
            continue;
        }
        
        // select Teleport (2nd option, but maybe HMs could change this)
        pbf_move_left_joystick(context, {0, -1}, 200ms, 300ms);
        pbf_press_button(context, BUTTON_A, 200ms, 1800ms);
        pbf_press_button(context, BUTTON_A, 200ms, 2800ms);

        BlackScreenWatcher teleport_transition(COLOR_RED);

        context.wait_for_all_requests();
        ret = wait_until(
            console, context, 20000ms,
            {teleport_transition}
        );

        if (ret < 0){
            console.log("Failed to use Teleport");
            errors++;
            pbf_mash_button(context, BUTTON_B, 4000ms);
            continue;
        }

        pbf_wait(context, 3000ms);
        context.wait_for_all_requests();
        console.log("Used Teleport.");
        return;
    }
}

void open_fly_map_from_overworld(ConsoleHandle& console, ProControllerContext& context){
    uint16_t errors = 0;
    while (true){
        if (errors > 5){
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "open_fly_map_from_overworld(): Failed to open Fly map 5 times in a row.",
                console
            );
        }

        open_party_menu_from_overworld(console, context);
        // navigate to last party slot
        pbf_move_left_joystick(context, {0, +1}, 200ms, 300ms);
        pbf_move_left_joystick(context, {0, +1}, 200ms, 300ms);

        PartySelectionWatcher fly_user_selected(COLOR_RED);

        context.wait_for_all_requests();
        int ret = run_until<ProControllerContext>(
            console, context,
            [](ProControllerContext& context) {
                pbf_press_button(context, BUTTON_A, 200ms, 1800ms);
            },
            { fly_user_selected }
        );

        if (ret < 0){
            console.log("Failed to select Fly user.");
            errors++;
            pbf_mash_button(context, BUTTON_B, 3000ms);
            continue;
        }
        
        // select Fly (2nd option, but maybe other HMs could change this)
        KantoMapWatcher map_opened(COLOR_RED);
        context.wait_for_all_requests();
        pbf_move_left_joystick(context, {0, -1}, 200ms, 300ms);
        pbf_press_button(context, BUTTON_A, 200ms, 0ms);
        ret = wait_until(
            console, context, 20000ms,
            {map_opened}
        );

        if (ret < 0){
            console.log("Failed to detect Kanto map");
            errors++;
            pbf_mash_button(context, BUTTON_B, 4000ms);
            continue;
        }

        pbf_wait(context, 1000ms);
        context.wait_for_all_requests();
        console.log("Kanto map detected.");
        return;
    }
}

void fly_from_kanto_map(ConsoleHandle& console, ProControllerContext& context, KantoFlyLocation destination){
    uint64_t errors = 0;
    
    while (true){
        if (errors > 5){
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "fly_from_kanto_map(): Failed to inititate Fly five times in a row.",
                console
            ); 
        }

        pbf_move_left_joystick(context, {-1, +1}, 4000ms, 500ms);
        context.wait_for_all_requests();

        // blindly move the cursor to the specified fly spot
        switch (destination){
        case KantoFlyLocation::pallettown:
            pbf_move_left_joystick(context, {0, -1}, 900ms, 100ms);
            pbf_move_left_joystick(context, {+1, 0}, 317ms, 100ms);
            break;
        case KantoFlyLocation::viridiancity:
            pbf_move_left_joystick(context, {0, -1}, 633ms, 100ms);
            pbf_move_left_joystick(context, {+1, 0}, 317ms, 100ms);
            break;
        case KantoFlyLocation::pewtercity:
            pbf_move_left_joystick(context, {0, -1}, 317ms, 100ms);
            pbf_move_left_joystick(context, {+1, 0}, 317ms, 100ms);
            break;
        case KantoFlyLocation::route4:
            pbf_move_left_joystick(context, {0, -1}, 233ms, 100ms);
            pbf_move_left_joystick(context, {+1, 0}, 633ms, 100ms);
            break;
        case KantoFlyLocation::ceruleancity:
            pbf_move_left_joystick(context, {0, -1}, 233ms, 100ms);
            pbf_move_left_joystick(context, {+1, 0}, 1150ms, 100ms);
            break;
        case KantoFlyLocation::vermilioncity:
            pbf_move_left_joystick(context, {0, -1}, 700ms, 100ms);
            pbf_move_left_joystick(context, {+1, 0}, 1150ms, 100ms);
            break;
        case KantoFlyLocation::route10:
            pbf_move_left_joystick(context, {0, -1}, 233ms, 100ms);
            pbf_move_left_joystick(context, {+1, 0}, 1500ms, 100ms);
            break;
        case KantoFlyLocation::lavendertown:
            pbf_move_left_joystick(context, {0, -1}, 467ms, 100ms);
            pbf_move_left_joystick(context, {+1, 0}, 1500ms, 100ms);
            break;
        case KantoFlyLocation::celadoncity:
            pbf_move_left_joystick(context, {0, -1}, 467ms, 100ms);
            pbf_move_left_joystick(context, {+1, 0}, 900ms, 100ms);
            break;
        case KantoFlyLocation::saffroncity:
            pbf_move_left_joystick(context, {0, -1}, 467ms, 100ms);
            pbf_move_left_joystick(context, {+1, 0}, 1150ms, 100ms);
            break;
        case KantoFlyLocation::fuschiacity:
            pbf_move_left_joystick(context, {0, -1}, 967ms, 100ms);
            pbf_move_left_joystick(context, {+1, 0}, 967ms, 100ms);
            break;
        case KantoFlyLocation::cinnabarisland:
            pbf_move_left_joystick(context, {0, -1}, 1100ms, 100ms);
            pbf_move_left_joystick(context, {+1, 0}, 317ms, 100ms);
            break;
        case KantoFlyLocation::indigoplateau:
            pbf_move_left_joystick(context, {0, -1}, 200ms, 100ms);
            pbf_move_left_joystick(context, {+1, 0}, 150ms, 100ms);
            break;
        default:
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "fly_from_kanto_map(): Unimplemented Kanto fly target.",
                console
            );
        }

        BlackScreenWatcher fly_initiated(COLOR_RED);
        context.wait_for_all_requests();
        int ret = run_until<ProControllerContext>(
            console, context,
            [](ProControllerContext& context) {
                // walk up to counter and initiate dialog
                pbf_mash_button(context, BUTTON_A, 5000ms);
            },
            { fly_initiated }
        );
        
        pbf_wait(context, 8000ms);
        context.wait_for_all_requests();

        if (ret == 0) {
            console.log("Fly initiated.");
            return;
        }else{
            errors++;
            console.log("Failed to detect black screen within 5 seconds of attempting to fly.");
            continue;
        }

    }
    
}

void enter_leave_pokecenter(ConsoleHandle& console, ProControllerContext& context, bool leave){
    uint16_t errors = 0;

    while (true){
        if (errors > 5){
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                leave ? "leave_pokecenter(): Failed to exit PokeCenter." : "enter_pokecenter(): Failed to enter PokeCenter.",
                console
            );
        }

        BlackScreenWatcher pokecenter_transition(COLOR_RED);

        int ret = run_until<ProControllerContext>(
            console, context,
            [leave](ProControllerContext& context){
                pbf_move_left_joystick(context, {0, (leave ? -1.0 : +1.0)}, 10000ms, 0ms);
            },
            { pokecenter_transition }
        );

        if (ret < 0){
            console.log(leave ? "Failed to exit PokeCenter." : "Failed to enter PokeCenter.");
            errors++;
            pbf_mash_button(context, BUTTON_B, 1000ms);
            continue;
        }

        pbf_wait(context, 2500ms);
        context.wait_for_all_requests();
        console.log(leave ? "Exited PokeCenter." : "Entered PokeCenter");
        return;
    }
}

void enter_pokecenter(ConsoleHandle& console, ProControllerContext& context){
    enter_leave_pokecenter(console, context, false);
}

void leave_pokecenter(ConsoleHandle& console, ProControllerContext& context){
    enter_leave_pokecenter(console, context, true);
}

void heal_at_pokecenter(ConsoleHandle& console, ProControllerContext& context){
    uint16_t errors = 0;

    while (true){
        if (errors > 5){
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "heal_at_pokecenter(): Failed to initiate PokeCenter dialog.",
                console
            );
        }

        AdvanceWhiteDialogWatcher dialog(COLOR_RED);

        context.wait_for_all_requests();
        int ret = run_until<ProControllerContext>(
            console, context,
            [](ProControllerContext& context){
                // walk up to counter and initiate dialog
                ssf_press_left_joystick(context, {0, +1}, 0ms, 10000ms);
                ssf_mash1_button(context, BUTTON_A, 10000ms);
            },
            { dialog }
        );

        if (ret < 0){
            console.log("Failed to detect PokeCenter dialog within 10 seconds");
            errors++;
            pbf_mash_button(context, BUTTON_B, 2000ms);
            continue;
        }

        console.log("Detected PokeCenter dialog.");
        pbf_mash_button(context, BUTTON_A, 8000ms);
        pbf_mash_button(context, BUTTON_B, 5000ms);
        context.wait_for_all_requests();
        return;
    }
}

int grass_spin(ConsoleHandle& console, ProControllerContext& context, bool leftright, Seconds timeout){
    BlackScreenWatcher battle_triggered(COLOR_RED);
    AdvanceBattleDialogWatcher battle_entered(COLOR_RED);

    context.wait_for_all_requests();
    console.log("Starting grass spin.");
    WallClock deadline = current_time() + timeout;

    int ret = run_until<ProControllerContext>(
        console, context,
        [leftright, deadline](ProControllerContext& context){
            while (current_time() < deadline){
                if (leftright){
                    pbf_move_left_joystick(context, {+1, 0}, 33ms, 150ms);
                    pbf_move_left_joystick(context, {-1, 0}, 33ms, 150ms);
                }else{
                    pbf_move_left_joystick(context, {0, +1}, 33ms, 150ms);
                    pbf_move_left_joystick(context, {0, -1}, 33ms, 150ms);
                }
            }
        },
        { battle_triggered, battle_entered }
    );

    if (ret < 0){
        return -1;
    }

    bool encounter_shiny = handle_encounter(console, context, true);
    return encounter_shiny ? 1 : 0;
}

int fish_encounter(ConsoleHandle& console, ProControllerContext& context, Seconds timeout){
    WhiteDialogWatcher fishing_dialog(COLOR_RED);
    BlackScreenWatcher battle_entered(COLOR_RED);
    AdvanceBattleDialogWatcher battle_dialog(COLOR_RED);
    BattleMenuWatcher battle_menu(COLOR_RED);

    context.wait_for_all_requests();
    console.log("Starting fish encounter.");
    WallClock start = current_time();

    while (true){
        if (current_time() - start > timeout){
            console.log("No pokemon hooked after timeout.");
            return -1;
        }

        pbf_press_button(context, BUTTON_MINUS, 200ms, 200ms);
        context.wait_for_all_requests();

        int ret = wait_until(
            console, context,
            std::chrono::milliseconds(2000),
            { fishing_dialog, battle_entered, battle_dialog, battle_menu }
        );

        if (ret == 0){
            console.log("Fishing dialog detected.");
            pbf_press_button(context, BUTTON_B, 200ms, 200ms);
            context.wait_for_all_requests();
        } else if (ret == 1 || ret == 2 || ret == 3){
            console.log("Battle entered.");
            break;
        }
    }

    bool encounter_shiny = handle_encounter(console, context, true);
    return encounter_shiny ? 1 : 0;
}

void switch_party_lead_overworld(ConsoleHandle& console, ProControllerContext& context, int game_slot_1indexed){
    //  game_slot_1indexed is the 1-based party slot to promote to lead (must be 2–6).
    //  Navigation convention used throughout this file:
    //    {+1, 0} = right,  {-1, 0} = left
    //    {0, -1} = down (south),  {0, +1} = up (north)
    //  Party list: right from slot 1 enters the right column (slot 2).
    //              down {0,-1} advances within the right column.
    //  Sub-menu:   one {0,-1} down from option 1 (STATS) reaches option 2 (SWITCH).

    if (game_slot_1indexed < 2){
        return;
    }

    open_party_menu_from_overworld(console, context);

    //  Navigate to target slot: right into the right column, then down (N-2) more times.
    pbf_move_left_joystick(context, {+1, 0}, 200ms, 300ms);
    for (int i = 2; i < game_slot_1indexed; i++){
        pbf_move_left_joystick(context, {0, -1}, 200ms, 300ms);
    }

    //  Open the Pokémon's context sub-menu.
    PartySelectionWatcher selection_open(COLOR_RED);
    context.wait_for_all_requests();
    int ret = run_until<ProControllerContext>(
        console, context,
        [](ProControllerContext& ctx){
            pbf_press_button(ctx, BUTTON_A, 200ms, 1800ms);
        },
        { selection_open }
    );
    if (ret < 0){
        OperationFailedException::fire(
            ErrorReport::SEND_ERROR_REPORT,
            "switch_party_lead_overworld(): Failed to open party selection sub-menu.",
            console
        );
    }

    //  One down moves to SWITCH (option 2 under STATS).
    pbf_move_left_joystick(context, {0, -1}, 200ms, 300ms);
    pbf_press_button(context, BUTTON_A, 200ms, 500ms);

    //  Navigate back to slot 1: up to slot 2, then left.
    for (int i = 2; i < game_slot_1indexed; i++){
        pbf_move_left_joystick(context, {0, +1}, 200ms, 300ms);
    }
    pbf_move_left_joystick(context, {-1, 0}, 200ms, 300ms);

    //  Confirm the swap.
    pbf_press_button(context, BUTTON_A, 200ms, 800ms);

    //  Close the party menu.
    pbf_press_button(context, BUTTON_B, 200ms, 300ms);
    pbf_press_button(context, BUTTON_B, 200ms, 800ms);
    close_start_menu(console, context);
    context.wait_for_all_requests();
    console.log("Party lead swapped with slot " + std::to_string(game_slot_1indexed) + ".");
}

void select_forced_switch_slot(ConsoleHandle& console, ProControllerContext& context, int game_slot_1indexed){
    //  Called after spam_first_move() returns BattleResult::playerfainted when alive allies remain.
    //  The game shows the forced-switch party screen after advancing the faint dialog.
    //  Navigate to game_slot_1indexed and send that Pokémon into battle.

    PartyMenuWatcher party_screen(COLOR_RED);
    BattleMenuWatcher battle_menu(COLOR_RED);

    //  After the active Pokémon faints, the game shows a sequence:
    //    1. "<NAME> fainted!" advance dialog (red triangle on teal background)
    //    2. "Use next POKéMON?" Yes/No prompt (default cursor on Yes)
    //    3. Forced-switch party screen
    //
    //  Critical: we must NOT press B at step 2 — B selects "No" and forfeits
    //  the battle (causes a whiteout). To avoid blowing past the prompt, we
    //  identify the current screen on every iteration before pressing anything,
    //  and we use A (not B) so that even if the Yes/No detector misses a frame,
    //  pressing A on the prompt still selects Yes.
    //
    //  A is safe on every screen we may encounter here:
    //    - A on the faint dialog → advances
    //    - A on the Yes/No prompt (cursor defaults to Yes) → selects Yes
    //    - A on the party screen → would open the slot sub-menu (bad), so we
    //      detect the party screen first and break out before pressing A.
    console.log("Forced switch: state-machine loop until party screen appears.");
    context.wait_for_all_requests();

    AdvanceBattleDialogWatcher faint_dialog(COLOR_RED);
    BattleLearnDialogWatcher use_next_prompt(COLOR_RED);

    bool reached_party = false;
    for (int iteration = 0; iteration < 20; iteration++){
        int state = wait_until(
            console, context,
            std::chrono::seconds(5),
            { faint_dialog, use_next_prompt, party_screen }
        );

        if (state == 2){
            console.log("State: party screen detected. Proceeding to slot navigation.");
            reached_party = true;
            break;
        }

        //  state 0 = faint dialog, state 1 = Yes/No prompt, -1 = unrecognized.
        //  In every case A is the correct/safe button to press: it advances the
        //  faint dialog, confirms Yes on the prompt, and at worst on an
        //  unrecognized in-battle dialog it advances or repeats safely.
        const char* state_name =
            state == 0 ? "faint advance dialog" :
            state == 1 ? "\"Use next POKéMON?\" Yes/No prompt" :
            "unrecognized (5s timeout)";
        console.log(std::string("State: ") + state_name + ". Pressing A.");
        pbf_press_button(context, BUTTON_A, 200ms, 1000ms);
        context.wait_for_all_requests();
    }
    if (!reached_party){
        OperationFailedException::fire(
            ErrorReport::SEND_ERROR_REPORT,
            "select_forced_switch_slot(): Party screen did not appear after 20 advance iterations.",
            console
        );
    }

    //  The forced-switch party screen may start with the cursor on the first
    //  non-fainted slot rather than slot 1. Press left once to guarantee we
    //  land on slot 1 (the large left panel) before doing slot arithmetic.
    //  Pressing left from slot 1 is a no-op; from any right-column slot it
    //  returns to slot 1.
    context.wait_for_all_requests();
    pbf_wait(context, 500ms);           // let queued B presses clear and screen settle
    context.wait_for_all_requests();
    pbf_move_left_joystick(context, {-1, 0}, 200ms, 300ms); // reset to slot 1

    //  Navigate to target slot from slot 1.
    if (game_slot_1indexed >= 2){
        pbf_move_left_joystick(context, {+1, 0}, 200ms, 300ms);
        for (int i = 2; i < game_slot_1indexed; i++){
            pbf_move_left_joystick(context, {0, -1}, 200ms, 300ms);
        }
    }

    //  Press A on the target Pokémon — this opens a context sub-menu
    //  (SUMMARY / SEND OUT / CANCEL) rather than immediately sending them out.
    PartySelectionWatcher selection_open(COLOR_RED);
    context.wait_for_all_requests();
    int ret = run_until<ProControllerContext>(
        console, context,
        [](ProControllerContext& ctx){
            pbf_press_button(ctx, BUTTON_A, 200ms, 1800ms);
        },
        { selection_open }
    );
    if (ret < 0){
        OperationFailedException::fire(
            ErrorReport::SEND_ERROR_REPORT,
            "select_forced_switch_slot(): Context sub-menu did not appear after selecting Pokémon.",
            console
        );
    }

    //  In the in-battle forced-switch sub-menu, SEND OUT is the first/default option.
    //  Press A directly — no navigation required.
    pbf_press_button(context, BUTTON_A, 200ms, 500ms);

    //  Wait for the battle menu to confirm the new Pokémon is active.
    context.wait_for_all_requests();
    ret = wait_until(
        console, context, std::chrono::milliseconds(10000),
        { battle_menu }
    );
    if (ret < 0){
        OperationFailedException::fire(
            ErrorReport::SEND_ERROR_REPORT,
            "select_forced_switch_slot(): Battle menu did not reappear after SEND OUT.",
            console
        );
    }
    context.wait_for_all_requests();
    console.log("Forced switch complete: slot " + std::to_string(game_slot_1indexed) + " sent out.");
}


}
}
}
