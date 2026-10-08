/*  Pokemon FRLG Training Goal
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include "PokemonFRLG_MovePlan.h"
#include "PokemonFRLG_TrainingGoal.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


const EnumDropdownDatabase<TrainingGoalType>& TrainingGoalType_Database(){
    static const EnumDropdownDatabase<TrainingGoalType> database({
        {TrainingGoalType::TargetLevel,    "target-level",     "Target Level (grind to the MovePlan's target level)"},
        {TrainingGoalType::TargetMoveSet,  "target-move-set",  "Target Move Set (grind until desired moves are learned)"},
        {TrainingGoalType::EvolveThenStop, "evolve-then-stop", "Evolve, Then Stop"},
        {TrainingGoalType::Indefinite,     "indefinite",       "Indefinite (no per-Pokemon stop condition)"},
    });
    return database;
}


bool training_goal_satisfied(
    TrainingGoalType goal,
    const MovePlan& plan,
    int current_level,
    bool evolved_this_run
){
    switch (goal){
    case TrainingGoalType::TargetLevel:{
        int target = plan.target_level();
        //  Nothing reachable left to grind toward: this row can't
        //  contribute any more progress to a level target, so treat it as
        //  satisfied rather than stalling forever.
        if (target < 0){
            return true;
        }
        return current_level >= target;
    }
    case TrainingGoalType::TargetMoveSet:
        //  MovePlan::complete() docs a plan with no goals at all as
        //  trivially complete; require has_goals() too so an unconfigured
        //  row never reads as "already done".
        return plan.has_goals() && plan.complete();
    case TrainingGoalType::EvolveThenStop:
        return evolved_this_run;
    case TrainingGoalType::Indefinite:
    default:
        return false;
    }
}


}
}
}
