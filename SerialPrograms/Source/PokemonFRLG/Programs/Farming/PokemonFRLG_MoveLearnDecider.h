/*  Pokemon FRLG Move Learn Decider
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Pure logic for the smart move-learn decision. Snapshot-style: holds an
 *  immutable view of one Pokemon's desired final 4-move set and answers two
 *  questions during a level-up "<Pokemon> wants to learn <Move>!" dialog:
 *
 *      1. Should we accept the new move (FirstAction = Replace) or decline it
 *         (FirstAction = Decline / Stop)?
 *      2. If accepting, which slot (0..3) should be forgotten? Pick the
 *         current move that is NOT in the desired set; if all 4 current
 *         moves are desired, pick the one with the lowest priority rank
 *         in the desired list.
 *
 *  The decider has no I/O. The caller (exit_wild_battle) is responsible for
 *  OCR'ing inputs and pressing buttons.
 *
 */

#ifndef PokemonAutomation_PokemonFRLG_MoveLearnDecider_H
#define PokemonAutomation_PokemonFRLG_MoveLearnDecider_H

#include <array>
#include <string>
#include <vector>

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


//  What to do when an unknown / unreadable new move is offered. The user
//  picks this per Pokemon row in the team table.
enum class OnUnknownOffered{
    Decline,    //  Default: skip moves we can't identify.
    Stop,       //  Halt the program for manual review.
};


class MoveLearnDecider{
public:
    enum class FirstAction{
        Decline,    //  Press B on the "should X forget a move?" prompt.
        Stop,       //  Halt the program; let the caller surface the prompt.
        Replace,    //  Press A; navigate the forget screen using pick_forget_slot().
    };

    //  desired_slugs: ordered priority. Slot 0 = highest priority, slot 3 = lowest.
    //  Empty strings mean "no preference for this priority slot" — they never match.
    //  Moves in this list are "pinned": they are always accepted when offered
    //  and are never chosen to be forgotten.
    //
    //  auto_rank: when true, moves that are NOT pinned are judged by
    //  STAB-adjusted base power instead of being blanket-declined. This lets
    //  the grinder improve a moveset the user hasn't fully specified.
    //  species_types: the Pokemon's Gen-3 typing, used for the STAB bonus.
    //  Empty means "no STAB information" — ranking still works, just without
    //  the 1.5x same-type bonus.
    //  known_current: the caller's cached view of the Pokemon's current four
    //  moves. Auto-ranking needs this to answer accept-vs-decline, because the
    //  game only shows the real moveset after the offer is accepted. If every
    //  entry is empty the decider has no basis to compare and declines
    //  unpinned offers.
    MoveLearnDecider(
        std::array<std::string, 4> desired_slugs,
        OnUnknownOffered on_unknown = OnUnknownOffered::Decline,
        bool auto_rank = false,
        std::vector<std::string> species_types = {},
        std::array<std::string, 4> known_current = {}
    );

    //  First decision: should we accept or decline the offered move?
    //  new_move_slug: the result of LearnMoveDialogReader. Empty string means
    //  OCR failed — falls back to on_unknown action.
    FirstAction decide_accept_or_decline(const std::string& new_move_slug) const;

    //  Second decision: which slot (0..3) should be forgotten?
    //  current_moves: the 4 current move slugs (typically OCR'd from the
    //  forget-move screen). Any slot whose slug is empty is treated as
    //  "unknown" and considered discardable.
    //
    //  Algorithm: rank each current slot by its priority in desired_slugs
    //  (lower rank = better; not in desired = INT_MAX). Forget the slot with
    //  the worst rank. Ties broken by lowest slot index.
    int pick_forget_slot(
        const std::string& new_move_slug,
        const std::array<std::string, 4>& current_moves
    ) const;

    //  Convenience: compute the move-priority list for spam_first_move() given
    //  the latest known current_moves. Returns 0-indexed slot numbers in
    //  preference order: highest-priority desired moves we actually have first,
    //  then any remaining slots (so we still use unwanted moves as a last
    //  resort instead of fleeing).
    std::vector<size_t> battle_move_priority(
        const std::array<std::string, 4>& current_moves
    ) const;

    const std::array<std::string, 4>& desired() const{ return m_desired; }

    //  Damage-ranking score for one move, in tenths so the STAB multiplier
    //  stays exact in integer math (base power x10, x1.5 again for STAB).
    //  Returns 0 for anything that cannot be ranked by raw damage: empty or
    //  unknown slugs, status moves, and moves with no fixed base power (OHKO,
    //  Seismic Toss, Night Shade, Counter, Flail, Low Kick, ...). A 0 here
    //  means "don't rank me", NOT "worthless".
    int move_score(const std::string& slug) const;

    //  True if `slug` appears in the desired list, i.e. the user pinned it.
    bool is_pinned(const std::string& slug) const;

private:
    //  Returns the position of `slug` in m_desired (0..3, 0 = highest priority),
    //  or INT_MAX if not found / slug is empty.
    int desired_rank(const std::string& slug) const;

    std::array<std::string, 4> m_desired;
    OnUnknownOffered m_on_unknown;
    bool m_auto_rank;
    std::vector<std::string> m_species_types;
    std::array<std::string, 4> m_known_current;
};


}
}
}
#endif
