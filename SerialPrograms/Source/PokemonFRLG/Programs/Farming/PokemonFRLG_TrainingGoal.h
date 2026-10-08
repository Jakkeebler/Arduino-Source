/*  Pokemon FRLG Training Goal
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Named per-Pokemon training goal (FRO-235, follow-up to FRO-227 Phase 4).
 *  Today the XP Grinder's 4 stop conditions work via a combination of
 *  independent settings -- MAX_BATTLES == 0, STOP_WHEN_TEAM_COMPLETE +
 *  MovePlan::complete(), EvolutionPolicyType per species, and
 *  MovePlan::target_level() -- rather than one named per-row selector. This
 *  gives the team table an explicit dropdown so a user picks one goal per
 *  row instead of inferring it, and a pure (no I/O) helper the grinder and
 *  tests can both call to ask "is this row done?"
 *
 */

#ifndef PokemonAutomation_PokemonFRLG_TrainingGoal_H
#define PokemonAutomation_PokemonFRLG_TrainingGoal_H

#include "Common/Cpp/Options/EnumDropdownDatabase.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

class MovePlan;


//  What "done" means for this party slot.
enum class TrainingGoalType{
    //  Grind until this Pokemon reaches MovePlan::target_level() -- the
    //  highest level among its still-reachable desired moves.
    TargetLevel,
    //  Grind until MovePlan::complete() is true for this row: every
    //  reachable desired move has been learned. (Goals already missed or
    //  not learnable by level-up are excluded, so an impossible pick can
    //  never make this row un-satisfiable.)
    TargetMoveSet,
    //  Grind until this Pokemon evolves, then treat it as done. Pairs
    //  naturally with EvolutionPolicyType::LevelUp or ::Stone on the same
    //  row.
    EvolveThenStop,
    //  No per-Pokemon stop condition -- this row never signals "done" on
    //  its own. Matches today's default behavior (MAX_BATTLES == 0, no
    //  STOP_WHEN_TEAM_COMPLETE).
    Indefinite,
};
const EnumDropdownDatabase<TrainingGoalType>& TrainingGoalType_Database();


//  Pure, unit-testable: has this row's training goal been satisfied given
//  its live state? No I/O, no data tables beyond the caller-supplied
//  MovePlan.
//
//  `plan`: this row's current MovePlan (PokemonFRLG_MovePlan.h).
//  `current_level`: the Pokemon's level right now.
//  `evolved_this_run`: true once this Pokemon has evolved at least once
//  since the grinder started. Only meaningful for EvolveThenStop.
bool training_goal_satisfied(
    TrainingGoalType goal,
    const MovePlan& plan,
    int current_level,
    bool evolved_this_run
);


}
}
}
#endif
