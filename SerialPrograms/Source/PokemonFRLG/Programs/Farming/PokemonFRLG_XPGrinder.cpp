/*  XP Grinder
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include <algorithm>
#include <array>
#include <functional>
#include <vector>
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
#include "PokemonFRLG_MovePlan.h"
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
    , NAVIGATE_TO_GRIND_ON_START(
        "<b>Walk to the grind spot when the program starts:</b><br>"
        "Route to the Grind Location above before the first encounter, instead of assuming you "
        "are already standing in the grass. Uses the same map-driven navigation as the post-heal "
        "walk back, so it only works from somewhere on the mapped area (Viridian City and "
        "Route 1). Turn this off if you always start in position and would rather not pay for "
        "the initial localize.",
        LockMode::LOCK_WHILE_RUNNING,
        true
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
        "PP Exhaustion: switch to the next member when move 1 runs out of PP instead of healing immediately.<br>"
        "Switch Training: keep a weak Pokémon in slot 1 so it is sent out and counts as a battle "
        "participant, then immediately switch to the Fighter Slot below to actually win. Gen 3 splits "
        "EXP among everyone sent out, and switching resolves before the opponent moves, so the trainee "
        "levels without ever taking a hit. This is how you raise a Magikarp.",
        {
            {RotationMode::disabled,        "disabled",        "Disabled"},
            {RotationMode::per_battle,      "per_battle",      "Per Battle"},
            {RotationMode::faint_triggered, "faint_triggered", "Faint Triggered"},
            {RotationMode::pp_exhaustion,   "pp_exhaustion",   "PP Exhaustion"},
            {RotationMode::switch_training, "switch_training", "Switch Training"},
        },
        LockMode::LOCK_WHILE_RUNNING,
        RotationMode::disabled
    )
    , FIGHTER_SLOT(
        "<b>Fighter Slot:</b><br>Only used in Switch Training mode. The party slot (2–6) that is "
        "switched in to do the fighting while slot 1 collects participation EXP. Put something that "
        "can one-shot the local wild Pokémon here.",
        LockMode::LOCK_WHILE_RUNNING,
        2, 2, 6
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
    , AUTOFILL_DESIRED_MOVES(
        "<b>Auto-fill empty move rows from the party scan:</b><br>"
        "For any team row that has no moves chosen, pick the four strongest level-up moves "
        "that " + Pokemon::STRING_POKEMON + " can still reach in its evolution line, biased "
        "toward type coverage and ordered strongest-first. Rows you have filled in yourself "
        "are never touched. Moves that KO the user (Explosion, Self-Destruct) are excluded, "
        "and two-turn moves are de-prioritised as poor grinding defaults — pin those by "
        "hand if you want them.",
        LockMode::LOCK_WHILE_RUNNING,
        true
    )
    , HOLD_EVOLUTION_FOR_MOVES(
        "<b>Hold evolution until stage-locked moves are learned:</b><br>"
        "In Gen 3 the evolved form has its own level-up learnset, and for stone evolutions it "
        "is far smaller — Growlithe loses Flamethrower, Flame Wheel, Agility and Take Down "
        "the instant it becomes Arcanine; Pikachu loses Thunder, Agility and Slam on becoming "
        "Raichu. When a desired move is only learnable at the current stage, this cancels "
        "evolution for that " + Pokemon::STRING_POKEMON + " until it has the move, then lets it "
        "evolve normally. Applies per party member.",
        LockMode::LOCK_WHILE_RUNNING,
        true
    )
    , STOP_WHEN_TEAM_COMPLETE(
        "<b>Stop when every party member has its desired moveset:</b><br>"
        "Moves that are already missed or not learnable by level-up are excluded from the "
        "check, so an impossible pick can never make this run forever.",
        LockMode::LOCK_WHILE_RUNNING,
        false
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
    PA_ADD_OPTION(NAVIGATE_TO_GRIND_ON_START);
    PA_ADD_OPTION(AUTO_HEAL_LOCATION);
    PA_ADD_OPTION(HEAL_LOCATION);
    PA_ADD_OPTION(ROTATION_MODE);
    PA_ADD_OPTION(FIGHTER_SLOT);
    PA_ADD_OPTION(PARTY_SIZE);
    PA_ADD_OPTION(LANGUAGE);
    PA_ADD_OPTION(AUTO_SCAN_ON_START);
    PA_ADD_OPTION(IMPORT_TEAM_FILE);
    PA_ADD_OPTION(IMPORT_TEAM_BUTTON);
    PA_ADD_OPTION(AUTO_RANK_MOVES);
    PA_ADD_OPTION(AUTOFILL_DESIRED_MOVES);
    PA_ADD_OPTION(HOLD_EVOLUTION_FOR_MOVES);
    PA_ADD_OPTION(STOP_WHEN_TEAM_COMPLETE);
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

//  Phases 1 and 2 of whiteout handling: clear the faint dialogs, ride out the
//  warp, and clear the "scurried back to a Pokemon Center" prompt. Split out so
//  the "stop the program on a wipe" path can also leave the game on a clean
//  overworld screen instead of abandoning it mid-dialog.
void whiteout_settle(
    SingleSwitchProgramEnvironment& env, ProControllerContext& context
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
    pbf_mash_button(context, BUTTON_B, 8000ms);
    context.wait_for_all_requests();
}

//  How many encounters between drift checks.
//
//  Measured drift is about 0.007 tiles per encounter (nine tiles over ~1,260 on
//  2026-08-19), so 25 encounters is roughly a fifth of a tile -- comfortably
//  inside the navigator's 4-tile hint radius, which is what keeps the check
//  itself cheap. Deliberately not a user option: there is no value a user could
//  usefully choose here, and getting it wrong silently breaks long runs.
constexpr int BATTLES_PER_DRIFT_CHECK = 25;

//  Wait until the overworld is actually visible again after a battle.
//
//  This used to be a bare BlackScreenOverWatcher, which cannot answer the
//  question being asked. That watcher only fires once it has *seen* black and
//  then seen the black end (BlackScreenDetector.h: `m_has_been_black`).
//  exit_wild_battle normally returns after the battle-end fade is already over,
//  so the watcher never observes black, never fires, and every single battle
//  paid the full 5 s timeout before logging "not detected ... proceeding
//  anyway" -- having confirmed nothing at all. The one case the wait existed to
//  prevent, a screen still mid-fade, was proceeded through regardless.
//
//  Ask the question we actually care about instead: is the screen non-black,
//  and has it stayed non-black? Two consecutive clear frames rule out latching
//  onto a single bright frame partway through a fade. Returns false on timeout.
bool wait_for_overworld_after_battle(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    int timeout_ms = 5000
){
    constexpr int POLL_MS = 200;
    constexpr int REQUIRED_CLEAR_FRAMES = 2;

    BlackScreenDetector black(COLOR_RED);
    int consecutive_clear = 0;
    const int max_polls = timeout_ms / POLL_MS;
    for (int i = 0; i < max_polls; i++){
        context.wait_for_all_requests();
        VideoSnapshot snap = env.console.video().snapshot();
        if (snap && !black.detect(*snap.frame)){
            consecutive_clear++;
            if (consecutive_clear >= REQUIRED_CLEAR_FRAMES){
                return true;
            }
        }else{
            consecutive_clear = 0;
        }
        context.wait_for(std::chrono::milliseconds(POLL_MS));
    }
    return false;
}

//  `on_healed` fires the moment the party is actually restored -- i.e. after the
//  whiteout warp, BEFORE the walk back to the grind spot. If the walk throws, the
//  caller's health bookkeeping has still been updated to match reality; the old
//  code only updated it after the whole trip succeeded, so a failed walk-back left
//  the program believing Pokemon were still fainted.
void whiteout_resume(
    SingleSwitchProgramEnvironment& env, ProControllerContext& context,
    GrindLocationId grind_location,
    const std::function<void()>& on_healed
){
    whiteout_settle(env, context);

    //  The warp itself fully restores HP and PP.
    if (on_healed){
        on_healed();
    }

    env.log("Whiteout phase 3: leaving PC and walking back to grind location via map navigation.", COLOR_BLUE);
    leave_pokecenter(env.console, context);
    kanto_navigate_to(env, context, goal_for_grind_location(grind_location));
    env.log("Heal trip complete (whiteout). Resuming grinding.", COLOR_BLUE);
}

//  See whiteout_resume: `on_healed` fires right after heal_at_pokecenter, not at
//  the end of the trip.
void routine_heal_trip(
    SingleSwitchProgramEnvironment& env, ProControllerContext& context,
    XPGrinder::TravelMethod travel,
    GrindLocationId grind_location,
    HealLocationId heal_location,
    const std::function<void()>& on_healed
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
        //  We have been spinning on the grind tile for the last N battles, so
        //  that is where we are. Seeding it keeps the first fix out of the
        //  ambiguity gate -- a cold match in the middle of a uniform grass field
        //  fails every time (observed 8/18: 16 straight "Position unknown").
        kanto_navigate_to(
            env, context,
            pc_entrance_for_heal_location(heal_location),
            goal_for_grind_location(grind_location)
        );
        break;
    }
    env.log("Heal trip phase 2: entering PC, healing party, leaving PC.", COLOR_BLUE);
    enter_pokecenter(env.console, context);
    heal_at_pokecenter(env.console, context);
    if (on_healed){
        on_healed();
    }
    leave_pokecenter(env.console, context);
    env.log("Heal trip phase 3: walking back to grind location via map navigation.", COLOR_BLUE);
    //  leave_pokecenter() has just put us on the Center's doorway, one tile north
    //  of its entrance goal -- seed that rather than paying for a cold fix.
    kanto_navigate_to(
        env, context,
        goal_for_grind_location(grind_location),
        pc_entrance_for_heal_location(heal_location)
    );
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
    //  Last read level per rotation index; -1 when unknown.
    int level[6];
    //  Desired-moveset plan per rotation index. Rebuilt from the team table
    //  whenever a slot is (re)scanned, which is exactly when its species, level
    //  or moves can have changed.
    MovePlan plan[6];

    explicit PartyState(int size)
        : party_size(size)
    {
        for (int i = 0; i < 6; i++){
            game_slot[i] = i + 1;  // game slots are 1-indexed
            fainted[i] = false;
            out_of_pp[i] = false;
            level[i] = -1;
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

    //  As next_alive(), but prefers a member whose desired moveset is still
    //  unfinished, so a party where one Pokemon is done doesn't keep feeding it
    //  experience while another still needs levels. Falls back to plain
    //  next_alive() when everyone eligible is already finished.
    int next_alive_preferring_unfinished(bool skip_pp_exhausted = false) const{
        for (int offset = 1; offset < party_size; offset++){
            int idx = (current_rotation + offset) % party_size;
            if (fainted[idx]) continue;
            if (skip_pp_exhausted && out_of_pp[idx]) continue;
            if (plan[idx].complete()) continue;
            return idx;
        }
        return next_alive(skip_pp_exhausted);
    }

    //  True when every party member's desired moveset is satisfied. Members with
    //  no desired moves configured count as complete (nothing was asked of them).
    bool all_plans_complete() const{
        for (int i = 0; i < party_size; i++){
            if (!plan[i].complete()) return false;
        }
        return true;
    }

    //  True if ANY party member has desired moves configured. Without this,
    //  a party where nothing was asked for (scan failed, or the user left the
    //  table empty) reads as "everyone is complete" and the completion check
    //  would stop the program immediately, claiming success it never earned.
    bool any_plan_has_goals() const{
        for (int i = 0; i < party_size; i++){
            if (plan[i].has_goals()) return true;
        }
        return false;
    }

    bool all_fainted() const{
        for (int i = 0; i < party_size; i++){
            if (!fainted[i]) return false;
        }
        return true;
    }

    //  Switch training only.
    //
    //  Slot 1 is the trainee and is never a fighter: it is sent out for the EXP
    //  share and withdrawn before the opponent moves. These two ask about the
    //  rest of the party -- the Pokemon that are actually expected to win.
    //
    //  Party order is never touched in switch_training (see
    //  normalize_party_after_battle), so rotation index i is always game slot
    //  i+1 and indices 1..party_size-1 are exactly the fighters.
    //
    //  Returns the first alive fighter's rotation index, or -1 when every
    //  fighter is down and only the trainee is left standing.
    int next_alive_fighter() const{
        for (int i = 1; i < party_size; i++){
            if (!fainted[i]) return i;
        }
        return -1;
    }
    bool only_trainee_left() const{
        return next_alive_fighter() < 0;
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

    //  Returns the rotation index whose current game slot equals the given 1-indexed slot.
    //  Returns -1 if not found.
    int find_rotation_with_game_slot(int slot) const{
        for (int i = 0; i < party_size; i++){
            if (game_slot[i] == slot) return i;
        }
        return -1;
    }

    //  Bookkeeping for switch_party_lead_overworld(N), which SWAPS slot 1 with
    //  slot N -- it does not rotate. Call this AFTER the physical switch, with
    //  the rotation index that was promoted.
    //
    //  This replaces the old on_swap(from, to), which assumed the outgoing
    //  Pokemon was already in slot 1. That held in the per-battle path (which
    //  normalizes the winner into slot 1 first) but not in the PP-exhaustion
    //  path: after a mid-battle forced switch the active Pokemon sits in its
    //  own slot, so on_swap recorded a swap the game never performed and the
    //  rotation->slot table stayed wrong for the rest of the run.
    void on_promote_to_lead(int rotation_promoted){
        if (rotation_promoted < 0 || rotation_promoted >= party_size) return;
        int target = game_slot[rotation_promoted];
        if (target == 1) return;
        int rotation_in_slot1 = find_rotation_with_game_slot(1);
        game_slot[rotation_promoted] = 1;
        if (rotation_in_slot1 >= 0){
            game_slot[rotation_in_slot1] = target;
        }
    }

    //  Which Pokemon the game will send out for the next encounter: the
    //  lowest-numbered game slot that has not fainted. Gen 3 always leads with
    //  the first healthy party member, regardless of who was last on the field.
    int rotation_game_will_lead() const{
        for (int slot = 1; slot <= party_size; slot++){
            int rot = find_rotation_with_game_slot(slot);
            if (rot >= 0 && !fainted[rot]) return rot;
        }
        return -1;
    }

    //  Called after a heal trip or whiteout. Clears only the per-trip health
    //  flags.
    //
    //  Deliberately PRESERVES game_slot[] and current_moves[]:
    //    - Healing does not reorder the party, so the permutation built up by
    //      earlier switch_party_lead_overworld calls is still accurate. The old
    //      `party = PartyState(party_size)` reset it to the identity, which
    //      silently re-pointed every rotation index at the wrong Pokemon and
    //      corrupted the team table for the rest of the run.
    //    - current_moves[] cost a full party scan to build. Dropping it sent
    //      MoveLearnDecider down its no-cache path, which declines every move
    //      offered and collapses battle_move_priority to slot 1 only -- so one
    //      heal trip permanently disabled the smart move handling and could
    //      spam a status move until the 50-turn fatal guard tripped.
    void on_healed(){
        for (int i = 0; i < 6; i++){
            fainted[i] = false;
            out_of_pp[i] = false;
        }
        int lead = find_rotation_with_game_slot(1);
        if (lead >= 0){
            current_rotation = lead;
        }
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

    //  Switch training depends on a party order that never moves: the trainee
    //  stays in slot 1 so the game always leads with it, and the fighter is
    //  pulled in with an in-battle switch, which does not reorder the party.
    //
    //  The post-battle "normalize the winner into slot 1" step exists for the
    //  forced-switch modes, where the winner really is stranded in its original
    //  slot. Under switch training it is actively destructive: it physically
    //  swapped the fighter into slot 1 after every battle, so the trainee walked
    //  down the party one slot at a time and FIGHTER_SLOT ended up pointing at a
    //  fainted Pokemon. Observed 8/19: normalize at 13:05:00 and 13:07:06, then
    //  the 13:07:30 switch hung on a KO'd slot 2.
    const bool normalize_party_after_battle =
        multi_party && ROTATION_MODE != RotationMode::switch_training;

    env.log(
        "Config: MAX_BATTLES=" + std::to_string((uint64_t)MAX_BATTLES) +
        "; PREVENT_EVOLUTION=" + bool_str(PREVENT_EVOLUTION) +
        "; IGNORE_SHINIES=" + bool_str(IGNORE_SHINIES) +
        "; GRIND_LOCATION=" + GrindLocationId_Database().find(grind_location)->display +
        "; HEAL_LOCATION=" + HealLocationId_Database().find(heal_location)->display +
        (AUTO_HEAL_LOCATION ? " (auto)" : " (manual)") +
        "; ROTATION_MODE=" + std::to_string((int)(RotationMode)ROTATION_MODE) +
        (ROTATION_MODE == RotationMode::switch_training
            ? "; FIGHTER_SLOT=" + std::to_string((uint64_t)FIGHTER_SLOT)
            : std::string()) +
        "; PARTY_SIZE=" + std::to_string(party_size) +
        "; HEAL_ON_FAINT=" + bool_str(HEAL_ON_FAINT) +
        "; HEAL_ON_OUT_OF_PP=" + bool_str(HEAL_ON_OUT_OF_PP) +
        "; BATTLES_PER_HEAL_TRIP=" + std::to_string((uint64_t)BATTLES_PER_HEAL_TRIP) +
        "; TRAVEL_METHOD=" + travel_method_string(TRAVEL_METHOD) + ".",
        COLOR_BLUE
    );

    //  Switch training needs a fighter that actually exists. Catching this at
    //  startup is the difference between one clear message and a run that spends
    //  the night failing a party-screen navigation once per battle.
    if (ROTATION_MODE == RotationMode::switch_training &&
        (int)(uint64_t)FIGHTER_SLOT > party_size
    ){
        OperationFailedException::fire(
            ErrorReport::NO_ERROR_REPORT,
            "Switch Training is set to fight with slot " +
                std::to_string((uint64_t)FIGHTER_SLOT) + ", but the party only has " +
                std::to_string(party_size) + " Pokemon. Lower the Fighter Slot, or add "
                "Pokemon to the party.",
            env.console
        );
    }

    bool spin_leftright = true;
    uint8_t failed_encounters = 0;
    bool stop_program = false;
    PartyState party(party_size);

    //  Consecutive recoverable failures. Every helper this program calls already
    //  retries internally, so one escaping OperationFailedException means a single
    //  operation genuinely failed -- previously that ended the whole run, which
    //  meant one missed menu at 3am cost the entire overnight grind.
    int consecutive_errors = 0;
    constexpr int MAX_CONSECUTIVE_ERRORS = 4;

    if (startup_scan_ok){
        //  Rotation index = scan order (slot_1indexed - 1) at startup.
        for (const PartyScanResult& r : startup_scan){
            int rot = r.slot_1indexed - 1;
            if (rot >= 0 && rot < 6){
                party.current_moves[rot] = r.read.move_slugs;
                party.level[rot] = r.read.level;

                //  A conflicted or unidentified read leaves species_slug empty on
                //  purpose, so the block below won't touch the table row. Say so
                //  out loud: the row keeps whatever species it already had, and
                //  everything downstream -- the chain learnset, the auto-filled
                //  moves, the evolution hold -- is only as right as that value.
                if (r.species_confidence == SpeciesConfidence::Conflicted ||
                    r.species_confidence == SpeciesConfidence::None
                ){
                    const std::string current_row = TEAM_TABLE.species_for((size_t)rot);
                    env.log(
                        "Slot " + std::to_string(r.slot_1indexed) + ": species " +
                        std::string(species_confidence_name(r.species_confidence)) +
                        ". Team table row keeps '" +
                        (current_row.empty() ? std::string("(unset)") : current_row) +
                        "'. Set it by hand if that is wrong -- move planning depends on it.",
                        COLOR_RED
                    );
                    stats.errors++;
                }
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

    //  The table now ships 6 rows, but a config saved before that change still
    //  has however many rows it had. Rows that don't exist can't be written to:
    //  set_species() returns false and the Pokemon silently gets no pinned
    //  moves, no STAB and no auto-species. Say so rather than letting it be
    //  invisible.
    if (TEAM_TABLE.row_count() < (size_t)party_size){
        env.log(
            "Team table has only " + std::to_string(TEAM_TABLE.row_count()) +
                " row(s) but the party has " + std::to_string(party_size) +
                ". Slots beyond row " + std::to_string(TEAM_TABLE.row_count()) +
                " cannot be configured -- add rows to the Team Table option, or "
                "reset this program's settings to pick up the new 6-row default.",
            COLOR_RED
        );
        stats.errors++;
    }

    //  Auto-fill any team row the user left empty, using the scanned level and
    //  moves so the suggestion only contains moves still reachable. Rows the
    //  user filled in are never touched.
    if (AUTOFILL_DESIRED_MOVES){
        std::vector<int> levels;
        std::vector<std::array<std::string, 4>> currents;
        for (int i = 0; i < party_size; i++){
            levels.push_back(party.level[i]);
            currents.push_back(party.current_moves[i]);
        }
        size_t filled = TEAM_TABLE.autofill_desired_moves(false, levels, currents);
        env.log(
            filled == 0
                ? std::string("Auto-fill: nothing to do (every row with a species already has moves chosen).")
                : "Auto-fill: populated desired moves for " + std::to_string(filled) + " row(s).",
            COLOR_BLUE
        );
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

    //  Build and report the per-Pokemon move plan. This is the "what will
    //  actually happen" summary: what each member already has, what it will
    //  learn and at what level, what is already gone for good, and whether
    //  evolution has to be held back to avoid forfeiting something.
    {
        env.log("---- Move plan ----", COLOR_BLUE);
        for (int i = 0; i < party_size; i++){
            party.plan[i] = TEAM_TABLE.build_plan(
                (size_t)i, party.level[i], party.current_moves[i]
            );
            env.log(party.plan[i].to_log_string(), COLOR_BLUE);
        }
        int team_target = -1;
        for (int i = 0; i < party_size; i++){
            team_target = std::max(team_target, party.plan[i].target_level());
        }
        if (party.all_plans_complete()){
            env.log("Move plan: every party member already has its desired moveset.", COLOR_BLUE);
        }else if (team_target >= 0){
            env.log(
                "Move plan: highest level needed across the party is Lv " +
                    std::to_string(team_target) + ".",
                COLOR_BLUE
            );
        }
        env.log("-------------------", COLOR_BLUE);
    }

    if (STOP_WHEN_TEAM_COMPLETE && !party.any_plan_has_goals()){
        env.log(
            "Stop-when-complete is on, but no party member has any desired moves configured "
            "(the scan may have failed to identify species). The completion check is disabled "
            "for this run so it does not exit claiming success.",
            COLOR_RED
        );
    }else if (STOP_WHEN_TEAM_COMPLETE && party.all_plans_complete()){
        env.log("Every party member already has its desired moveset. Nothing to do.", COLOR_BLUE);
        if (GO_HOME_WHEN_DONE){
            pbf_press_button(context, BUTTON_HOME, 200ms, 1000ms);
        }
        send_program_finished_notification(env, NOTIFICATION_PROGRAM_FINISH);
        return;
    }

    //  Start-of-run navigation.
    //
    //  Every other kanto_navigate_to() call in this program recovers a *known*
    //  situation -- after a heal trip, after a whiteout, after an error. The
    //  first encounter, though, simply assumed the player was already standing
    //  in the grass. Starting anywhere else meant grass_spin() spun on dry land
    //  until the failed-encounter guard killed the run, with nothing in the log
    //  to say why.
    if (NAVIGATE_TO_GRIND_ON_START){
        env.log("Navigating to the grind location before the first encounter.", COLOR_BLUE);
        try{
            kanto_navigate_to(env, context, goal_for_grind_location(grind_location));
        }catch (const OperationFailedException& e){
            //  Re-fire with the one piece of context the navigator cannot know:
            //  which option to turn off, and what counts as a valid start.
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "Could not walk to the grind spot at startup: " + e.message() +
                    "  Start the program somewhere in Viridian City or on Route 1, or turn off "
                    "\"Walk to the grind spot when the program starts\" and begin standing in "
                    "the grass.",
                env.console
            );
        }
    }

    int battles_since_drift_check = 0;

    while (!stop_program && (MAX_BATTLES == 0 || stats.battles_won.load() < MAX_BATTLES)){
        try{
            //  Periodic drift correction.
            //
            //  grass_spin() moves the player around to trigger encounters and
            //  nothing pulls them back, so over a long run they migrate. On
            //  2026-08-19 the character finished ~1,260 encounters nine tiles
            //  north and three east of the grind tile -- far outside the
            //  navigator's hint radius, and hard against the boundary trees where
            //  the map's border void made localization worst. Both failure modes
            //  are fixed now; the drift that walked into them is not.
            //
            //  Re-navigating to the grind goal IS the check. If we are already
            //  there, kanto_navigate_to() localizes once and returns on the first
            //  poll, so the common case costs one hinted match. Seeding the goal
            //  also makes the navigator's own "N tile(s) from the seeded hint"
            //  line report the measured drift for free -- no extra plumbing, and
            //  it turns drift into something visible in the log rather than
            //  something we only learn about when a run dies.
            //
            //  Deliberately best-effort: a routine drift check must not spend the
            //  run's recoverable-failure budget. If it fails we log and carry on;
            //  the next heal trip runs the same navigation with full handling.
            if (++battles_since_drift_check >= BATTLES_PER_DRIFT_CHECK){
                battles_since_drift_check = 0;
                const KantoGoal grind_goal = goal_for_grind_location(grind_location);
                env.log(
                    "Drift check (every " + std::to_string(BATTLES_PER_DRIFT_CHECK) +
                        " encounters): re-centring on the grind spot.",
                    COLOR_BLUE
                );
                try{
                    kanto_navigate_to(env, context, grind_goal, grind_goal);
                }catch (const OperationFailedException& e){
                    env.log(
                        std::string("Drift check could not complete (continuing anyway): ") +
                            e.message(),
                        COLOR_RED
                    );
                }
            }

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

            //  Switch training.
            //
            //  Slot 1 holds the trainee and has just been sent out, which is all
            //  Gen 3 requires for it to share the battle's EXP. Withdraw it now
            //  for something that can actually win. The switch resolves before
            //  the opponent's move, so the trainee never takes a hit -- that is
            //  what makes this safe for a level 5 Magikarp against wild Pokemon
            //  that would otherwise flatten it.
            //
            //  current_rotation follows the Pokemon that is actually out, so the
            //  move decider and PP tracking reason about the fighter rather than
            //  the trainee for the rest of the battle. In-battle switches do not
            //  reorder the party, so the next encounter starts with the trainee
            //  in front again and we repeat.
            if (ROTATION_MODE == RotationMode::switch_training){
                //  Party order is never touched in this mode, so rotation index i
                //  is always game slot i+1 and we can reason in raw slots.
                int fighter_slot = (int)(uint64_t)FIGHTER_SLOT;
                if (fighter_slot > party.party_size || party.fainted[fighter_slot - 1]){
                    //  The configured fighter is down. Fall forward to the next
                    //  healthy Pokemon that is not the trainee rather than trying
                    //  to send out a KO'd one -- that is what wedged the party
                    //  screen on 8/19.
                    const int replacement_rotation = party.next_alive_fighter();
                    const int replacement = replacement_rotation < 0
                        ? -1
                        : replacement_rotation + 1;
                    if (replacement > 0){
                        env.log(
                            "Switch training: slot " + std::to_string(fighter_slot) +
                                " is down. Fighting with slot " + std::to_string(replacement) +
                                " instead.",
                            COLOR_BLUE
                        );
                    }
                    fighter_slot = replacement;
                }

                if (fighter_slot < 2){
                    //  Every fighter is fainted and only the trainee is left. A
                    //  level 5 Magikarp cannot win this battle -- it can only lose
                    //  it -- so run and heal instead of feeding it to the grass.
                    env.log(
                        "Switch training: no healthy fighter left in the party. Fleeing and taking a heal trip.",
                        COLOR_RED
                    );
                    flee_battle(env.console, context);
                    routine_heal_trip(
                        env, context, TRAVEL_METHOD, grind_location, heal_location,
                        [&]{ party.on_healed(); }
                    );
                    stats.healing_trips++;
                    failed_encounters = 0;
                    env.update_stats();
                    continue;
                }

                try{
                    switch_pokemon_in_battle(env.console, context, fighter_slot);
                    party.current_rotation = std::min(fighter_slot - 1, party.party_size - 1);
                }catch (const OperationFailedException& e){
                    //  Fall through and fight with whoever is out. Losing the
                    //  trainee's share of one battle's EXP is a far better
                    //  outcome than ending the run.
                    env.log(
                        std::string("Switch training: could not swap in slot ") +
                            std::to_string(fighter_slot) + " (" + e.message() +
                            "). Fighting with the current lead this battle.",
                        COLOR_RED
                    );

                    //  Mark the slot unusable so the next battle falls forward
                    //  instead of retrying the same doomed switch forever.
                    //
                    //  fainted[] was previously only set by a BattleResult of
                    //  playerfainted, which misses a fighter that goes down
                    //  without the program registering it -- Struggle recoil
                    //  landing on the same turn the opponent faints is the
                    //  obvious way in. On 8/19 slot 2 stopped being switchable
                    //  after battle 21 and the program retried it for the next
                    //  hour, wedging on a menu each time. A refused send-out is
                    //  itself sufficient evidence that the slot cannot fight,
                    //  whatever the reason; on_healed() clears it again.
                    if (fighter_slot >= 1 && fighter_slot <= party.party_size){
                        party.fainted[fighter_slot - 1] = true;
                        env.log(
                            "Switch training: marking slot " + std::to_string(fighter_slot) +
                                " unusable until the next heal trip.",
                            COLOR_RED
                        );
                    }
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
                    consecutive_errors = 0;   //  A clean win means we are healthy again.
                    bool evolved = false;

                    //  Per-Pokemon evolution hold. A stone evolution replaces the
                    //  level-up learnset wholesale, so evolving before a
                    //  stage-locked move is learned loses it permanently. The plan
                    //  knows which moves those are; cancel evolution for this
                    //  Pokemon until they are banked, then let it evolve normally.
                    const MovePlan& active_plan = party.plan[party.current_rotation];
                    bool prevent_evo = !!PREVENT_EVOLUTION;
                    if (!prevent_evo && HOLD_EVOLUTION_FOR_MOVES && !active_plan.safe_to_evolve()){
                        prevent_evo = true;
                        std::vector<std::string> blocking = active_plan.evolution_blocking_moves();
                        std::string list;
                        for (size_t b = 0; b < blocking.size(); b++){
                            if (b != 0){
                                list += ", ";
                            }
                            list += blocking[b];
                        }
                        env.log(
                            "Holding evolution for rotation " +
                                std::to_string(party.current_rotation) +
                                ": would forfeit " + list + ".",
                            COLOR_BLUE
                        );
                    }

                    WildBattleExit exit_result = exit_wild_battle(
                        env.console, context, false, prevent_evo,
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

                    //  Confirm the overworld is up before any subsequent menu
                    //  navigation: pressing START while the screen is still
                    //  mid-fade drops inputs and corrupts the post-battle
                    //  switch_party_lead_overworld call below.
                    //
                    //  Note exit_wild_battle does NOT reliably return while the
                    //  screen is still black -- it usually returns after the fade
                    //  has finished. Assuming otherwise is what made the old
                    //  BlackScreenOverWatcher here useless.
                    {
                        if (wait_for_overworld_after_battle(env, context)){
                            env.log("Overworld visible after battle exit.", COLOR_BLUE);
                        }else{
                            //  Now a real signal rather than the every-battle noise
                            //  the old BlackScreenOverWatcher produced: the screen
                            //  genuinely never went clear. Something is still on it.
                            env.log(
                                "Overworld still not visible 5s after battle exit -- the screen "
                                "never went clear. Proceeding anyway.",
                                COLOR_RED
                            );
                            stats.errors++;
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
                        //  Scan the PHYSICAL slot the active Pokemon occupies, not
                        //  the rotation index. After any lead switch the two differ,
                        //  and using the rotation index scanned a different party
                        //  member -- writing its moves into this one's cache and
                        //  overwriting the wrong team-table row.
                        const int active_slot = party.game_slot[party.current_rotation];
                        env.log(
                            std::string(evolved ? "Evolution" : "Move learn") +
                            " occurred. Rescanning game slot " + std::to_string(active_slot) +
                            " (rotation " + std::to_string(party.current_rotation) +
                            ") to refresh species and move cache.",
                            COLOR_BLUE
                        );
                        try{
                            PartyScanResult r = scan_party_slot(
                                env, context, LANGUAGE, active_slot
                            );
                            party.current_moves[party.current_rotation] = r.read.move_slugs;
                            party.level[party.current_rotation] = r.read.level;
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

                        //  Rebuild this slot's plan from the refreshed species,
                        //  level and moves. A rescan happens on exactly the events
                        //  that can change the plan -- a move learned, or an
                        //  evolution swapping in a different learnset -- so this is
                        //  where the evolution hold and the completion check get
                        //  their new answer.
                        const size_t rot = (size_t)party.current_rotation;
                        MovePlan before = party.plan[rot];
                        party.plan[rot] = TEAM_TABLE.build_plan(
                            rot, party.level[rot], party.current_moves[rot]
                        );
                        if (!before.complete() && party.plan[rot].complete()){
                            env.log(
                                "Rotation " + std::to_string(party.current_rotation) +
                                    " now has its full desired moveset.",
                                COLOR_BLUE
                            );
                        }else{
                            env.log(party.plan[rot].to_log_string(), COLOR_BLUE);
                        }
                    }

                    //  Whole-team completion check.
                    if (STOP_WHEN_TEAM_COMPLETE && party.any_plan_has_goals() && party.all_plans_complete()){
                        env.log("Every party member now has its desired moveset. Stopping.", COLOR_BLUE);
                        send_program_notification(
                            env,
                            NOTIFICATION_STATUS_UPDATE,
                            COLOR_BLUE,
                            "Team movesets complete.",
                            {}, "",
                            env.console.video().snapshot(),
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
                            routine_heal_trip(
                                env, context, TRAVEL_METHOD, grind_location, heal_location,
                                [&]{ party.on_healed(); }
                            );
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
                    if (normalize_party_after_battle && party.game_slot[party.current_rotation] != 1){
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
                        party.on_promote_to_lead(party.current_rotation);
                    }

                    //  Per-battle rotation: cycle the lead to the next alive party member.
                    if (multi_party && ROTATION_MODE == RotationMode::per_battle){
                        //  Prefer a member that still needs moves, so a finished
                        //  Pokemon stops soaking up experience the rest of the
                        //  party needs.
                        int next = party.next_alive_preferring_unfinished();
                        if (next >= 0 && next != party.current_rotation){
                            env.log("Per-battle rotation: switching lead to rotation slot " + std::to_string(next) + " (game slot " + std::to_string(party.game_slot[next]) + ").", COLOR_BLUE);
                            switch_party_lead_overworld(env.console, context, party.game_slot[next]);
                            party.on_promote_to_lead(next);
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

                    //  Switch training: the last fighter just went down and the
                    //  only Pokemon left is the trainee. The game does not offer
                    //  a choice here -- something has to come out -- so send the
                    //  trainee, then run before it takes a turn, and heal.
                    //
                    //  Without this, next_alive() wraps around to rotation 0 and
                    //  hands a level 5 Magikarp a battle it cannot win, and the
                    //  program keeps doing that until the whiteout.
                    if (ROTATION_MODE == RotationMode::switch_training &&
                        multi_party && party.only_trainee_left() && !party.fainted[0]
                    ){
                        env.log(
                            "Switch training: last fighter is down and only the trainee is left. "
                            "Sending it out, fleeing, then healing.",
                            COLOR_RED
                        );
                        select_forced_switch_slot(env.console, context, party.game_slot[0]);
                        party.current_rotation = 0;
                        try{
                            flee_battle(env.console, context);
                        }catch (const OperationFailedException& e){
                            //  Escape can fail; the trainee is now exposed, so let
                            //  the outer handler recover rather than pressing on.
                            env.log(
                                std::string("Switch training: could not flee with the trainee out: ") +
                                    e.message(),
                                COLOR_RED
                            );
                            throw;
                        }
                        routine_heal_trip(
                            env, context, TRAVEL_METHOD, grind_location, heal_location,
                            [&]{ party.on_healed(); }
                        );
                        stats.healing_trips++;
                        failed_encounters = 0;
                        env.update_stats();
                        battle_ongoing = false;
                        break;
                    }

                    int next_after_faint = multi_party && !party.all_fainted()
                        ? (ROTATION_MODE == RotationMode::switch_training
                            ? party.next_alive_fighter()
                            : party.next_alive())
                        : -1;
                    if (next_after_faint >= 0){
                        //  Alive allies remain — use the forced-switch screen.
                        //  next_alive() is checked for -1 above rather than being
                        //  used to index game_slot[] directly: it starts scanning at
                        //  offset 1, so it can return -1 even when the current
                        //  rotation is alive, and an unchecked -1 here is an
                        //  out-of-bounds read.
                        const int next = next_after_faint;
                        env.log("Party member fainted. Forced-switch to rotation slot " + std::to_string(next) + " (game slot " + std::to_string(party.game_slot[next]) + ").", COLOR_BLUE);
                        select_forced_switch_slot(env.console, context, party.game_slot[next]);
                        party.current_rotation = next;
                        //  Continue battle_ongoing = true so spam_first_move is called again.
                    }else{
                        //  All fainted (whiteout) or single-Pokémon mode.
                        //  HEAL_ON_FAINT is honoured in both modes: the option
                        //  promises "accept the whiteout and resume instead of
                        //  stopping", and the old `|| multi_party` made it
                        //  impossible to turn off with rotation enabled.
                        if (HEAL_ON_FAINT){
                            env.log("All party fainted — whiteout. Resuming via PC.", COLOR_BLUE);
                            whiteout_resume(
                                env, context, grind_location,
                                [&]{ party.on_healed(); }
                            );
                            stats.healing_trips++;
                            failed_encounters = 0;
                            env.update_stats();
                        }else{
                            env.log("Party wiped. HEAL_ON_FAINT is off, stopping program.", COLOR_RED);
                            //  Ride out the whiteout before stopping. The warp
                            //  happens regardless of input, so simply mashing B for
                            //  a few seconds used to leave the game parked on the
                            //  "you scurried back" prompt -- and GO_HOME_WHEN_DONE
                            //  then pressed HOME on top of it.
                            whiteout_settle(env, context);
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

                    const int next_with_pp =
                        (multi_party && ROTATION_MODE == RotationMode::pp_exhaustion && !party.all_need_heal(true))
                            ? party.next_alive(/*skip_pp_exhausted=*/true)
                            : -1;
                    if (next_with_pp >= 0){
                        //  PP exhausted but alive allies with PP remain — rotate overworld.
                        //  Guarded against -1 for the same reason as the faint path:
                        //  next_alive() skips the current rotation, so it can return
                        //  -1, and game_slot[-1] is an out-of-bounds read.
                        const int next = next_with_pp;
                        env.log("PP exhaustion rotation: switching lead to rotation slot " + std::to_string(next) + " (game slot " + std::to_string(party.game_slot[next]) + ").", COLOR_BLUE);
                        switch_party_lead_overworld(env.console, context, party.game_slot[next]);
                        party.on_promote_to_lead(next);
                        party.current_rotation = next;
                    }else if (HEAL_ON_OUT_OF_PP || (multi_party && party.all_need_heal(true))){
                        env.log("Trigger: move 1 out of PP. Taking routine heal trip.", COLOR_BLUE);
                        routine_heal_trip(
                            env, context, TRAVEL_METHOD, grind_location, heal_location,
                            [&]{ party.on_healed(); }
                        );
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

                case BattleResult::unknown:{
                    //  Usually the wild Pokemon fled. Nothing normalizes the party
                    //  here, so if a mid-battle forced switch happened earlier in
                    //  this battle the active Pokemon is NOT in slot 1 -- and the
                    //  next encounter will start with whoever the game leads with,
                    //  not whoever was last on the field. Re-derive rather than
                    //  leaving current_rotation pointing at the wrong Pokemon.
                    env.log("Battle ended without a faint (wild Pokemon likely fled). Continuing.");
                    if (multi_party){
                        int lead = party.rotation_game_will_lead();
                        if (lead >= 0 && lead != party.current_rotation){
                            env.log(
                                "Re-deriving active Pokemon: rotation " +
                                std::to_string(party.current_rotation) + " -> " +
                                std::to_string(lead) + " (game slot " +
                                std::to_string(party.game_slot[lead]) + ").",
                                COLOR_BLUE
                            );
                            party.current_rotation = lead;
                        }
                    }
                    env.update_stats();
                    battle_ongoing = false;
                    break;
                }
                }
            }

        }catch (OperationFailedException& e){
            stats.errors++;
            consecutive_errors++;
            env.update_stats();
            env.log(
                "Recoverable failure (" + std::to_string(consecutive_errors) + "/" +
                std::to_string(MAX_CONSECUTIVE_ERRORS) + "): " + e.message(),
                COLOR_RED
            );
            if (consecutive_errors >= MAX_CONSECUTIVE_ERRORS){
                env.log("Too many consecutive failures. Aborting.", COLOR_RED);
                throw;
            }

            //  Back out of whatever screen we are stuck on and let the game settle.
            pbf_mash_button(context, BUTTON_B, 3000ms);
            context.wait_for_all_requests();
            pbf_wait(context, 1000ms);
            context.wait_for_all_requests();

            //  The failure may have happened mid-heal-trip, so we could be anywhere
            //  on the map. Walk back to the grind spot before resuming. This is a
            //  no-op (single goal check) when we are already there.
            try{
                kanto_navigate_to(env, context, goal_for_grind_location(grind_location));
            }catch (OperationFailedException& nav){
                env.log(
                    std::string("Could not re-navigate to the grind spot after recovery: ") +
                    nav.message() + ". Will retry.",
                    COLOR_RED
                );
            }

            //  We no longer know who is on the field after an aborted operation.
            if (multi_party){
                int lead = party.rotation_game_will_lead();
                if (lead >= 0){
                    party.current_rotation = lead;
                }
            }
            failed_encounters = 0;
            continue;
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
