/*  Pokemon FRLG Move Learn Decider
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include <climits>
#include <vector>
#include "PokemonFRLG/Resources/PokemonFRLG_MoveData.h"
#include "PokemonFRLG_MoveLearnDecider.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


MoveLearnDecider::MoveLearnDecider(
    std::array<std::string, 4> desired_slugs,
    OnUnknownOffered on_unknown
)
    : m_desired(std::move(desired_slugs))
    , m_on_unknown(on_unknown)
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

MoveLearnDecider::FirstAction MoveLearnDecider::decide_accept_or_decline(
    const std::string& new_move_slug
) const{
    if (new_move_slug.empty()){
        //  OCR failed — fall back to user's policy.
        return m_on_unknown == OnUnknownOffered::Stop ? FirstAction::Stop : FirstAction::Decline;
    }
    int rank = desired_rank(new_move_slug);
    return rank == INT_MAX ? FirstAction::Decline : FirstAction::Replace;
}

int MoveLearnDecider::pick_forget_slot(
    const std::string& new_move_slug,
    const std::array<std::string, 4>& current_moves
) const{
    //  Defensive: if the offered move is already known, forget the duplicate
    //  slot rather than something else (shouldn't normally happen — the game
    //  doesn't offer moves you already have).
    for (int i = 0; i < 4; i++){
        if (!current_moves[i].empty() && current_moves[i] == new_move_slug){
            return i;
        }
    }

    //  Pick the slot with the WORST rank (highest desired_rank value).
    //  Slots not in desired (rank=INT_MAX) get forgotten first; ties broken by
    //  lowest slot index.
    int worst_slot = 0;
    int worst_rank = desired_rank(current_moves[0]);
    for (int i = 1; i < 4; i++){
        int r = desired_rank(current_moves[i]);
        if (r > worst_rank){
            worst_rank = r;
            worst_slot = i;
        }
    }
    return worst_slot;
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
    //  in `priority`.
    auto add_remaining = [&](bool damaging_only){
        for (int i = 0; i < 4; i++){
            if (current_moves[i].empty()) continue;
            if (already_in((size_t)i)) continue;
            if (damaging_only && !is_damaging_move(current_moves[i])) continue;
            priority.push_back((size_t)i);
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
