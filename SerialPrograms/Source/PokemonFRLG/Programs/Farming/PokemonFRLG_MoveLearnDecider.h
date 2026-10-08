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
 *      2. If accepting, which slot (0..3) should be forgotten? Never a pinned
 *         (desired) move; of the rest, the one that adds least to the set under
 *         the scoring policy (STAB > weakness coverage > power > status).
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


//  One tier of the move-ranking policy. Moves are compared tier by tier, most
//  significant first; a later tier only matters when every earlier one ties.
enum class ScoreCriterion{
    Stab,       //  Damaging move matching one of the Pokemon's own types.
    Coverage,   //  Opponent types the move hits super-effectively that the rest
                //  of the moveset does not (see coverage_gain()).
    Power,      //  Base power.
    Damaging,   //  Any damaging move beats a status move.
};
struct MoveScorePolicy{
    //  Default: STAB > weakness coverage > power > status.
    std::array<ScoreCriterion, 4> order{
        ScoreCriterion::Stab,
        ScoreCriterion::Coverage,
        ScoreCriterion::Power,
        ScoreCriterion::Damaging,
    };
};

//  Everything about the Pokemon beyond its moves that the decider may use.
struct MoveLearnContext{
    //  Species slug. Needed only for evolution-aware protection.
    std::string species_slug;
    //  Level the Pokemon is at when the move is offered. -1 = infer it from the
    //  species' level-up learnset (a move is offered at exactly its learn level).
    int level = -1;
    //  Decline (and log why) when the Pokemon is exactly one level from a
    //  level-up evolution that changes its remaining move pool, instead of
    //  committing a moveset change right before the pool shifts. Only ever
    //  applies to auto-ranked decisions: a move the user pinned is still taken.
    bool evolution_protection = true;
    //  Types the coverage score tries to hit. Empty = every Gen-3 type.
    std::vector<std::string> opponent_types;
    MoveScorePolicy policy;
};


//  ---- Pure helpers (no data tables, unit-testable) ----

//  True if a level-up evolution at `evolution_level` is exactly one level away
//  from a move offered at `learn_level` AND that evolution changes which moves
//  are still to come. `evolution_level` <= 0 means "no level-up evolution".
bool should_defer_for_evolution(int learn_level, int evolution_level, bool movepool_changes);

//  Number of `opponent_types` that `move_type` hits super-effectively and that
//  none of `other_move_types` already hits super-effectively. Gen-3 chart.
//  Empty `opponent_types` means every Gen-3 type.
int coverage_gain(
    const std::string& move_type,
    const std::vector<std::string>& other_move_types,
    const std::vector<std::string>& opponent_types = {}
);


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
        std::array<std::string, 4> known_current = {},
        MoveLearnContext context = {}
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
    //  Algorithm: a pinned move is never forgotten. Of the rest, forget the one
    //  that contributes least under the scoring policy (auto-rank) or whose
    //  desired rank is worst (otherwise); ties go to the lowest slot index.
    //
    //  Returns -1 when no slot may be forgotten -- all four current moves are
    //  pinned and the offered move is not already known. The caller must cancel
    //  the learn rather than pick a slot.
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
    OnUnknownOffered on_unknown() const{ return m_on_unknown; }

    //  Damage-ranking score for one move, in tenths so the STAB multiplier
    //  stays exact in integer math (base power x10, x1.5 again for STAB).
    //  Returns 0 for anything that cannot be ranked by raw damage: empty or
    //  unknown slugs, status moves, and moves with no fixed base power (OHKO,
    //  Seismic Toss, Night Shade, Counter, Flail, Low Kick, ...). A 0 here
    //  means "don't rank me", NOT "worthless".
    int move_score(const std::string& slug) const;

    //  True if `slug` appears in the desired list, i.e. the user pinned it.
    bool is_pinned(const std::string& slug) const;

    //  True if `a` strictly outranks `b` under the scoring policy, with
    //  `others` being the rest of the moveset that coverage is measured against.
    //  An empty slug always loses; an unknown slug ranks as a status move.
    bool outranks(
        const std::string& a, const std::string& b,
        const std::vector<std::string>& others
    ) const;

    //  Non-empty (a log-ready reason) if evolution protection says to hold off
    //  on committing a moveset change for this offered move.
    std::string evolution_deferral_reason(const std::string& new_move_slug) const;

private:
    //  Returns the position of `slug` in m_desired (0..3, 0 = highest priority),
    //  or INT_MAX if not found / slug is empty.
    int desired_rank(const std::string& slug) const;

    //  Ranking key for one move, in policy order, compared lexicographically.
    std::array<int, 4> rank_key(
        const std::string& slug, const std::vector<std::string>& others
    ) const;

    //  Weakest slot of `current` that may be forgotten for `new_move_slug`
    //  (unpinned, scored by marginal contribution to the rest of the set), or
    //  -1 if every slot is pinned. `*beaten` reports whether the new move
    //  outranks that slot.
    int weakest_forgettable(
        const std::string& new_move_slug,
        const std::array<std::string, 4>& current,
        bool* beaten
    ) const;

    std::array<std::string, 4> m_desired;
    OnUnknownOffered m_on_unknown;
    bool m_auto_rank;
    std::vector<std::string> m_species_types;
    std::array<std::string, 4> m_known_current;
    MoveLearnContext m_context;
};


}
}
}
#endif
