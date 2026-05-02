/*  XP Grinder
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include "CommonFramework/Exceptions/OperationFailedException.h"
#include "CommonFramework/Notifications/ProgramNotifications.h"
#include "CommonFramework/ProgramStats/StatsTracking.h"
#include "CommonFramework/VideoPipeline/VideoFeed.h"
#include "CommonTools/Async/InferenceRoutines.h"
#include "CommonTools/VisualDetectors/BlackScreenDetector.h"
#include "CommonTools/VisualDetectors/FrozenImageDetector.h"
#include "NintendoSwitch/Commands/NintendoSwitch_Commands_PushButtons.h"
#include "Pokemon/Pokemon_Strings.h"
#include "PokemonFRLG/Programs/PokemonFRLG_RoutePaths.h"
#include "PokemonFRLG/PokemonFRLG_Navigation.h"
#include "PokemonFRLG_XPGrinder.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

XPGrinder_Descriptor::XPGrinder_Descriptor()
    : SingleSwitchProgramDescriptor(
        "PokemonFRLG:XPGrinder",
        Pokemon::STRING_POKEMON + " FRLG", "XP Grinder",
        "Programs/PokemonFRLG/XPGrinder.html",
        "Trigger wild encounters in place and keep using move 1 until the battle ends.",
        ProgramControllerClass::StandardController_NoRestrictions,
        FeedbackType::REQUIRED,
        AllowCommandsWhenRunning::DISABLE_COMMANDS
    )
{}

struct XPGrinder_Descriptor::Stats : public StatsTracker{
    Stats()
        : encounters(m_stats["Encounters"])
        , battles_won(m_stats["Battles Won"])
        , healing_trips(m_stats["Healing Trips"])
        , times_fainted(m_stats["Times Fainted"])
        , out_of_pp(m_stats["Out of PP"])
        , shinies(m_stats["Shinies"])
        , errors(m_stats["Errors"])
    {
        m_display_order.emplace_back("Encounters");
        m_display_order.emplace_back("Battles Won");
        m_display_order.emplace_back("Healing Trips", HIDDEN_IF_ZERO);
        m_display_order.emplace_back("Times Fainted", HIDDEN_IF_ZERO);
        m_display_order.emplace_back("Out of PP", HIDDEN_IF_ZERO);
        m_display_order.emplace_back("Shinies", HIDDEN_IF_ZERO);
        m_display_order.emplace_back("Errors", HIDDEN_IF_ZERO);
    }

    std::atomic<uint64_t>& encounters;
    std::atomic<uint64_t>& battles_won;
    std::atomic<uint64_t>& healing_trips;
    std::atomic<uint64_t>& times_fainted;
    std::atomic<uint64_t>& out_of_pp;
    std::atomic<uint64_t>& shinies;
    std::atomic<uint64_t>& errors;
};

std::unique_ptr<StatsTracker> XPGrinder_Descriptor::make_stats() const{
    return std::unique_ptr<StatsTracker>(new Stats());
}

XPGrinder::XPGrinder()
    : MAX_BATTLES(
        "<b>Max Battles to Win:</b><br>Set to 0 to continue indefinitely until a stop condition is hit.",
        LockMode::UNLOCK_WHILE_RUNNING,
        0, 0
    )
    , PREVENT_EVOLUTION(
        "<b>Prevent " + Pokemon::STRING_POKEMON + " from evolving</b>",
        LockMode::LOCK_WHILE_RUNNING,
        false
    )
    , STOP_ON_MOVE_LEARN(
        "<b>Quit when a new move is learned</b><br>Stop this program when a new move is learned. If unchecked, new moves will not be learned.",
        LockMode::LOCK_WHILE_RUNNING,
        false
    )
    , IGNORE_SHINIES(
        "<b>Ignore shinies</b><br>Do not stop the program when a wild shiny is encountered.",
        LockMode::LOCK_WHILE_RUNNING,
        false
    )
    , ROTATION_MODE(
        "<b>Party Rotation Mode:</b><br>"
        "Disabled: single-Pokémon mode (original behaviour).<br>"
        "Per Battle: cycle the lead after each won battle so each party member fights equally.<br>"
        "Faint Triggered: switch mid-battle using the forced-switch screen when the active Pokémon faints.<br>"
        "PP Exhaustion: switch to the next member when move 1 runs out of PP instead of healing immediately.",
        {
            {RotationMode::disabled,       "disabled",       "Disabled"},
            {RotationMode::per_battle,     "per_battle",     "Per Battle"},
            {RotationMode::faint_triggered,"faint_triggered","Faint Triggered"},
            {RotationMode::pp_exhaustion,  "pp_exhaustion",  "PP Exhaustion"},
        },
        LockMode::LOCK_WHILE_RUNNING,
        RotationMode::disabled
    )
    , PARTY_SIZE(
        "<b>Party Size:</b><br>Number of Pokémon to rotate through (slots 1–N). Only used when rotation mode is not Disabled.",
        LockMode::LOCK_WHILE_RUNNING,
        1, 1, 6
    )
    , HEAL_ON_FAINT(
        "<b>Heal on faint:</b><br>When the lead party faints, accept the whiteout and resume grinding instead of stopping. The game will warp you to the last visited Pokemon Center and fully heal the party automatically.",
        LockMode::LOCK_WHILE_RUNNING,
        true
    )
    , HEAL_ON_OUT_OF_PP(
        "<b>Heal when move 1 is out of PP:</b><br>Travel to the Pokemon Center to restore PP, then resume grinding instead of stopping.",
        LockMode::LOCK_WHILE_RUNNING,
        true
    )
    , BATTLES_PER_HEAL_TRIP(
        "<b>Battles per heal trip:</b><br>After this many wins, take a heal trip preventively. Set to 0 to disable cadence-based healing.",
        LockMode::LOCK_WHILE_RUNNING,
        0, 0
    )
    , TRAVEL_METHOD(
        "<b>Travel Method:</b><br>How to reach the Pokemon Center for routine heal trips (out-of-PP and cadence). Walk is the early-game option and assumes you are grinding on Route 1.",
        {
            {TravelMethod::fly,      "fly",      "Fly"},
            {TravelMethod::teleport, "teleport", "Teleport"},
            {TravelMethod::walk,     "walk",     "Walk"},
        },
        LockMode::LOCK_WHILE_RUNNING,
        TravelMethod::fly
    )
    , TAKE_VIDEO(
        "<b>Take Video:</b><br>Record a video when the shiny is found.",
        LockMode::UNLOCK_WHILE_RUNNING,
        true
    )
    , GO_HOME_WHEN_DONE(true)
    , NOTIFICATION_SHINY(
        "Shiny found",
        true, true, ImageAttachmentMode::JPG,
        {"Notifs", "Showcase"}
    )
    , NOTIFICATION_STATUS_UPDATE("Status Update", true, false, std::chrono::seconds(3600))
    , NOTIFICATIONS({
        &NOTIFICATION_SHINY,
        &NOTIFICATION_STATUS_UPDATE,
        &NOTIFICATION_PROGRAM_FINISH,
        &NOTIFICATION_ERROR_FATAL,
    })
{
    PA_ADD_OPTION(MAX_BATTLES);
    PA_ADD_OPTION(PREVENT_EVOLUTION);
    PA_ADD_OPTION(STOP_ON_MOVE_LEARN);
    PA_ADD_OPTION(IGNORE_SHINIES);
    PA_ADD_OPTION(ROTATION_MODE);
    PA_ADD_OPTION(PARTY_SIZE);
    PA_ADD_OPTION(HEAL_ON_FAINT);
    PA_ADD_OPTION(HEAL_ON_OUT_OF_PP);
    PA_ADD_OPTION(BATTLES_PER_HEAL_TRIP);
    PA_ADD_OPTION(TRAVEL_METHOD);
    PA_ADD_OPTION(TAKE_VIDEO);
    PA_ADD_OPTION(GO_HOME_WHEN_DONE);
    PA_ADD_OPTION(NOTIFICATIONS);
}

namespace{

const char* travel_method_string(XPGrinder::TravelMethod travel){
    switch (travel){
    case XPGrinder::TravelMethod::fly:      return "Fly";
    case XPGrinder::TravelMethod::teleport: return "Teleport";
    case XPGrinder::TravelMethod::walk:     return "Walk";
    }
    return "?";
}

void whiteout_resume(SingleSwitchProgramEnvironment& env, ProControllerContext& context){
    env.log("Whiteout phase 1: mashing through faint dialogs, watching for screen transition into PC.", COLOR_BLUE);

    WhiteScreenOverWatcher white_fade;
    BlackScreenOverWatcher black_load;
    FrozenImageDetector idle(std::chrono::milliseconds(5000), 10.0);
    int phase1 = run_until<ProControllerContext>(
        env.console, context,
        [](ProControllerContext& subcontext){
            pbf_mash_button(subcontext, BUTTON_B, 30000ms);
        },
        {white_fade, black_load, idle}
    );
    switch (phase1){
    case 0:  env.log("Whiteout phase 1 done: WhiteScreenOverWatcher fired (whiteout fade detected).", COLOR_BLUE); break;
    case 1:  env.log("Whiteout phase 1 done: BlackScreenOverWatcher fired (PC interior loaded).", COLOR_BLUE); break;
    case 2:  env.log("Whiteout phase 1 done: FrozenImageDetector fired (dialogs cleared, screen stable).", COLOR_BLUE); break;
    default: env.log("Whiteout phase 1: 30s budget elapsed without a screen-transition signal. Proceeding anyway.", COLOR_RED); break;
    }

    env.log("Whiteout phase 2: clearing 'scurried back to a Pokemon Center' dialog.", COLOR_BLUE);
    pbf_mash_button(context, BUTTON_B, 5000ms);
    context.wait_for_all_requests();

    env.log("Whiteout phase 3: leaving PC and walking back to Route 1 grass.", COLOR_BLUE);
    leave_pokecenter(env.console, context);
    walk_to_route1(env, context);
    env.log("Heal trip complete (whiteout). Resuming grinding.", COLOR_BLUE);
}

void routine_heal_trip(
    SingleSwitchProgramEnvironment& env, ProControllerContext& context,
    XPGrinder::TravelMethod travel
){
    env.log(std::string("Heal trip phase 1: traveling to the Pokemon Center via ") + travel_method_string(travel) + ".", COLOR_BLUE);
    switch (travel){
    case XPGrinder::TravelMethod::fly:
        open_fly_map_from_overworld(env.console, context);
        fly_from_kanto_map(env.console, context, KantoFlyLocation::viridiancity);
        break;
    case XPGrinder::TravelMethod::teleport:
        use_teleport_from_overworld(env.console, context);
        break;
    case XPGrinder::TravelMethod::walk:
        walk_from_route1_to_pokecenter(env, context);
        break;
    }
    env.log("Heal trip phase 2: entering PC, healing party, leaving PC.", COLOR_BLUE);
    enter_pokecenter(env.console, context);
    heal_at_pokecenter(env.console, context);
    leave_pokecenter(env.console, context);
    env.log("Heal trip phase 3: walking back to Route 1 grass.", COLOR_BLUE);
    walk_to_route1(env, context);
    env.log("Heal trip complete. Resuming grinding.", COLOR_BLUE);
}

//  Tracks party rotation state across battles.
struct PartyState{
    int party_size = 1;
    int current_rotation = 0;   // 0-indexed rotation position (not a raw game slot)
    int game_slot[6];           // game_slot[rotation_index] = current 1-indexed game slot
    bool fainted[6] = {};
    bool out_of_pp[6] = {};

    explicit PartyState(int size)
        : party_size(size)
    {
        for (int i = 0; i < 6; i++){
            game_slot[i] = i + 1;  // game slots are 1-indexed
            fainted[i] = false;
            out_of_pp[i] = false;
        }
    }

    //  Returns the next rotation index that is alive (not fainted).
    //  If skip_pp_exhausted is true, also skips out-of-PP slots.
    //  Returns -1 if no eligible slot exists.
    int next_alive(bool skip_pp_exhausted = false) const{
        for (int offset = 1; offset < party_size; offset++){
            int idx = (current_rotation + offset) % party_size;
            if (fainted[idx]) continue;
            if (skip_pp_exhausted && out_of_pp[idx]) continue;
            return idx;
        }
        return -1;
    }

    bool all_fainted() const{
        for (int i = 0; i < party_size; i++){
            if (!fainted[i]) return false;
        }
        return true;
    }

    //  True when every slot needs healing (all fainted, or all PP-exhausted in pp_exhaustion mode).
    bool all_need_heal(bool pp_mode) const{
        if (all_fainted()) return true;
        if (!pp_mode) return false;
        for (int i = 0; i < party_size; i++){
            if (!fainted[i] && !out_of_pp[i]) return false;
        }
        return true;
    }

    //  Call after swapping rotation_from's physical slot with rotation_to's physical slot.
    void on_swap(int rotation_from, int rotation_to){
        std::swap(game_slot[rotation_from], game_slot[rotation_to]);
    }
};

} // namespace

void XPGrinder::program(SingleSwitchProgramEnvironment& env, ProControllerContext& context){
    /*
    * Settings: Text Speed fast
    * Setup: Stand on Route 1 grass with the desired lead active.
    *        Last visited Pokemon Center should be Viridian City.
    * Multi-party rotation: set ROTATION_MODE and PARTY_SIZE as needed.
    */

    XPGrinder_Descriptor::Stats& stats = env.current_stats<XPGrinder_Descriptor::Stats>();

    home_black_border_check(env.console, context);

    const bool multi_party = (ROTATION_MODE != RotationMode::disabled) && (PARTY_SIZE > 1);
    const int party_size = multi_party ? (int)(uint64_t)PARTY_SIZE : 1;

    auto bool_str = [](bool b){ return b ? "true" : "false"; };
    env.log("Starting XP Grinder.", COLOR_BLUE);
    env.log(
        "Config: MAX_BATTLES=" + std::to_string((uint64_t)MAX_BATTLES) +
        "; PREVENT_EVOLUTION=" + bool_str(PREVENT_EVOLUTION) +
        "; STOP_ON_MOVE_LEARN=" + bool_str(STOP_ON_MOVE_LEARN) +
        "; IGNORE_SHINIES=" + bool_str(IGNORE_SHINIES) +
        "; ROTATION_MODE=" + std::to_string((int)(RotationMode)ROTATION_MODE) +
        "; PARTY_SIZE=" + std::to_string(party_size) +
        "; HEAL_ON_FAINT=" + bool_str(HEAL_ON_FAINT) +
        "; HEAL_ON_OUT_OF_PP=" + bool_str(HEAL_ON_OUT_OF_PP) +
        "; BATTLES_PER_HEAL_TRIP=" + std::to_string((uint64_t)BATTLES_PER_HEAL_TRIP) +
        "; TRAVEL_METHOD=" + travel_method_string(TRAVEL_METHOD) + ".",
        COLOR_BLUE
    );

    bool spin_leftright = true;
    uint8_t failed_encounters = 0;
    bool stop_program = false;
    PartyState party(party_size);

    while (!stop_program && (MAX_BATTLES == 0 || stats.battles_won.load() < MAX_BATTLES)){
        try{
            if (failed_encounters >= 5){
                OperationFailedException::fire(
                    ErrorReport::SEND_ERROR_REPORT,
                    "Failed to trigger a wild encounter within 60 seconds 5 times in a row.",
                    env.console
                );
            }

            int encounter = grass_spin(env.console, context, spin_leftright);
            spin_leftright = !spin_leftright;

            if (encounter < 0){
                failed_encounters++;
                env.log("Failed to trigger encounter.");
                pbf_mash_button(context, BUTTON_B, 1000ms);
                context.wait_for_all_requests();
                continue;
            }

            failed_encounters = 0;
            stats.encounters++;

            if (encounter == 1){
                env.log("Shiny found!");
                stats.shinies++;
                env.update_stats();

                VideoSnapshot screen = env.console.video().snapshot();
                send_program_notification(
                    env,
                    NOTIFICATION_SHINY,
                    COLOR_YELLOW,
                    "Shiny found!",
                    {}, "",
                    screen,
                    true
                );
                if (TAKE_VIDEO){
                    pbf_press_button(context, BUTTON_CAPTURE, 2000ms, 0ms);
                }
                if (!IGNORE_SHINIES){
                    break;
                }
            }

            //  Inner battle loop: handles faint-triggered mid-battle switches.
            bool battle_ongoing = true;
            while (battle_ongoing){
                BattleResult battle_result = spam_first_move(env.console, context);
                switch (battle_result){

                case BattleResult::opponentfainted:{
                    stats.battles_won++;
                    bool move_learned = exit_wild_battle(env.console, context, !!STOP_ON_MOVE_LEARN, !!PREVENT_EVOLUTION);

                    env.update_stats();
                    send_program_status_notification(
                        env, NOTIFICATION_STATUS_UPDATE,
                        "Grinding experience."
                    );

                    if (move_learned && STOP_ON_MOVE_LEARN){
                        VideoSnapshot screen = env.console.video().snapshot();
                        send_program_notification(
                            env,
                            NOTIFICATION_STATUS_UPDATE,
                            COLOR_BLUE,
                            "Stopping: move learned.",
                            {}, "",
                            screen,
                            true
                        );
                        stop_program = true;
                        battle_ongoing = false;
                        break;
                    }

                    if (BATTLES_PER_HEAL_TRIP > 0){
                        uint64_t won = stats.battles_won.load();
                        if (won % BATTLES_PER_HEAL_TRIP == 0){
                            env.log("Cadence threshold reached (" + std::to_string(won) + " battles won). Taking heal trip.", COLOR_BLUE);
                            routine_heal_trip(env, context, TRAVEL_METHOD);
                            party = PartyState(party_size);
                            stats.healing_trips++;
                            failed_encounters = 0;
                            env.update_stats();
                        }else{
                            uint64_t until_next = BATTLES_PER_HEAL_TRIP - (won % BATTLES_PER_HEAL_TRIP);
                            env.log("Battle " + std::to_string(won) + " won. " + std::to_string(until_next) + " until next cadence heal trip.");
                        }
                    }

                    //  Per-battle rotation: cycle the lead to the next alive party member.
                    if (multi_party && ROTATION_MODE == RotationMode::per_battle){
                        int next = party.next_alive();
                        if (next >= 0 && next != party.current_rotation){
                            env.log("Per-battle rotation: switching lead to rotation slot " + std::to_string(next) + " (game slot " + std::to_string(party.game_slot[next]) + ").", COLOR_BLUE);
                            switch_party_lead_overworld(env.console, context, party.game_slot[next]);
                            party.on_swap(party.current_rotation, next);
                            party.current_rotation = next;
                        }
                    }

                    battle_ongoing = false;
                    break;
                }

                case BattleResult::playerfainted:{
                    stats.times_fainted++;
                    party.fainted[party.current_rotation] = true;
                    env.update_stats();

                    if (multi_party && !party.all_fainted()){
                        //  Alive allies remain — use the forced-switch screen.
                        int next = party.next_alive();
                        env.log("Party member fainted. Forced-switch to rotation slot " + std::to_string(next) + " (game slot " + std::to_string(party.game_slot[next]) + ").", COLOR_BLUE);
                        select_forced_switch_slot(env.console, context, party.game_slot[next]);
                        party.current_rotation = next;
                        //  Continue battle_ongoing = true so spam_first_move is called again.
                    }else{
                        //  All fainted (whiteout) or single-Pokémon mode.
                        if (HEAL_ON_FAINT || multi_party){
                            env.log("All party fainted — whiteout. Resuming via PC.", COLOR_BLUE);
                            whiteout_resume(env, context);
                            party = PartyState(party_size);
                            stats.healing_trips++;
                            failed_encounters = 0;
                            env.update_stats();
                        }else{
                            env.log("Lead Pokemon fainted. HEAL_ON_FAINT is off, stopping program.", COLOR_RED);
                            pbf_mash_button(context, BUTTON_B, 5000ms);
                            context.wait_for_all_requests();
                            stop_program = true;
                        }
                        battle_ongoing = false;
                    }
                    break;
                }

                case BattleResult::outofpp:{
                    stats.out_of_pp++;
                    party.out_of_pp[party.current_rotation] = true;
                    env.update_stats();

                    if (multi_party && ROTATION_MODE == RotationMode::pp_exhaustion && !party.all_need_heal(true)){
                        //  PP exhausted but alive allies with PP remain — rotate overworld.
                        int next = party.next_alive(/*skip_pp_exhausted=*/true);
                        env.log("PP exhaustion rotation: switching lead to rotation slot " + std::to_string(next) + " (game slot " + std::to_string(party.game_slot[next]) + ").", COLOR_BLUE);
                        switch_party_lead_overworld(env.console, context, party.game_slot[next]);
                        party.on_swap(party.current_rotation, next);
                        party.current_rotation = next;
                    }else if (HEAL_ON_OUT_OF_PP || (multi_party && party.all_need_heal(true))){
                        env.log("Trigger: move 1 out of PP. Taking routine heal trip.", COLOR_BLUE);
                        routine_heal_trip(env, context, TRAVEL_METHOD);
                        party = PartyState(party_size);
                        stats.healing_trips++;
                        failed_encounters = 0;
                        env.update_stats();
                    }else{
                        env.log("Move 1 is out of PP. HEAL_ON_OUT_OF_PP is off, stopping program.", COLOR_RED);
                        stop_program = true;
                    }
                    battle_ongoing = false;
                    break;
                }

                case BattleResult::unknown:
                    env.log("Battle ended without a faint. Continuing.");
                    env.update_stats();
                    battle_ongoing = false;
                    break;
                }
            }

        }catch (OperationFailedException&){
            stats.errors++;
            throw;
        }
    }
    if (GO_HOME_WHEN_DONE){
        pbf_press_button(context, BUTTON_HOME, 200ms, 1000ms);
    }
    send_program_finished_notification(env, NOTIFICATION_PROGRAM_FINISH);
}

}
}
}
