/*  Autobattle
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include <chrono>
#include "Common/Cpp/Time.h"
#include "CommonFramework/Exceptions/OperationFailedException.h"
#include "CommonFramework/Notifications/ProgramNotifications.h"
#include "CommonFramework/ProgramStats/StatsTracking.h"
#include "CommonFramework/VideoPipeline/VideoFeed.h"
#include "CommonTools/Async/InferenceRoutines.h"
#include "CommonTools/StartupChecks/StartProgramChecks.h"
#include "CommonTools/StartupChecks/VideoResolutionCheck.h"
#include "CommonTools/VisualDetectors/BlackScreenDetector.h"
#include "CommonTools/VisualDetectors/FrozenImageDetector.h"
#include "NintendoSwitch/Commands/NintendoSwitch_Commands_PushButtons.h"
#include "Pokemon/Pokemon_Strings.h"
#include "PokemonChampions/Inference/PokemonChampions_MenuDetectors.h"
#include "PokemonChampions/Programs/PokemonChampions_Autobattle.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonChampions{

using namespace Pokemon;
using namespace std::chrono_literals;


const EnumDropdownDatabase<MoveSelectMode>& MoveSelectMode_Database(){
    //  The second field is persisted into user configs -- never rename one.
    static const EnumDropdownDatabase<MoveSelectMode> database({
        {MoveSelectMode::MashA,      "mash-a",      "Mash A (take whatever move the cursor lands on)"},
        {MoveSelectMode::FixedSlot,  "fixed-slot",  "Always use one move slot"},
        {MoveSelectMode::CycleSlots, "cycle-slots", "Cycle through move slots, one per turn"},
    });
    return database;
}


Autobattle_Descriptor::Autobattle_Descriptor()
    : SingleSwitchProgramDescriptor(
        "PokemonChampions:Autobattle",
        STRING_POKEMON + " Champions", "Autobattle",
        "Programs/PokemonChampions/Autobattle.html",
        "Repeatedly queue into online battles, play them out, and re-queue.",
        ProgramControllerClass::StandardController_NoRestrictions,
        FeedbackType::REQUIRED,
        AllowCommandsWhenRunning::DISABLE_COMMANDS
    )
{}
class Autobattle_Descriptor::Stats : public StatsTracker{
public:
    Stats()
        : battles(m_stats["Battles"])
        , wins(m_stats["Wins"])
        , losses(m_stats["Losses"])
        , unknown(m_stats["Unknown Result"])
        , errors(m_stats["Errors"])
    {
        m_display_order.emplace_back("Battles");
        m_display_order.emplace_back("Wins");
        m_display_order.emplace_back("Losses");
        m_display_order.emplace_back("Unknown Result", HIDDEN_IF_ZERO);
        m_display_order.emplace_back("Errors", HIDDEN_IF_ZERO);
    }

    std::atomic<uint64_t>& battles;
    std::atomic<uint64_t>& wins;
    std::atomic<uint64_t>& losses;
    std::atomic<uint64_t>& unknown;
    std::atomic<uint64_t>& errors;
};
std::unique_ptr<StatsTracker> Autobattle_Descriptor::make_stats() const{
    return std::unique_ptr<StatsTracker>(new Stats());
}


Autobattle::Autobattle()
    : STOP_AFTER_CURRENT("Battle")
    , BATTLE_LIMIT(
        "<b>Number of Battles to Run:</b><br>"
        "Zero will run until 'Stop after Current Battle' is pressed or the program is manually stopped.",
        LockMode::UNLOCK_WHILE_RUNNING,
        0,
        0
    )
    , MOVE_MODE(
        "<b>Move Selection:</b><br>"
        "How to pick a move once the program knows it is our turn. Move slot navigation is not yet "
        "calibrated against real frames, so 'Mash A' is the only mode that is guaranteed to make a legal "
        "move today.",
        MoveSelectMode_Database(),
        LockMode::UNLOCK_WHILE_RUNNING,
        MoveSelectMode::MashA
    )
    , FIXED_MOVE_SLOT(
        "<b>Move Slot:</b><br>Which move slot to use when 'Always use one move slot' is selected.",
        LockMode::UNLOCK_WHILE_RUNNING,
        1, 1, 4
    )
    , MATCHMAKING_TIMEOUT(
        "<b>Matchmaking Timeout (seconds):</b><br>"
        "How long to wait for an opponent before giving up on the current queue attempt. Online queues can "
        "be genuinely slow, so keep this generous.",
        LockMode::UNLOCK_WHILE_RUNNING,
        300, 30, 3600
    )
    , BATTLE_TIMEOUT(
        "<b>Battle Timeout (minutes):</b><br>"
        "Give up on a battle that runs longer than this and move on to the next one.",
        LockMode::UNLOCK_WHILE_RUNNING,
        20, 2, 120
    )
    , MAX_CONSECUTIVE_FAILURES(
        "<b>Max Consecutive Failures:</b><br>"
        "Stop the program after this many battle cycles fail back-to-back. Guards against sitting in a "
        "broken state forever after a disconnect.",
        LockMode::UNLOCK_WHILE_RUNNING,
        3, 1, 20
    )
    , GO_HOME_WHEN_DONE(false)
    , NOTIFICATION_STATUS_UPDATE("Status Update", true, false, std::chrono::seconds(3600))
    , NOTIFICATIONS({
        &NOTIFICATION_STATUS_UPDATE,
        &NOTIFICATION_PROGRAM_FINISH,
        &NOTIFICATION_ERROR_FATAL,
    })
{
    PA_ADD_OPTION(STOP_AFTER_CURRENT);
    PA_ADD_OPTION(BATTLE_LIMIT);
    PA_ADD_OPTION(MOVE_MODE);
    PA_ADD_OPTION(FIXED_MOVE_SLOT);
    PA_ADD_OPTION(MATCHMAKING_TIMEOUT);
    PA_ADD_OPTION(BATTLE_TIMEOUT);
    PA_ADD_OPTION(MAX_CONSECUTIVE_FAILURES);
    PA_ADD_OPTION(GO_HOME_WHEN_DONE);
    PA_ADD_OPTION(NOTIFICATIONS);
}


bool Autobattle::queue_for_battle(SingleSwitchProgramEnvironment& env, ProControllerContext& context){
    env.console.log("Queueing for a battle.");

    //  TODO(calibration): replace this blind A-mash with the real
    //  hub -> battle mode -> team select -> queue button path once those screens
    //  have been captured. Mashing A walks forward through any confirm-style
    //  menu chain, which is enough to get a bring-up running.
    BattleMenuWatcher battle_menu;
    MatchmakingWatcher matchmaking;
    BlackScreenOverWatcher battle_intro;

    const WallClock deadline = current_time() + std::chrono::seconds((int)MATCHMAKING_TIMEOUT);
    bool saw_any_signal = false;

    while (current_time() < deadline){
        int ret = run_until<ProControllerContext>(
            env.console, context,
            [](ProControllerContext& context){
                pbf_mash_button(context, BUTTON_A, 5000ms);
                context.wait_for_all_requests();
            },
            {battle_menu, matchmaking, battle_intro}
        );
        switch (ret){
        case 0:
            env.console.log("Battle menu detected. Battle has started.");
            return true;
        case 1:
            //  Stop pressing A while queued -- an errant press can cancel the search.
            if (!saw_any_signal){
                env.console.log("Matchmaking in progress. Waiting it out.");
            }
            saw_any_signal = true;
            context.wait_for(5000ms);
            continue;
        case 2:
            env.console.log("Screen transition detected. Assuming the battle is starting.");
            return true;
        default:
            continue;
        }
    }

    if (!saw_any_signal){
        //  Nothing fired for the entire window. With the detectors uncalibrated
        //  that is the expected outcome, so proceed blind rather than failing the
        //  cycle -- the A-mash fallback can still play a battle. Once the
        //  detectors are calibrated this branch stops being reachable and a real
        //  matchmaking timeout falls through to the failure path below.
        env.console.log(
            "No Champions screens recognized while queueing. Detectors are uncalibrated; proceeding blind.",
            COLOR_ORANGE
        );
        return true;
    }

    env.console.log("Timed out waiting for an opponent.", COLOR_ORANGE);
    return false;
}


void Autobattle::select_move(SingleSwitchProgramEnvironment& env, ProControllerContext& context, uint16_t turn){
    const MoveSelectMode mode = MOVE_MODE;

    uint16_t slot = 1;
    switch (mode){
    case MoveSelectMode::MashA:
        pbf_press_button(context, BUTTON_A, 160ms, 1000ms);
        context.wait_for_all_requests();
        return;
    case MoveSelectMode::FixedSlot:
        slot = FIXED_MOVE_SLOT;
        break;
    case MoveSelectMode::CycleSlots:
        slot = (uint16_t)(turn % 4) + 1;
        break;
    }

    env.console.log("Selecting move slot " + std::to_string(slot) + ".");

    //  TODO(calibration): move slot geometry is unknown, so this assumes a
    //  vertical list with the cursor resting on slot 1. Once the move select UI
    //  is captured, replace this with real cursor detection and navigation --
    //  PokemonSV/Inference/Battles/PokemonSV_NormalBattleMenus.h
    //  (MoveSelectDetector::detect_slot / move_to_slot) is the pattern to follow.
    for (uint16_t c = 1; c < slot; c++){
        pbf_press_dpad(context, DPAD_DOWN, 80ms, 300ms);
    }
    pbf_press_button(context, BUTTON_A, 160ms, 1000ms);
    context.wait_for_all_requests();
}


BattleResult Autobattle::run_battle(SingleSwitchProgramEnvironment& env, ProControllerContext& context){
    env.console.log("Battle in progress.");

    const WallClock deadline = current_time() + std::chrono::minutes((int)BATTLE_TIMEOUT);
    uint16_t turn = 0;
    uint16_t frozen_strikes = 0;

    while (current_time() < deadline){
        BattleMenuWatcher battle_menu;
        ForcedSwitchWatcher forced_switch;
        BattleResultWatcher result;
        BlackScreenOverWatcher battle_over;
        FrozenImageDetector frozen(std::chrono::seconds(30), 20.0);

        int ret = run_until<ProControllerContext>(
            env.console, context,
            [](ProControllerContext& context){
                pbf_mash_button(context, BUTTON_A, 10000ms);
                context.wait_for_all_requests();
            },
            {battle_menu, forced_switch, result, battle_over, frozen}
        );
        switch (ret){
        case 0:
            select_move(env, context, turn);
            turn++;
            continue;
        case 1:
            //  A is the only button confirmed safe on a forced switch prompt.
            //  Never press B here -- see the warning on ForcedSwitchDetector.
            env.console.log("Forced switch prompt. Confirming with A.");
            pbf_press_button(context, BUTTON_A, 160ms, 1000ms);
            context.wait_for_all_requests();
            continue;
        case 2:
            env.console.log("Result banner read.");
            return result.result();
        case 3:
            env.console.log("Battle end transition detected.");
            return BattleResult::Unknown;
        case 4:
            frozen_strikes++;
            env.console.log("Screen has not changed in a while.", COLOR_ORANGE);
            if (frozen_strikes >= 3){
                OperationFailedException::fire(
                    ErrorReport::SEND_ERROR_REPORT,
                    "Screen frozen. The game may be stuck or disconnected.",
                    env.console
                );
            }
            continue;
        default:
            continue;
        }
    }

    env.console.log("Battle timed out. Treating it as finished with an unknown result.", COLOR_ORANGE);
    return BattleResult::Unknown;
}


void Autobattle::clear_post_battle(SingleSwitchProgramEnvironment& env, ProControllerContext& context){
    env.console.log("Clearing post-battle screens.");

    //  Rewards, rating change, and any season popups are all A-advanced. Stop as
    //  soon as we are back at a recognized menu, or when the screen goes quiet.
    MainMenuWatcher main_menu;
    FrozenImageDetector frozen(std::chrono::seconds(20), 20.0);

    int ret = run_until<ProControllerContext>(
        env.console, context,
        [](ProControllerContext& context){
            pbf_mash_button(context, BUTTON_A, 30000ms);
            context.wait_for_all_requests();
        },
        {main_menu, frozen}
    );
    if (ret == 0){
        env.console.log("Back at the main menu.");
    }else{
        env.console.log("Main menu not confirmed. Continuing to the next queue attempt.", COLOR_ORANGE);
    }
}


void Autobattle::recover_to_main_menu(SingleSwitchProgramEnvironment& env, ProControllerContext& context){
    env.console.log("Attempting to get back to a known state.", COLOR_ORANGE);

    MainMenuWatcher main_menu;
    int ret = run_until<ProControllerContext>(
        env.console, context,
        [](ProControllerContext& context){
            //  A only. B can forfeit a battle on some prompts, which would turn a
            //  recoverable hiccup into a loss.
            pbf_mash_button(context, BUTTON_A, 20000ms);
            context.wait_for_all_requests();
        },
        {main_menu}
    );
    if (ret == 0){
        env.console.log("Recovered to the main menu.");
    }else{
        env.console.log("Could not confirm the main menu. Continuing anyway.", COLOR_ORANGE);
    }
}


void Autobattle::run_one_cycle(SingleSwitchProgramEnvironment& env, ProControllerContext& context){
    Autobattle_Descriptor::Stats& stats = env.current_stats<Autobattle_Descriptor::Stats>();

    if (!queue_for_battle(env, context)){
        OperationFailedException::fire(
            ErrorReport::NO_ERROR_REPORT,
            "Timed out waiting for an opponent.",
            env.console
        );
    }

    const BattleResult result = run_battle(env, context);

    stats.battles++;
    switch (result){
    case BattleResult::Win:
        stats.wins++;
        break;
    case BattleResult::Loss:
        stats.losses++;
        break;
    default:
        stats.unknown++;
        break;
    }
    env.update_stats();
    env.console.log(std::string("Battle finished. Result: ") + battle_result_name(result));

    clear_post_battle(env, context);
    send_program_status_notification(env, NOTIFICATION_STATUS_UPDATE);
}


void Autobattle::program(SingleSwitchProgramEnvironment& env, ProControllerContext& context){
    assert_16_9_720p_min(env.logger(), env.console);
    StartProgramChecks::check_border(env.console);

    Autobattle_Descriptor::Stats& stats = env.current_stats<Autobattle_Descriptor::Stats>();
    DeferredStopButtonOption::ResetOnExit reset_on_exit(STOP_AFTER_CURRENT);

    const uint64_t battle_limit = BATTLE_LIMIT;
    const uint16_t max_failures = MAX_CONSECUTIVE_FAILURES;
    uint16_t consecutive_failures = 0;

    while (true){
        if (STOP_AFTER_CURRENT.should_stop()){
            env.console.log("Stop requested. Finishing up.");
            break;
        }
        if (battle_limit != 0 && stats.battles.load() >= battle_limit){
            env.console.log("Battle limit reached.");
            break;
        }

        try{
            run_one_cycle(env, context);
            consecutive_failures = 0;
        }catch (OperationFailedException&){
            stats.errors++;
            env.update_stats();
            consecutive_failures++;
            if (consecutive_failures >= max_failures){
                env.console.log("Too many consecutive failures. Giving up.", COLOR_RED);
                throw;
            }
            env.console.log("Battle cycle failed. Attempting recovery.", COLOR_ORANGE);
            recover_to_main_menu(env, context);
        }
    }

    send_program_finished_notification(env, NOTIFICATION_PROGRAM_FINISH);
    GO_HOME_WHEN_DONE.run_end_of_program(context);
}



}
}
}
