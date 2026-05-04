/*  Pokemon FRLG Move Learn Decider
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include <climits>
#include <vector>
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

    //  Pass 1: walk desired in order. For each desired slug, find the slot
    //  in current_moves that matches and append its index.
    for (int d = 0; d < 4; d++){
        const std::string& want = m_desired[d];
        if (want.empty()){
            continue;
        }
        for (int i = 0; i < 4; i++){
            if (current_moves[i] == want){
                bool already_in = false;
                for (size_t s : priority){
                    if (s == (size_t)i){ already_in = true; break; }
                }
                if (!already_in){
                    priority.push_back((size_t)i);
                }
                break;
            }
        }
    }

    //  Pass 2: append any current slot we haven't used yet, so an unwanted-
    //  but-known move is still usable as last-resort PP rather than fleeing.
    for (int i = 0; i < 4; i++){
        if (current_moves[i].empty()){
            continue;
        }
        bool already_in = false;
        for (size_t s : priority){
            if (s == (size_t)i){ already_in = true; break; }
        }
        if (!already_in){
            priority.push_back((size_t)i);
        }
    }

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
