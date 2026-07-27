/*  Pokemon FRLG Move Learn Decider
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include <algorithm>
#include <climits>
#include <vector>
#include "PokemonFRLG/Resources/PokemonFRLG_MoveData.h"
#include "PokemonFRLG_MoveLearnDecider.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


MoveLearnDecider::MoveLearnDecider(
    std::array<std::string, 4> desired_slugs,
    OnUnknownOffered on_unknown,
    bool auto_rank,
    std::vector<std::string> species_types,
    std::array<std::string, 4> known_current
)
    : m_desired(std::move(desired_slugs))
    , m_on_unknown(on_unknown)
    , m_auto_rank(auto_rank)
    , m_species_types(std::move(species_types))
    , m_known_current(std::move(known_current))
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
    int new_score = move_score(new_move_slug);
    if (new_score == 0){
        //  Status move, or damage we can't quantify. Leave the set alone.
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
    int worst = INT_MAX;
    for (int i = 0; i < 4; i++){
        if (is_pinned(m_known_current[i])){
            continue;
        }
        worst = std::min(worst, move_score(m_known_current[i]));
    }
    if (worst == INT_MAX){
        //  Every current move is pinned. Nothing may be displaced.
        return FirstAction::Decline;
    }
    return new_score > worst ? FirstAction::Replace : FirstAction::Decline;
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

    if (m_auto_rank){
        //  Forget the weakest move that the user did not pin. Empty and
        //  status slots score 0 and so go first, which is what we want.
        int worst_slot = -1;
        int worst_score = INT_MAX;
        for (int i = 0; i < 4; i++){
            if (is_pinned(current_moves[i])){
                continue;
            }
            int s = move_score(current_moves[i]);
            if (s < worst_score){
                worst_score = s;
                worst_slot = i;
            }
        }
        if (worst_slot >= 0){
            return worst_slot;
        }
        //  All four pinned — fall through to the desired-rank tie-break so we
        //  still return something sane rather than always forgetting slot 0.
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
