/*  Pokemon FRLG Move Plan
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Pure logic, no I/O. Answers the question the XP Grinder actually needs:
 *  "given what this Pokemon is, what level it is, what it knows, and what the
 *  user wants it to end up with -- what happens next?"
 *
 *  Three things fall out of that, and the grinder uses all three:
 *
 *    1. Which desired moves are still reachable by level-up, and at what level.
 *       Gives a concrete target level to grind to, and honestly reports the
 *       ones that are already gone (Gen 3 can only relearn those via the Move
 *       Reminder, which this program cannot drive).
 *
 *    2. Whether it is safe to let this Pokemon evolve yet. In Gen 3 the
 *       evolved form has its OWN level-up learnset, and for stone evolutions
 *       it is drastically smaller -- Growlithe loses Flamethrower, Flame Wheel,
 *       Agility and Take Down the instant it becomes Arcanine; Pikachu loses
 *       Thunder, Agility, Slam and Thunder Wave on becoming Raichu. Evolving
 *       before those levels forfeits the move permanently. This is derived
 *       purely from the learnset difference, so no evolution-method data is
 *       needed.
 *
 *    3. Whether this Pokemon is done, so the grinder can move on to one that
 *       is not.
 *
 *  Known limitation: for a fan-shaped chain (Eevee), "a later stage" means any
 *  eeveelution, so a move only one of them learns is not reported as
 *  stage-locked even though picking a different evolution would lose it.
 *  Evolutions.json also lists post-Gen-3 eeveelutions; those have no FRLG
 *  learnset so they contribute nothing.
 *
 */

#ifndef PokemonAutomation_PokemonFRLG_MovePlan_H
#define PokemonAutomation_PokemonFRLG_MovePlan_H

#include <array>
#include <string>
#include <vector>

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


enum class MoveGoalStatus{
    Known,                   //  Already in the current moveset.
    Upcoming,                //  This species learns it at a level above the current one.
    UpcomingAfterEvolution,  //  Only a later chain stage learns it.
    Missed,                  //  This species learns it at or below the current level,
                             //  and it is not known -- gone, barring the Move Reminder.
    NotLearnable,            //  No species in the chain learns it by level-up
                             //  (a TM/HM/egg move, or simply a bad pick).
};

const char* move_goal_status_name(MoveGoalStatus status);


struct MoveGoal{
    std::string move_slug;
    MoveGoalStatus status = MoveGoalStatus::NotLearnable;
    //  Level at which `learned_as` offers it. 0 when NotLearnable.
    int level = 0;
    //  Which chain stage teaches it. Empty when NotLearnable.
    std::string learned_as;
    //  True if the CURRENT species teaches it and no later stage does, i.e.
    //  evolving forfeits it. Only meaningful while the goal is not yet Known.
    bool stage_locked = false;
};


class MovePlan{
public:
    std::string species_slug;
    int current_level = -1;          //  -1 when the level could not be read.
    std::vector<MoveGoal> goals;     //  In the user's priority order.

    //  Every goal the Pokemon can actually get is already known. Goals that are
    //  NotLearnable or Missed are excluded -- they can never be satisfied, so
    //  waiting on them would mean grinding forever.
    //
    //  NOTE: a plan with no goals at all is trivially complete. Callers deciding
    //  whether to STOP on completion must also check has_goals(), or a party
    //  with nothing configured reads as "already done".
    bool complete() const;

    //  True if the user actually asked for anything on this Pokemon.
    bool has_goals() const{ return !goals.empty(); }

    //  No still-wanted move would be forfeited by evolving right now.
    bool safe_to_evolve() const;

    //  The moves that make safe_to_evolve() false, for logging and for the
    //  grinder's per-Pokemon evolution hold.
    std::vector<std::string> evolution_blocking_moves() const;

    //  Highest level among goals still reachable by level-up, i.e. the level
    //  this Pokemon must reach. -1 when nothing is left to learn.
    int target_level() const;

    //  Human-readable multi-line summary for the program log.
    std::string to_log_string() const;
};


//  Build the plan. `current_level` may be -1 if unknown, in which case nothing
//  is classified as Missed (we cannot know what has already gone by, and
//  wrongly reporting a move as lost is worse than being optimistic).
MovePlan build_move_plan(
    const std::string& species_slug,
    int current_level,
    const std::array<std::string, 4>& current_moves,
    const std::array<std::string, 4>& desired_moves
);


//  True if `species_slug` learns `move_slug` by level-up and no later stage in
//  its evolution chain does.
bool move_is_stage_locked(const std::string& species_slug, const std::string& move_slug);


//  Suggest a desired-move set for a species: the four strongest level-up moves
//  in its evolution chain, biased toward type coverage (one per type first,
//  then filled by score) and returned strongest-first, because the grinder
//  treats slot 0 as highest battle priority.
//
//  Scoring is STAB-adjusted base power. Excluded outright: moves that KO the
//  user (Explosion, Self-Destruct). Halved: two-turn and recharge moves, which
//  are a poor default for a grinder one-shotting low-level wilds -- they can
//  still win a slot if nothing better exists. The user can pin any of these by
//  hand; this only affects the automatic suggestion.
//
//  `from_level` filters to moves still reachable from that level (pass 1 or -1
//  for no filter). Moves already in `current_moves` are always eligible even if
//  their level has passed.
//
//  Slots with no good candidate are returned as empty strings, which the team
//  table renders as "(none)" -- an honest "no preference" rather than a guess.
std::array<std::string, 4> suggest_desired_moves(
    const std::string& species_slug,
    int from_level = -1,
    const std::array<std::string, 4>& current_moves = {}
);


}
}
}
#endif
