/*  Pokemon FRLG Menu Confirm Guard
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Pure logic, no I/O. Generic "read-verify-before-confirm" bookkeeping for
 *  a menu selection that is expensive or impossible to undo once confirmed
 *  (the Fly map destination cursor, the Teleport/Dig field-move confirms).
 *  Mirrors MoveLearnStateMachine's bounded-retry shape: the caller does the
 *  actual reading and button presses and reports how each attempt went; this
 *  only tracks the bounded retry budget and says whether to proceed, retry,
 *  or give up.
 *
 *  Before FRO-225 none of Fly/Teleport/Dig's menu confirms read back what was
 *  actually selected before pressing the confirming button -- a missed or
 *  garbled selection (wrong Fly destination, wrong field-move slot) just
 *  silently went through. Route every such confirm through this instead of a
 *  bare "press A and hope".
 *
 *  See FRO-225 (FRLG Auto-Train Phase 2).
 */

#ifndef PokemonAutomation_PokemonFRLG_MenuConfirmGuard_H
#define PokemonAutomation_PokemonFRLG_MenuConfirmGuard_H

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


class MenuConfirmGuard{
public:
    enum class Outcome{
        Verified,     //  The read-back matches what we intended to select.
        Mismatch,     //  Read-back succeeded but disagrees (wrong cursor spot).
        Unreadable,   //  Could not get a read-back at all (detector miss / timeout).
    };
    enum class Step{
        Confirm,   //  Proceed to press the confirming button.
        Retry,     //  Re-attempt the selection (re-move the cursor and re-read).
        GiveUp,    //  Retry budget exhausted. Caller must stop and report.
    };

    explicit MenuConfirmGuard(int max_attempts = 3);

    //  Report one attempt's read-back outcome and get the next step.
    Step report(Outcome outcome);

    int attempts() const{ return m_attempts; }
    void reset(){ m_attempts = 0; }

private:
    int m_max_attempts;
    int m_attempts = 0;
};


}
}
}
#endif
