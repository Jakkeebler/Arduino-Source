/*  Pokemon FRLG AutoStory Tools
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Reusable navigation primitives, stats tracking, and shared options for
 *  the FRLG AutoStory program. Segments built on top of this library should
 *  use these wrappers instead of calling the lower-level navigation /
 *  menu functions directly, so behavior (logging, stats, notifications,
 *  heal triggers) stays consistent across the whole story run.
 */

#ifndef PokemonAutomation_PokemonFRLG_AutoStoryTools_H
#define PokemonAutomation_PokemonFRLG_AutoStoryTools_H

#include <cstdint>
#include <string>
#include "CommonFramework/ProgramStats/StatsTracking.h"
#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "PokemonFRLG/PokemonFRLG_Navigation.h"
#include "PokemonFRLG/Programs/PokemonFRLG_KantoMapNavigator.h"

namespace PokemonAutomation{
    class EventNotificationOption;
namespace NintendoSwitch{
namespace PokemonFRLG{


//  Starter chosen at the Pallet Town lab. Drives which segments/dialogs the
//  AutoStory needs to navigate (move names, box art, etc.).
enum class StarterChoice{
    Bulbasaur,
    Charmander,
    Squirtle,
};

//  Whether a run starts from scratch or resumes partway through the story.
//  START_FROM_CHECKPOINT expects the caller to have already restored state
//  (save file, party, inventory) to match the checkpoint being resumed from.
enum class CheckpointMode{
    START_FROM_BEGINNING,
    START_FROM_CHECKPOINT,
};

//  Which building-entry/exit routine to use. Add new enumerators here (and
//  in enter_building()/exit_building()) as more building types get their own
//  navigation helpers in PokemonFRLG_Navigation.h.
enum class BuildingType{
    POKECENTER,
    GYM,
};


//  Stats tracked across the whole AutoStory run. Shared by every segment so
//  totals accumulate instead of resetting per-segment.
struct AutoStoryStats : public StatsTracker{
    AutoStoryStats()
        : m_battles_won(m_stats["Battles Won"])
        , m_steps_walked(m_stats["Steps Walked"])
        , m_phases_complete(m_stats["Phases Complete"])
        , m_healing_trips(m_stats["Healing Trips"])
        , m_resets(m_stats["Resets"])
        , m_errors(m_stats["Errors"])
    {
        m_display_order.emplace_back("Battles Won");
        m_display_order.emplace_back("Steps Walked");
        m_display_order.emplace_back("Phases Complete");
        m_display_order.emplace_back("Healing Trips", HIDDEN_IF_ZERO);
        m_display_order.emplace_back("Resets", HIDDEN_IF_ZERO);
        m_display_order.emplace_back("Errors", HIDDEN_IF_ZERO);
    }
    std::atomic<uint64_t>& m_battles_won;
    std::atomic<uint64_t>& m_steps_walked;
    std::atomic<uint64_t>& m_phases_complete;
    std::atomic<uint64_t>& m_healing_trips;
    std::atomic<uint64_t>& m_resets;
    std::atomic<uint64_t>& m_errors;
};


//  Options shared across every AutoStory segment. The owning program builds
//  one of these from its GUI options (and NOTIFICATIONS table) once at
//  startup, then passes it down to every segment by const reference.
struct AutoStoryOptions{
    StarterChoice starter_choice = StarterChoice::Charmander;
    CheckpointMode checkpoint_mode = CheckpointMode::START_FROM_BEGINNING;

    //  Heal automatically once this many battles have been won since the
    //  last heal trip. 0 disables automatic healing entirely (manual/segment
    //  -driven healing only).
    uint16_t auto_heal_threshold = 10;

    EventNotificationOption& notif_status_update;
};


//  ---- Navigation primitives ----

//  High-level wrapper around kanto_navigate_to(). Resolves which overload to
//  call based on whether a start hint is available, and tracks steps/phases
//  on `stats`. Throws OperationFailedException on navigation failure (same
//  as kanto_navigate_to()).
void navigate_to(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    AutoStoryStats& stats,
    const KantoGoal& goal,
    const KantoGoal* start_hint = nullptr,
    int max_steps = 200
);

//  Enter a building. Assumes navigate_to() has already placed the player on
//  the building's entrance goal tile (i.e. standing in front of the door).
void enter_building(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    BuildingType building
);

//  Leave a building. Assumes the player is standing where the building's
//  specific exit routine expects (e.g. leave_pokecenter() expects the player
//  directly north of the Center's exit).
void exit_building(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    BuildingType building
);

//  Fly to `destination`. Assumes Fly has already been unlocked (available
//  after Badge 6 / the Cinnabar Island gym in FRLG) and the last party
//  member has it learned. Wraps open_fly_map_from_overworld() +
//  fly_from_kanto_map().
void fly_to(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    KantoFlyLocation destination
);


//  ---- Health management ----

//  Heal the party if `battles_since_last_heal` has reached
//  `options.auto_heal_threshold`. No-op (returns false) if the threshold is
//  0 (disabled) or hasn't been reached yet.
//
//  On trigger: navigates to `heal_pc_entrance` (seeded with `start_hint` if
//  given), enters the Center, heals, and leaves it. Does NOT navigate back
//  to wherever the caller was before healing -- that is the caller's job,
//  using the Center's exit tile as the new start_hint for its own
//  navigate_to() call.
//
//  Returns true if a heal trip was performed (caller should reset its own
//  battles-since-heal counter), false otherwise.
bool ensure_healed(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats,
    uint16_t battles_since_last_heal,
    const KantoGoal& heal_pc_entrance,
    const KantoGoal* start_hint = nullptr
);


//  ---- Stats tracking helpers ----

//  Call after a battle is won (wild or trainer).
void track_battle_won(AutoStoryStats& stats);

//  Call after a story phase/segment finishes. `phase_name` is only used for
//  the status-update notification; the counter itself is unconditional.
void track_phase_complete(
    SingleSwitchProgramEnvironment& env,
    const AutoStoryOptions& options,
    AutoStoryStats& stats,
    const std::string& phase_name
);


}
}
}
#endif
