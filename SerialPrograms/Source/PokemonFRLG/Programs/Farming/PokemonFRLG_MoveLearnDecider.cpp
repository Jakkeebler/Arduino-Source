/*  Pokemon FRLG Move Learn Decider
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include <algorithm>
#include <climits>
#include <set>
#include <vector>
#include "PokemonSwSh/PkmnLib/PokemonSwSh_PkmnLib_Types.h"
#include "PokemonFRLG/Resources/PokemonFRLG_Evolutions.h"
#include "PokemonFRLG/Resources/PokemonFRLG_Learnsets.h"
#include "PokemonFRLG/Resources/PokemonFRLG_MoveData.h"
#include "PokemonFRLG_MoveLearnDecider.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


namespace{

namespace pl = PokemonSwSh::papkmnlib;

//  Gen-3 type count: Normal..Dark in the shared chart's enum order. Fairy
//  (index 17) did not exist yet.
constexpr int GEN3_TYPE_COUNT = 17;

//  The shared chart is the modern one. The only Gen-3 difference that matters
//  for attacking moves: Steel resisted Ghost and Dark until Gen 6.
float gen3_multiplier(pl::Type attack, pl::Type defense){
    if (defense == pl::Type::steel && (attack == pl::Type::ghost || attack == pl::Type::dark)){
        return 0.5f;
    }
    return pl::damage_multiplier(attack, defense);
}

bool super_effective(pl::Type attack, pl::Type defense){
    return gen3_multiplier(attack, defense) > 1.0f;
}

}  //  namespace


bool should_defer_for_evolution(int learn_level, int evolution_level, bool movepool_changes){
    return evolution_level > 0
        && movepool_changes
        && learn_level == evolution_level - 1;
}

int coverage_gain(
    const std::string& move_type,
    const std::vector<std::string>& other_move_types,
    const std::vector<std::string>& opponent_types
){
    const pl::Type attack = pl::get_type_from_string(move_type);
    if (attack == pl::Type::none){
        return 0;
    }

    std::vector<pl::Type> opponents;
    if (opponent_types.empty()){
        for (int i = 0; i < GEN3_TYPE_COUNT; i++){
            opponents.push_back((pl::Type)i);
        }
    }else{
        for (const std::string& name : opponent_types){
            pl::Type t = pl::get_type_from_string(name);
            if (t != pl::Type::none){
                opponents.push_back(t);
            }
        }
    }

    std::vector<pl::Type> others;
    for (const std::string& name : other_move_types){
        pl::Type t = pl::get_type_from_string(name);
        if (t != pl::Type::none){
            others.push_back(t);
        }
    }

    int gain = 0;
    for (pl::Type opponent : opponents){
        if (!super_effective(attack, opponent)){
            continue;
        }
        bool already_covered = false;
        for (pl::Type other : others){
            if (super_effective(other, opponent)){
                already_covered = true;
                break;
            }
        }
        if (!already_covered){
            gain++;
        }
    }
    return gain;
}


MoveLearnDecider::MoveLearnDecider(
    std::array<std::string, 4> desired_slugs,
    OnUnknownOffered on_unknown,
    bool auto_rank,
    std::vector<std::string> species_types,
    std::array<std::string, 4> known_current,
    MoveLearnContext context
)
    : m_desired(std::move(desired_slugs))
    , m_on_unknown(on_unknown)
    , m_auto_rank(auto_rank)
    , m_species_types(std::move(species_types))
    , m_known_current(std::move(known_current))
    , m_context(std::move(context))
{}

int MoveLearnDecider::desired_rank(const std::string& slug) const{
    if (slug.empty()){
        return INT_MAX;
    }
    for (int i = 0; i < 4; i++){
        if (!m_desired[i].empty() && m_desired[i] == slug){
            return i;
        }
    }
    return INT_MAX;
}

bool MoveLearnDecider::is_pinned(const std::string& slug) const{
    return desired_rank(slug) != INT_MAX;
}

int MoveLearnDecider::move_score(const std::string& slug) const{
    if (slug.empty()){
        return 0;
    }
    const MoveData* m = get_move_nothrow(slug);
    if (m == nullptr || m->category == "status" || m->power == 0){
        return 0;
    }
    int score = (int)m->power * 10;
    for (const std::string& t : m_species_types){
        if (t == m->type){
            score = score * 3 / 2;   //  STAB
            break;
        }
    }
    return score;
}

std::array<int, 4> MoveLearnDecider::rank_key(
    const std::string& slug, const std::vector<std::string>& others
) const{
    const MoveData* m = slug.empty() ? nullptr : get_move_nothrow(slug);
    const bool damaging = m != nullptr && m->category != "status";
    //  Power 0 on a damaging move means "no fixed base power" (Seismic Toss,
    //  OHKO, ...): not rankable by damage, so it earns no STAB / coverage either.
    const bool rankable = damaging && m->power != 0;

    int stab = 0;
    if (rankable){
        for (const std::string& t : m_species_types){
            if (t == m->type){
                stab = 1;
                break;
            }
        }
    }
    int coverage = 0;
    if (rankable){
        std::vector<std::string> other_types;
        for (const std::string& o : others){
            const MoveData* om = o.empty() ? nullptr : get_move_nothrow(o);
            if (om != nullptr && om->category != "status" && om->power != 0){
                other_types.push_back(om->type);
            }
        }
        coverage = coverage_gain(m->type, other_types, m_context.opponent_types);
    }

    std::array<int, 4> key{};
    for (size_t i = 0; i < 4; i++){
        switch (m_context.policy.order[i]){
        case ScoreCriterion::Stab:     key[i] = stab;                          break;
        case ScoreCriterion::Coverage: key[i] = coverage;                      break;
        case ScoreCriterion::Power:    key[i] = rankable ? (int)m->power : 0;  break;
        case ScoreCriterion::Damaging: key[i] = damaging ? 1 : 0;              break;
        }
    }
    return key;
}

bool MoveLearnDecider::outranks(
    const std::string& a, const std::string& b,
    const std::vector<std::string>& others
) const{
    if (a.empty()){
        return false;
    }
    if (b.empty()){
        return true;
    }
    return rank_key(a, others) > rank_key(b, others);
}

int MoveLearnDecider::weakest_forgettable(
    const std::string& new_move_slug,
    const std::array<std::string, 4>& current,
    bool* beaten
) const{
    int weakest = -1;
    std::array<int, 4> weakest_key{};
    std::vector<std::string> weakest_others;
    for (int i = 0; i < 4; i++){
        if (is_pinned(current[i])){
            continue;
        }
        //  Score each candidate by what it adds to the REST of the set, so a
        //  move that merely duplicates another's coverage is the one to lose.
        std::vector<std::string> others;
        for (int j = 0; j < 4; j++){
            if (j != i && !current[j].empty()){
                others.push_back(current[j]);
            }
        }
        //  An empty / unreadable slot is discardable and loses to anything.
        std::array<int, 4> key = current[i].empty()
            ? std::array<int, 4>{-1, -1, -1, -1}
            : rank_key(current[i], others);
        if (weakest < 0 || key < weakest_key){
            weakest = i;
            weakest_key = key;
            weakest_others = std::move(others);
        }
    }
    if (beaten != nullptr){
        *beaten = weakest >= 0 && outranks(new_move_slug, current[weakest], weakest_others);
    }
    return weakest;
}

std::string MoveLearnDecider::evolution_deferral_reason(const std::string& new_move_slug) const{
    //  A pinned move is the user's explicit choice, and declining it could lose
    //  it for good, so protection only gates auto-ranked decisions.
    if (!m_context.evolution_protection || !m_auto_rank ||
        new_move_slug.empty() || m_context.species_slug.empty() ||
        is_pinned(new_move_slug)
    ){
        return std::string();
    }
    const LevelEvolution* evolution = level_evolution_for(m_context.species_slug);
    if (evolution == nullptr){
        return std::string();
    }

    const std::vector<LearnsetEntry>& learnset = learnset_for(m_context.species_slug);
    int level = m_context.level;
    if (level < 0){
        //  A level-up move is offered at exactly the level it is learned at.
        for (const LearnsetEntry& e : learnset){
            if (e.move_slug == new_move_slug && (level < 0 || (int)e.level < level)){
                level = (int)e.level;
            }
        }
    }
    if (level < 0){
        return std::string();   //  Not a level-up move for this species; nothing to judge by.
    }

    //  Does evolving change which level-up moves are still to come?
    std::set<std::string> now;
    std::set<std::string> later;
    for (const LearnsetEntry& e : learnset){
        if ((int)e.level > level){
            now.insert(e.move_slug);
        }
    }
    for (const LearnsetEntry& e : learnset_for(evolution->evolves_to)){
        if ((int)e.level > level){
            later.insert(e.move_slug);
        }
    }
    if (!should_defer_for_evolution(level, evolution->level, now != later)){
        return std::string();
    }
    return
        m_context.species_slug + " is at Lv " + std::to_string(level) +
        ", one level from evolving into " + evolution->evolves_to + " at Lv " +
        std::to_string(evolution->level) +
        ", and its remaining level-up moves change when it does";
}

MoveLearnDecider::FirstAction MoveLearnDecider::decide_accept_or_decline(
    const std::string& new_move_slug
) const{
    if (new_move_slug.empty()){
        //  OCR failed — fall back to user's policy.
        return m_on_unknown == OnUnknownOffered::Stop ? FirstAction::Stop : FirstAction::Decline;
    }
    //  A move the user pinned in the team table is always taken.
    if (is_pinned(new_move_slug)){
        return FirstAction::Replace;
    }
    if (!m_auto_rank){
        return FirstAction::Decline;
    }

    //  Auto-ranking. Only displace a move we can prove is worse.
    const MoveData* offered = get_move_nothrow(new_move_slug);
    if (offered == nullptr || offered->category == "status" || offered->power == 0){
        //  Status move, or damage we can't quantify. Leave the set alone.
        return FirstAction::Decline;
    }

    //  Not worth committing a change right before the move pool shifts.
    if (!evolution_deferral_reason(new_move_slug).empty()){
        return FirstAction::Decline;
    }

    //  Without a cached moveset there is nothing to compare against, and
    //  guessing here would happily forget a good move. Decline instead.
    bool have_cache = false;
    for (int i = 0; i < 4; i++){
        if (!m_known_current[i].empty()){
            have_cache = true;
            break;
        }
    }
    if (!have_cache){
        return FirstAction::Decline;
    }

    //  An empty slot means the move is free to learn.
    for (int i = 0; i < 4; i++){
        if (m_known_current[i].empty()){
            return FirstAction::Replace;
        }
    }

    //  Take it only if it beats the weakest move we are allowed to forget.
    //  Pinned slots are excluded — auto-ranking never overrides the user.
    bool beaten = false;
    int slot = weakest_forgettable(new_move_slug, m_known_current, &beaten);
    if (slot < 0){
        //  Every current move is pinned. Nothing may be displaced.
        return FirstAction::Decline;
    }
    return beaten ? FirstAction::Replace : FirstAction::Decline;
}

int MoveLearnDecider::pick_forget_slot(
    const std::string& new_move_slug,
    const std::array<std::string, 4>& current_moves
) const{
    //  Defensive: if the offered move is already known, forget the duplicate
    //  slot rather than something else (shouldn't normally happen — the game
    //  doesn't offer moves you already have). Same move, so nothing is lost
    //  even if it is pinned.
    for (int i = 0; i < 4; i++){
        if (!current_moves[i].empty() && current_moves[i] == new_move_slug){
            return i;
        }
    }

    if (m_auto_rank){
        //  Forget the weakest unpinned move. -1 if all four are pinned.
        return weakest_forgettable(new_move_slug, current_moves, nullptr);
    }

    //  Not auto-ranking: nothing distinguishes the unpinned moves, so forget
    //  the first. A pinned move is never forgotten; if all four are pinned
    //  there is no legal slot.
    for (int i = 0; i < 4; i++){
        if (!is_pinned(current_moves[i])){
            return i;
        }
    }
    return -1;
}

std::vector<size_t> MoveLearnDecider::battle_move_priority(
    const std::array<std::string, 4>& current_moves
) const{
    std::vector<size_t> priority;
    priority.reserve(4);

    auto already_in = [&priority](size_t idx){
        for (size_t s : priority){
            if (s == idx) return true;
        }
        return false;
    };

    //  Walks the user's desired-priority list. For each desired slug that
    //  matches a current slot (and optionally is damaging), append the slot
    //  index. Skips slots already in `priority`.
    auto add_desired_in_order = [&](bool damaging_only){
        for (int d = 0; d < 4; d++){
            const std::string& want = m_desired[d];
            if (want.empty()) continue;
            for (int i = 0; i < 4; i++){
                if (current_moves[i] != want) continue;
                if (already_in((size_t)i)) continue;
                if (damaging_only && !is_damaging_move(current_moves[i])) continue;
                priority.push_back((size_t)i);
                break;
            }
        }
    };

    //  Appends any remaining occupied slot (optionally damaging) not already
    //  in `priority`. When auto-ranking, the damaging pass is ordered by
    //  STAB-adjusted power so the hardest-hitting move is used first; without
    //  it, slot order is preserved (legacy behaviour).
    auto add_remaining = [&](bool damaging_only){
        std::vector<size_t> candidates;
        for (int i = 0; i < 4; i++){
            if (current_moves[i].empty()) continue;
            if (already_in((size_t)i)) continue;
            if (damaging_only && !is_damaging_move(current_moves[i])) continue;
            candidates.push_back((size_t)i);
        }
        if (m_auto_rank && damaging_only){
            //  Stable sort on descending score keeps slot order as the
            //  tie-break for moves that score equally (or aren't rankable).
            std::stable_sort(
                candidates.begin(), candidates.end(),
                [this, &current_moves](size_t a, size_t b){
                    return move_score(current_moves[a]) > move_score(current_moves[b]);
                }
            );
        }
        for (size_t i : candidates){
            priority.push_back(i);
        }
    };

    //  Order of preference for grinding battles:
    //    1. Desired damaging moves (in user's priority order).
    //    2. Any other damaging move on the Pokemon (so we still attack if
    //       the desired moves are out of PP).
    //    3. Desired status moves (last-resort utility).
    //    4. Any other status move (last-resort PP before fleeing).
    //  This skips Growl/Tail Whip/Leer/etc. unless every damaging slot is
    //  exhausted — which is the right behaviour for an XP Grinder.
    add_desired_in_order(/*damaging_only=*/true);
    add_remaining       (/*damaging_only=*/true);
    add_desired_in_order(/*damaging_only=*/false);
    add_remaining       (/*damaging_only=*/false);

    if (priority.empty()){
        //  Nothing identified — fall back to slot 1 (legacy behaviour) so
        //  spam_first_move can still flee on PP exhaustion if needed.
        priority.push_back(0);
    }
    return priority;
}


}
}
}
