/*  Pokemon FRLG Move Plan
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include <algorithm>
#include <set>
#include <utility>
#include "PokemonFRLG/Resources/PokemonFRLG_Evolutions.h"
#include "PokemonFRLG/Resources/PokemonFRLG_Learnsets.h"
#include "PokemonFRLG/Resources/PokemonFRLG_MoveData.h"
#include "PokemonFRLG/Resources/PokemonFRLG_SpeciesData.h"
#include "PokemonFRLG_MovePlan.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


const char* move_goal_status_name(MoveGoalStatus status){
    switch (status){
    case MoveGoalStatus::Known:                  return "known";
    case MoveGoalStatus::Upcoming:               return "upcoming";
    case MoveGoalStatus::UpcomingAfterEvolution: return "upcoming after evolution";
    case MoveGoalStatus::Missed:                 return "missed";
    case MoveGoalStatus::NotLearnable:           return "not learnable by level-up";
    }
    return "?";
}


namespace{

//  Never auto-pick a move that KOs the user. The user can still pin these.
const std::set<std::string>& self_ko_moves(){
    static const std::set<std::string> s{"explosion", "self-destruct"};
    return s;
}
//  Two-turn / recharge moves. Fine in general play, poor as a grinder default.
const std::set<std::string>& slow_moves(){
    static const std::set<std::string> s{
        "solar-beam", "hyper-beam", "sky-attack", "razor-wind",
        "skull-bash", "bide", "fly", "dig",
    };
    return s;
}

const std::vector<std::string>& species_types(const std::string& species_slug){
    static const std::vector<std::string> none;
    const SpeciesData* sp = get_species_nothrow(species_slug);
    return sp != nullptr ? sp->types : none;
}

//  STAB-adjusted base power, in tenths so the 1.5x stays exact in integer math.
//  Returns 0 for anything not rankable by raw damage: status moves, unknown
//  slugs, and moves with no fixed base power. 0 means "do not rank me", NOT
//  "worthless".
int damage_score(
    const std::string& move_slug,
    const std::vector<std::string>& types,
    bool for_autofill
){
    if (move_slug.empty()){
        return 0;
    }
    const MoveData* mv = get_move_nothrow(move_slug);
    if (mv == nullptr || mv->category == "status" || mv->power == 0){
        return 0;
    }
    if (for_autofill && self_ko_moves().count(move_slug) != 0){
        return 0;
    }
    int score = (int)mv->power * 10;
    for (const std::string& t : types){
        if (t == mv->type){
            score = score * 3 / 2;
            break;
        }
    }
    if (for_autofill && slow_moves().count(move_slug) != 0){
        score /= 2;
    }
    return score;
}

//  Level at which `species_slug` learns `move_slug`, or -1 if it does not.
//  If a species lists a move at more than one level, the earliest wins.
int level_learned_at(const std::string& species_slug, const std::string& move_slug){
    int found = -1;
    for (const LearnsetEntry& e : learnset_for(species_slug)){
        if (e.move_slug == move_slug && (found < 0 || (int)e.level < found)){
            found = (int)e.level;
        }
    }
    return found;
}

//  Chain stages strictly after `species_slug`. Empty if it is the last stage or
//  the chain is unknown.
std::vector<std::string> later_stages(const std::string& species_slug){
    const std::vector<std::string>& chain = evolution_chain_for(species_slug);
    auto it = std::find(chain.begin(), chain.end(), species_slug);
    if (it == chain.end()){
        return {};
    }
    return std::vector<std::string>(it + 1, chain.end());
}

}  //  namespace


bool move_is_stage_locked(const std::string& species_slug, const std::string& move_slug){
    if (species_slug.empty() || move_slug.empty()){
        return false;
    }
    if (level_learned_at(species_slug, move_slug) < 0){
        return false;   //  This stage does not teach it, so evolving loses nothing.
    }
    //  A fully-evolved Pokemon has nothing to lose by "evolving". Without this
    //  the loop below never runs and every move reads as stage-locked, which made
    //  the grinder log a hold-evolution warning after every battle for a Pokemon
    //  that cannot evolve at all.
    if (later_stages(species_slug).empty()){
        return false;
    }
    for (const std::string& next : later_stages(species_slug)){
        if (level_learned_at(next, move_slug) >= 0){
            return false;
        }
    }
    return true;
}


MovePlan build_move_plan(
    const std::string& species_slug,
    int current_level,
    const std::array<std::string, 4>& current_moves,
    const std::array<std::string, 4>& desired_moves
){
    MovePlan plan;
    plan.species_slug = species_slug;
    plan.current_level = current_level;

    std::set<std::string> known;
    for (const std::string& m : current_moves){
        if (!m.empty()){
            known.insert(m);
        }
    }

    for (const std::string& want : desired_moves){
        if (want.empty()){
            continue;
        }
        MoveGoal goal;
        goal.move_slug = want;

        if (known.count(want) != 0){
            goal.status = MoveGoalStatus::Known;
            goal.learned_as = species_slug;
            int lv = level_learned_at(species_slug, want);
            goal.level = lv < 0 ? 0 : lv;
            plan.goals.push_back(std::move(goal));
            continue;
        }

        goal.stage_locked = move_is_stage_locked(species_slug, want);

        int lv = level_learned_at(species_slug, want);
        if (lv >= 0){
            goal.learned_as = species_slug;
            goal.level = lv;
            //  With an unknown level, prefer optimism: reporting a move as lost
            //  when it is not is worse than the reverse.
            goal.status = (current_level < 0 || lv > current_level)
                ? MoveGoalStatus::Upcoming
                : MoveGoalStatus::Missed;
            plan.goals.push_back(std::move(goal));
            continue;
        }

        bool found_later = false;
        for (const std::string& next : later_stages(species_slug)){
            int lv2 = level_learned_at(next, want);
            if (lv2 >= 0){
                goal.status = MoveGoalStatus::UpcomingAfterEvolution;
                goal.learned_as = next;
                goal.level = lv2;
                found_later = true;
                break;
            }
        }
        if (!found_later){
            goal.status = MoveGoalStatus::NotLearnable;
        }
        plan.goals.push_back(std::move(goal));
    }

    return plan;
}


bool MovePlan::complete() const{
    for (const MoveGoal& g : goals){
        switch (g.status){
        case MoveGoalStatus::Known:
        //  Unsatisfiable goals must not block completion, or the grinder would
        //  run forever waiting for a move it can never get.
        case MoveGoalStatus::Missed:
        case MoveGoalStatus::NotLearnable:
            break;
        case MoveGoalStatus::Upcoming:
        case MoveGoalStatus::UpcomingAfterEvolution:
            return false;
        }
    }
    return true;
}

std::vector<std::string> MovePlan::evolution_blocking_moves() const{
    std::vector<std::string> ret;
    for (const MoveGoal& g : goals){
        //  Only Upcoming can block: Known is already banked, and Missed is gone
        //  whether we evolve or not.
        if (g.stage_locked && g.status == MoveGoalStatus::Upcoming){
            ret.push_back(g.move_slug);
        }
    }
    return ret;
}

bool MovePlan::safe_to_evolve() const{
    return evolution_blocking_moves().empty();
}

int MovePlan::target_level() const{
    int target = -1;
    for (const MoveGoal& g : goals){
        if (g.status == MoveGoalStatus::Upcoming ||
            g.status == MoveGoalStatus::UpcomingAfterEvolution
        ){
            target = std::max(target, g.level);
        }
    }
    return target;
}

std::string MovePlan::to_log_string() const{
    auto display = [](const std::string& slug) -> std::string{
        const MoveData* mv = get_move_nothrow(slug);
        return mv != nullptr ? mv->display_eng : slug;
    };

    std::string out = "  " + species_slug;
    out += current_level >= 0 ? " (Lv " + std::to_string(current_level) + ")" : " (level unknown)";
    if (goals.empty()){
        return out + ": no desired moves set.";
    }
    out += ":";

    for (const MoveGoal& g : goals){
        out += "\n    " + display(g.move_slug) + " - ";
        switch (g.status){
        case MoveGoalStatus::Known:
            out += "already known";
            break;
        case MoveGoalStatus::Upcoming:
            out += "at Lv " + std::to_string(g.level);
            break;
        case MoveGoalStatus::UpcomingAfterEvolution:
            out += "at Lv " + std::to_string(g.level) + ", as " + g.learned_as;
            break;
        case MoveGoalStatus::Missed:
            out += "MISSED (learned at Lv " + std::to_string(g.level) +
                   "; only the Move Reminder can recover it)";
            break;
        case MoveGoalStatus::NotLearnable:
            out += "NOT LEARNABLE by level-up in this evolution line (TM/HM or bad pick)";
            break;
        }
    }

    int target = target_level();
    out += "\n    -> ";
    if (complete()){
        out += "complete.";
    }else if (target >= 0){
        out += "grind to Lv " + std::to_string(target) + ".";
    }else{
        out += "nothing further obtainable by level-up.";
    }

    std::vector<std::string> blocking = evolution_blocking_moves();
    if (!blocking.empty()){
        out += "\n    -> HOLDING EVOLUTION: ";
        for (size_t i = 0; i < blocking.size(); i++){
            if (i != 0){
                out += ", ";
            }
            out += display(blocking[i]);
        }
        out += " would be lost permanently if it evolved now.";
    }
    return out;
}


std::array<std::string, 4> suggest_desired_moves(
    const std::string& species_slug,
    int from_level,
    const std::array<std::string, 4>& current_moves
){
    std::array<std::string, 4> result{};
    if (species_slug.empty()){
        return result;
    }
    const std::vector<std::string>& types = species_types(species_slug);

    std::set<std::string> keep_regardless;
    for (const std::string& m : current_moves){
        if (!m.empty()){
            keep_regardless.insert(m);
        }
    }

    //  Candidate pool: every level-up move in the chain that is still reachable
    //  from `from_level`, plus anything already known.
    std::set<std::string> pool;
    for (const std::string& stage : evolution_chain_for(species_slug)){
        for (const LearnsetEntry& e : learnset_for(stage)){
            if (from_level < 0 || (int)e.level >= from_level ||
                keep_regardless.count(e.move_slug) != 0
            ){
                pool.insert(e.move_slug);
            }
        }
    }

    std::vector<std::string> ranked(pool.begin(), pool.end());
    std::sort(
        ranked.begin(), ranked.end(),
        [&](const std::string& a, const std::string& b){
            int sa = damage_score(a, types, true);
            int sb = damage_score(b, types, true);
            if (sa != sb){
                return sa > sb;
            }
            return a < b;   //  Stable, deterministic tie-break.
        }
    );

    //  Pass 1: best move of each distinct type, for coverage.
    std::vector<std::string> picked;
    std::set<std::string> types_taken;
    for (const std::string& m : ranked){
        if (picked.size() >= 4){
            break;
        }
        if (damage_score(m, types, true) == 0){
            continue;
        }
        const MoveData* mv = get_move_nothrow(m);
        if (mv == nullptr || types_taken.count(mv->type) != 0){
            continue;
        }
        types_taken.insert(mv->type);
        picked.push_back(m);
    }
    //  Pass 2: fill any remaining slots by raw score.
    for (const std::string& m : ranked){
        if (picked.size() >= 4){
            break;
        }
        if (damage_score(m, types, true) == 0){
            continue;
        }
        if (std::find(picked.begin(), picked.end(), m) != picked.end()){
            continue;
        }
        picked.push_back(m);
    }

    //  Slot 0 is the grinder's highest battle priority, so return strongest-first.
    //
    //  stable_sort with NO alphabetical tie-break: equal-scoring moves must keep
    //  the order the two passes above produced, which puts the type-coverage pick
    //  ahead of the fill pick. An alphabetical tie-break instead reorders them --
    //  e.g. Growlithe's Take Down (90 normal) and Flame Wheel (60 fire, STAB)
    //  both score 900, and sorting by name would promote the weaker-in-practice
    //  Flame Wheel to a higher battle priority.
    std::stable_sort(
        picked.begin(), picked.end(),
        [&](const std::string& a, const std::string& b){
            return damage_score(a, types, true) > damage_score(b, types, true);
        }
    );

    for (size_t i = 0; i < picked.size() && i < 4; i++){
        result[i] = picked[i];
    }
    return result;
}


}
}
}
