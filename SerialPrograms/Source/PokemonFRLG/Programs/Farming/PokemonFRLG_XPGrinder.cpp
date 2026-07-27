/*  XP Grinder
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include "Common/Cpp/Color.h"
#include "Common/Cpp/Exceptions.h"
#include "Common/Cpp/Json/JsonValue.h"
#include "CommonFramework/Logging/Logger.h"
#include "CommonFramework/Exceptions/OperationFailedException.h"
#include "CommonFramework/Notifications/ProgramNotifications.h"
#include "CommonFramework/ProgramStats/StatsTracking.h"
#include "CommonFramework/VideoPipeline/VideoFeed.h"
#include "CommonTools/Async/InferenceRoutines.h"
#include "CommonTools/VisualDetectors/BlackScreenDetector.h"
#include "CommonTools/VisualDetectors/FrozenImageDetector.h"
#include "NintendoSwitch/Commands/NintendoSwitch_Commands_PushButtons.h"
#include "Pokemon/Pokemon_Strings.h"
#include "Pokemon/Inference/Pokemon_NameReader.h"
#include "PokemonFRLG/Programs/PokemonFRLG_RoutePaths.h"
#include "PokemonFRLG/Programs/PokemonFRLG_PartyScanner.h"
#include "PokemonFRLG/Programs/PokemonFRLG_KantoMapNavigator.h"
#include "PokemonFRLG/Programs/PokemonFRLG_GrindHealLocations.h"
#include "PokemonFRLG/PokemonFRLG_Navigation.h"
#include "PokemonFRLG_MoveLearnDecider.h"
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
    , GRIND_LOCATION(
        "<b>Grind Location:</b><br>Where to spin for wild encounters. Map-driven navigation routes the program here from the Pokémon Center after each heal trip and after whiteouts.",
        GrindLocationId_Database(),
        LockMode::LOCK_WHILE_RUNNING,
        GrindLocationId::Route1NorthGrass
    )
    , AUTO_HEAL_LOCATION(
        "<b>Heal at the Pokémon Center nearest the grind spot</b><br>"
        "Ignores the Heal Location dropdown below and uses the Center that goes with the grind spot you picked. "
        "Uncheck to choose a specific Center yourself.",
        LockMode::LOCK_WHILE_RUNNING,
        true
    )
    , HEAL_LOCATION(
        "<b>Heal Location:</b><br>Which Pokémon Center to heal at. Only used when the option above is unchecked. "
        "Travel mode dictates how to get there (Fly / Teleport / Walk); the post-heal walk back to the grind spot is always map-driven.",
        HealLocationId_Database(),
        LockMode::LOCK_WHILE_RUNNING,
        HealLocationId::ViridianCity
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
        "<b>Party Size:</b><br>Number of Pokémon to rotate through (slots 1–N). Only used when rotation mode is not Disabled. "
        "Leave at <b>0</b> to detect the party size from the party menu at program start (recommended — no need to update this as you add Pokémon).",
        LockMode::LOCK_WHILE_RUNNING,
        0, 0, 6
    )
    , LANGUAGE(
        "<b>Game Language:</b>",
        Pokemon::PokemonNameReader::instance().languages(),
        LockMode::LOCK_WHILE_RUNNING, true
    )
    , AUTO_SCAN_ON_START(
        "<b>Auto-scan party at program start:</b><br>Walk the party menu and OCR each Pokémon's species/moves before grinding. Required for the smart move-learn decider to know what each Pokémon currently has.",
        LockMode::LOCK_WHILE_RUNNING,
        true
    )
    , IMPORT_TEAM_FILE(
        false,
        "<b>Team File:</b><br>Path to a team file produced by the <i>Team Scanner</i> program. "
        "Relative paths are resolved from the program's working directory.",
        LockMode::LOCK_WHILE_RUNNING,
        "FRLG_Team.json",
        "FRLG_Team.json"
    )
    , IMPORT_TEAM_BUTTON(
        "<b>Import the Team File into the table below:</b>",
        "Import Team From File"
    )
    , AUTO_RANK_MOVES(
        "<b>Auto-pick moves:</b><br>When a level-up offers a move you did not list in the table below, decide by "
        "STAB-adjusted base power: take it if it beats the weakest unlisted move the " + Pokemon::STRING_POKEMON + " knows. "
        "Moves you <i>do</i> list are always kept and never forgotten, so use the table to pin anything you want protected "
        "(HM moves, status moves, coverage picks). Turn this off to decline every unlisted move.",
        LockMode::LOCK_WHILE_RUNNING,
        true
    )
    , TEAM_TABLE()
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
    PA_ADD_OPTION(IGNORE_SHINIES);
    PA_ADD_OPTION(GRIND_LOCATION);
    PA_ADD_OPTION(AUTO_HEAL_LOCATION);
    PA_ADD_OPTION(HEAL_LOCATION);
    PA_ADD_OPTION(ROTATION_MODE);
    PA_ADD_OPTION(PARTY_SIZE);
    PA_ADD_OPTION(LANGUAGE);
    PA_ADD_OPTION(AUTO_SCAN_ON_START);
    PA_ADD_OPTION(IMPORT_TEAM_FILE);
    PA_ADD_OPTION(IMPORT_TEAM_BUTTON);
    PA_ADD_OPTION(AUTO_RANK_MOVES);
    PA_ADD_OPTION(TEAM_TABLE);
    PA_ADD_OPTION(HEAL_ON_FAINT);
    PA_ADD_OPTION(HEAL_ON_OUT_OF_PP);
    PA_ADD_OPTION(BATTLES_PER_HEAL_TRIP);
    PA_ADD_OPTION(TRAVEL_METHOD);
    PA_ADD_OPTION(TAKE_VIDEO);
    PA_ADD_OPTION(GO_HOME_WHEN_DONE);
    PA_ADD_OPTION(NOTIFICATIONS);

    IMPORT_TEAM_BUTTON.add_listener(*this);
}
XPGrinder::~XPGrinder(){
    IMPORT_TEAM_BUTTON.remove_listener(*this);
}
void XPGrinder::on_press(ButtonCell& button){
    //  Runs on the GUI thread when the user clicks "Import Team From File".
    //  Loads the team file into TEAM_TABLE so the user can review/edit it
    //  before running. The file is the XP Grinder team-table serialization
    //  written by the Team Scanner program.
    std::string path = IMPORT_TEAM_FILE;
    if (path.empty()){
        path = "FRLG_Team.json";
    }
    try{
        JsonValue json = load_json_file(path);
        TEAM_TABLE.load_json(json);
        global_logger_tagged().log(
            "XP Grinder: imported team from '" + path + "'.", COLOR_BLUE
        );
    }catch (const Exception& e){
        global_logger_tagged().log(
            "XP Grinder: failed to import team from '" + path + "': " + e.message(),
            COLOR_RED
        );
    }
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

void whiteout_resume(
    SingleSwitchProgramEnvironment& env, ProControllerContext& context,
    GrindLocationId grind_location
){
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

    env.log("Whiteout phase 3: leaving PC and walking back to grind location via map navigation.", COLOR_BLUE);
    leave_pokecenter(env.console, context);
    kanto_navigate_to(env, context, goal_for_grind_location(grind_location));
    env.log("Heal trip complete (whiteout). Resuming grinding.", COLOR_BLUE);
}

void routine_heal_trip(
    SingleSwitchProgramEnvironment& env, ProControllerContext& context,
    XPGrinder::TravelMethod travel,
    GrindLocationId grind_location,
    HealLocationId heal_location
){
    env.log(std::string("Heal trip phase 1: traveling to the Pokemon Center via ") + travel_method_string(travel) + ".", COLOR_BLUE);
    switch (travel){
    case XPGrinder::TravelMethod::fly:
        open_fly_map_from_overworld(env.console, context);
        fly_from_kanto_map(env.console, context, fly_for_heal_location(heal_location));
        break;
    case XPGrinder::TravelMethod::teleport:
        use_teleport_from_overworld(env.console, context);
        break;
    case XPGrinder::TravelMethod::walk:
        kanto_navigate_to(env, context, pc_entrance_for_heal_location(heal_location));
        break;
    }
    env.log("Heal trip phase 2: entering PC, healing party, leaving PC.", COLOR_BLUE);
    enter_pokecenter(env.console, context);
    heal_at_pokecenter(env.console, context);
    leave_pokecenter(env.console, context);
    env.log("Heal trip phase 3: walking back to grind location via map navigation.", COLOR_BLUE);
    kanto_navigate_to(env, context, goal_for_grind_location(grind_location));
    env.log("Heal trip complete. Resuming grinding.", COLOR_BLUE);
}

//  Tracks party rotation state across battles.
struct PartyState{
    int party_size = 1;
    int current_rotation = 0;   // 0-indexed rotation position (not a raw game slot)
    int game_slot[6];           // game_slot[rotation_index] = current 1-indexed game slot
    bool fainted[6] = {};
    bool out_of_pp[6] = {};
    //  Cached current move slugs per rotation index, refreshed by scan_party()
    //  at start and scan_party_slot() after each move-learn.
    std::array<std::string, 4> current_moves[6];

    explicit PartyState(int size)
        : party_size(size)
    {
        for (int i = 0; i < 6; i++){
            game_slot[i] = i + 1;  // game slots are 1-indexed
            fainted[i] = false;
            out_of_pp[i] = false;
            for (int m = 0; m < 4; m++){
                current_moves[i][m].clear();
            }
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

    //  Returns the rotation index whose current game slot equals the given 1-indexed slot.
    //  Returns -1 if not found.
    int find_rotation_with_game_slot(int slot) const{
        for (int i = 0; i < party_size; i++){
            if (game_slot[i] == slot) return i;
        }
        return -1;
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

    auto bool_str = [](bool b){ return b ? "true" : "false"; };
    env.log("Starting XP Grinder.", COLOR_BLUE);

    //  Resolve the heal target up front so the whole run uses one value.
    const GrindLocationId grind_location = GRIND_LOCATION;
    const HealLocationId heal_location = AUTO_HEAL_LOCATION
        ? nearest_heal_location(grind_location)
        : (HealLocationId)HEAL_LOCATION;

    //  Resolve the party size before anything depends on it. PARTY_SIZE == 0
    //  means "read it off the party menu", which is the default so the user
    //  never has to keep this option in sync with their actual team.
    //
    //  When auto-scanning we let scan_party() do the detection so the party
    //  menu is only opened once; otherwise we open it just to count slots.
    int party_size = (int)(uint64_t)PARTY_SIZE;
    const bool rotation_enabled = (ROTATION_MODE != RotationMode::disabled);
    std::vector<PartyScanResult> startup_scan;
    bool startup_scan_ok = false;

    if (AUTO_SCAN_ON_START){
        env.log(
            party_size == 0
                ? std::string("Auto-scanning party (size auto-detected) before grinding.")
                : "Auto-scanning party (" + std::to_string(party_size) + " slot(s)) before grinding.",
            COLOR_BLUE
        );
        try{
            startup_scan = scan_party(env, context, LANGUAGE, party_size);
            startup_scan_ok = true;
            if (party_size == 0){
                party_size = (int)startup_scan.size();
            }
        }catch (OperationFailedException& e){
            env.log(std::string("Auto-scan failed: ") + e.message() + ". Continuing with empty move cache (decider will treat all moves as unknown).", COLOR_RED);
            stats.errors++;
        }
    }

    if (party_size == 0){
        //  Either auto-scan is off, or it threw before detecting a size.
        try{
            party_size = detect_party_size(env, context);
        }catch (OperationFailedException& e){
            env.log(std::string("Party-size detection failed: ") + e.message() + ". Falling back to 1 (single-Pokemon mode).", COLOR_RED);
            stats.errors++;
            party_size = 1;
        }
    }
    if (party_size < 1) party_size = 1;
    if (party_size > 6) party_size = 6;

    const bool multi_party = rotation_enabled && (party_size > 1);

    env.log(
        "Config: MAX_BATTLES=" + std::to_string((uint64_t)MAX_BATTLES) +
        "; PREVENT_EVOLUTION=" + bool_str(PREVENT_EVOLUTION) +
        "; IGNORE_SHINIES=" + bool_str(IGNORE_SHINIES) +
        "; GRIND_LOCATION=" + GrindLocationId_Database().find(grind_location)->display +
        "; HEAL_LOCATION=" + HealLocationId_Database().find(heal_location)->display +
        (AUTO_HEAL_LOCATION ? " (auto)" : " (manual)") +
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

    if (startup_scan_ok){
        //  Rotation index = scan order (slot_1indexed - 1) at startup.
        for (const PartyScanResult& r : startup_scan){
            int rot = r.slot_1indexed - 1;
            if (rot >= 0 && rot < 6){
                party.current_moves[rot] = r.read.move_slugs;
                //  Auto-update the team-table species cell when the scan
                //  identifies a species that's different from (or absent
                //  in) the row.
                if (!r.species_slug.empty() && r.species_slug != TEAM_TABLE.species_for((size_t)rot)){
                    env.log(
                        "Slot " + std::to_string(r.slot_1indexed) +
                        ": detected species '" + r.species_slug +
                        "' (was '" + TEAM_TABLE.species_for((size_t)rot) + "'). Updating team table.",
                        COLOR_BLUE
                    );
                    TEAM_TABLE.set_species((size_t)rot, r.species_slug);
                }
            }
        }
    }

    //  Warn the user about any desired moves that the species (and its
    //  evolution chain) cannot learn. Non-blocking — the user can still run
    //  with mismatched picks if they want to.
    {
        std::vector<std::string> warnings = TEAM_TABLE.validate_against_learnsets();
        for (const std::string& w : warnings){
            env.log(std::string("Team-table validation warning: ") + w, COLOR_RED);
        }
        if (warnings.empty()){
            env.log("Team-table validation: all desired moves are in their species' chain learnsets.", COLOR_BLUE);
        }
    }

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
                size_t table_index = (size_t)party.current_rotation;
                MoveLearnDecider decider = TEAM_TABLE.make_decider(
                    table_index, !!AUTO_RANK_MOVES,
                    party.current_moves[party.current_rotation]
                );
                std::vector<size_t> priority = decider.battle_move_priority(
                    party.current_moves[party.current_rotation]
                );

                BattleResult battle_result = spam_first_move(env.console, context, priority);
                switch (battle_result){

                case BattleResult::opponentfainted:{
                    stats.battles_won++;
                    bool evolved = false;
                    WildBattleExit exit_result = exit_wild_battle(
                        env.console, context, false, !!PREVENT_EVOLUTION,
                        &decider, LANGUAGE, &evolved
                    );

                    //  Stop signal: dialog is still active, do not navigate.
                    //  Halt the program here and let the user intervene.
                    if (exit_result == WildBattleExit::StopBattleStuck){
                        VideoSnapshot screen = env.console.video().snapshot();
                        send_program_notification(
                            env,
                            NOTIFICATION_STATUS_UPDATE,
                            COLOR_BLUE,
                            "Stopping: move-learn dialog left active per OnUnknown=Stop policy.",
                            {}, "",
                            screen,
                            true
                        );
                        env.log("Halting program; move-learn dialog still on screen.", COLOR_RED);
                        stop_program = true;
                        battle_ongoing = false;
                        break;
                    }

                    //  exit_wild_battle returns on the battle-end black fade (currently
                    //  black). Wait for the overworld fade-in to complete before any
                    //  subsequent menu navigation; pressing START while the screen is
                    //  still mid-fade can drop inputs and corrupt the post-battle
                    //  switch_party_lead_overworld call below.
                    {
                        BlackScreenOverWatcher overworld_entered(COLOR_RED);
                        int over = wait_until(
                            env.console, context,
                            std::chrono::seconds(5),
                            { overworld_entered }
                        );
                        if (over < 0){
                            env.log("Overworld fade-in not detected within 5s after battle exit. Proceeding anyway.", COLOR_BLUE);
                        }else{
                            env.log("Overworld visible after battle exit.", COLOR_BLUE);
                        }
                        context.wait_for_all_requests();
                    }

                    env.update_stats();
                    send_program_status_notification(
                        env, NOTIFICATION_STATUS_UPDATE,
                        "Grinding experience."
                    );

                    //  Rescan whenever the slot's species or moveset can have
                    //  changed. A level-up evolution changes the species (and
                    //  therefore the learnset the decider reasons about) even
                    //  when no move was offered, so `evolved` must trigger a
                    //  rescan on its own.
                    if (exit_result == WildBattleExit::LearnHandled || evolved){
                        env.log(
                            std::string(evolved ? "Evolution" : "Move learn") +
                            " occurred. Rescanning slot " + std::to_string(party.current_rotation + 1) +
                            " to refresh species and move cache.",
                            COLOR_BLUE
                        );
                        try{
                            PartyScanResult r = scan_party_slot(
                                env, context, LANGUAGE, party.current_rotation + 1
                            );
                            party.current_moves[party.current_rotation] = r.read.move_slugs;
                            //  Evolutions can also fire on level-up. If the
                            //  detected species changed, update the table row
                            //  so subsequent battles use the new species.
                            if (!r.species_slug.empty() && r.species_slug != TEAM_TABLE.species_for((size_t)party.current_rotation)){
                                env.log(
                                    "Slot " + std::to_string(party.current_rotation + 1) +
                                    " evolved: '" + TEAM_TABLE.species_for((size_t)party.current_rotation) +
                                    "' -> '" + r.species_slug + "'. Updating team table.",
                                    COLOR_BLUE
                                );
                                TEAM_TABLE.set_species((size_t)party.current_rotation, r.species_slug);
                            }
                        }catch (OperationFailedException& e){
                            env.log(std::string("Post-learn rescan failed: ") + e.message() + ". Move cache may be stale.", COLOR_RED);
                            stats.errors++;
                        }
                    }

                    if (BATTLES_PER_HEAL_TRIP > 0){
                        uint64_t won = stats.battles_won.load();
                        if (won % BATTLES_PER_HEAL_TRIP == 0){
                            env.log("Cadence threshold reached (" + std::to_string(won) + " battles won). Taking heal trip.", COLOR_BLUE);
                            routine_heal_trip(env, context, TRAVEL_METHOD, grind_location, heal_location);
                            party = PartyState(party_size);
                            stats.healing_trips++;
                            failed_encounters = 0;
                            env.update_stats();
                        }else{
                            uint64_t until_next = BATTLES_PER_HEAL_TRIP - (won % BATTLES_PER_HEAL_TRIP);
                            env.log("Battle " + std::to_string(won) + " won. " + std::to_string(until_next) + " until next cadence heal trip.");
                        }
                    }

                    //  After a forced switch the winner is still in their original slot, not
                    //  slot 1.  Swap them into slot 1 now so the next encounter starts cleanly.
                    if (multi_party && party.game_slot[party.current_rotation] != 1){
                        int winner_slot = party.game_slot[party.current_rotation];
                        env.log("Post-battle normalize: swapping game slot " + std::to_string(winner_slot) + " into slot 1.", COLOR_BLUE);
                        //  Try once; on transient failure (most common cause is the
                        //  overworld not being fully ready), back out of any partial
                        //  menu state and retry once before surfacing a fatal error.
                        bool swapped = false;
                        for (int attempt = 0; attempt < 2 && !swapped; attempt++){
                            try{
                                switch_party_lead_overworld(env.console, context, winner_slot);
                                swapped = true;
                            }catch (OperationFailedException& e){
                                stats.errors++;
                                env.log(
                                    "Post-battle normalize attempt " + std::to_string(attempt + 1) +
                                    " failed: " + e.message() + ". Recovering.",
                                    COLOR_RED
                                );
                                if (attempt == 1){
                                    throw;
                                }
                                //  Back out of any partial menu and let the overworld settle.
                                pbf_mash_button(context, BUTTON_B, 2000ms);
                                context.wait_for_all_requests();
                                pbf_wait(context, 500ms);
                                context.wait_for_all_requests();
                            }
                        }
                        int old_slot1_rotation = party.find_rotation_with_game_slot(1);
                        if (old_slot1_rotation >= 0){
                            party.on_swap(party.current_rotation, old_slot1_rotation);
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
                            whiteout_resume(env, context, grind_location);
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
                        routine_heal_trip(env, context, TRAVEL_METHOD, grind_location, heal_location);
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
