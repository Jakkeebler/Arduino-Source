/*  Pokemon FRLG Move Learn State Machine
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Pure logic, no I/O. Bookkeeping for the move-learn flow in
 *  exit_wild_battle():
 *
 *      AwaitingPrompt -> ReadingNewMove -> ReadingMoveList
 *                     -> AwaitingForgetChoice -> Confirmed
 *
 *  The caller does the actual waiting, reading and button pressing; it reports
 *  how each attempt at the current state's work went, and the machine says
 *  whether to advance, try that state again, or give up and recover. Every
 *  edge is therefore bounded: MAX_CONSECUTIVE_FAILURES disagreements or
 *  timeouts in a row on one state ends in Recover, never an endless retry.
 *
 *  Also holds the multi-frame voting helpers. A single OCR read of the GBA
 *  pixel font is wrong often enough that committing to one frame can replace
 *  the wrong move, so reads are only trusted once a majority of independent
 *  snapshots agree.
 *
 */

#ifndef PokemonAutomation_PokemonFRLG_MoveLearnStateMachine_H
#define PokemonAutomation_PokemonFRLG_MoveLearnStateMachine_H

#include <array>
#include <chrono>
#include <cstddef>
#include <string>
#include <vector>

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


enum class MoveLearnState{
    AwaitingPrompt,         //  Waiting for the "wants to learn <Move>" select prompt.
    ReadingNewMove,         //  Prompt is up; OCR the offered move.
    ReadingMoveList,        //  On the forget screen; OCR the four current moves.
    AwaitingForgetChoice,   //  Slot chosen and confirmed; waiting for the game to act on it.
    Confirmed,              //  Done: declined, replaced or cancelled.
};
const char* move_learn_state_name(MoveLearnState state);


//  Tunables for the flow. A plain struct so callers can override one field.
struct MoveLearnConfig{
    //  Snapshots OCR'd per voting round. 1 disables voting (single read).
    //  2 requires both to agree; 3 requires any two.
    size_t vote_samples = 2;
    //  Gap between snapshots in a round. Long enough that a single transient
    //  (a mid-blink cursor, a half-drawn glyph) cannot appear in two of them.
    std::chrono::milliseconds vote_interval{200};
    //  Consecutive disagreements / timeouts on one state before Recover.
    size_t max_consecutive_failures = 3;
    //  How long to wait for each expected screen / dialog before counting a
    //  timeout against its state.
    std::chrono::milliseconds screen_timeout{5000};
    std::chrono::milliseconds confirm_timeout{3000};
};


class MoveLearnStateMachine{
public:
    enum class Outcome{
        Success,        //  The state's work finished and was trustworthy.
        Disagreement,   //  Votes did not form a majority (or read was empty).
        Timeout,        //  The expected screen never appeared.
    };
    enum class Step{
        Advance,        //  Moved on to the next state.
        Retry,          //  Failed, but under the cap: do this state's work again.
        Recover,        //  Cap reached: take the state's explicit recovery path.
    };

    MoveLearnStateMachine(size_t max_consecutive_failures = 3);

    MoveLearnState state() const{ return m_state; }
    size_t consecutive_failures() const{ return m_failures; }
    //  Total failures across all states, for logging.
    size_t total_failures() const{ return m_total_failures; }

    //  Report one attempt at the current state's work. Success advances to the
    //  next state in the chain. A failure counts toward the cap; reaching it
    //  returns Recover and resets the counter so the caller's recovery path
    //  starts from a clean slate. Confirmed is terminal and ignores reports.
    Step report(Outcome outcome);

    //  Jump straight to Confirmed: used by the decline / cancel paths, which
    //  never visit the forget screen.
    void finish();

    //  Jump to a specific state (used by recovery, e.g. back to reading after
    //  a re-prompt). Resets the failure counter.
    void enter(MoveLearnState state);

private:
    size_t m_max_failures;
    MoveLearnState m_state = MoveLearnState::AwaitingPrompt;
    size_t m_failures = 0;
    size_t m_total_failures = 0;
};


//  ---- Multi-frame voting ----

//  Votes needed out of `samples` readings: strict majority, minimum 1.
//  1 -> 1, 2 -> 2, 3 -> 2, 4 -> 3.
size_t votes_required(size_t samples);

//  Majority over several OCR reads of one value. Empty strings are failed
//  reads and never count as a vote, so two blank reads can't "agree" on a
//  value that wasn't read. Returns "" when no non-empty value reaches
//  votes_required(samples.size()).
std::string vote_string(const std::vector<std::string>& samples);

//  Per-slot majority over several reads of the four-move list. Succeeds only
//  if every slot reaches a majority; on failure `out` is left unchanged.
bool vote_move_list(
    const std::vector<std::array<std::string, 4>>& samples,
    std::array<std::string, 4>& out
);


}
}
}
#endif
