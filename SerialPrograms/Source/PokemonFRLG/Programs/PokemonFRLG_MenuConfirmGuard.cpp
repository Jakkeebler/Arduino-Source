/*  Pokemon FRLG Menu Confirm Guard
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include <algorithm>
#include "PokemonFRLG_MenuConfirmGuard.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


MenuConfirmGuard::MenuConfirmGuard(int max_attempts)
    : m_max_attempts(std::max(1, max_attempts))
{}

MenuConfirmGuard::Step MenuConfirmGuard::report(Outcome outcome){
    if (outcome == Outcome::Verified){
        m_attempts = 0;
        return Step::Confirm;
    }
    m_attempts++;
    if (m_attempts >= m_max_attempts){
        return Step::GiveUp;
    }
    return Step::Retry;
}


}
}
}
