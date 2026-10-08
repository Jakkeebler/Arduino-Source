/*  Pokemon FRLG Walking Drift Guard
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include <algorithm>
#include <cstdlib>
#include "PokemonFRLG_WalkingDriftGuard.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


DriftCheckResult check_walk_drift(
    int predicted_x, int predicted_y,
    int actual_x, int actual_y,
    bool interrupted
){
    DriftCheckResult result;
    if (interrupted){
        return result;
    }
    const int dx = std::abs(predicted_x - actual_x);
    const int dy = std::abs(predicted_y - actual_y);
    result.drift_tiles = std::max(dx, dy);
    result.should_relocalize = result.drift_tiles > 0;
    return result;
}

bool drift_check_due(int steps_since_last_check, int interval){
    if (interval <= 1){
        return true;
    }
    return steps_since_last_check >= interval;
}


CollisionGuard::CollisionGuard(int attempts_before_reroute)
    : m_threshold(std::max(1, attempts_before_reroute))
{}

CollisionDecision CollisionGuard::report(bool moved, bool expected_no_move){
    if (moved){
        m_count = 0;
        return CollisionDecision::NotACollision;
    }
    if (expected_no_move){
        //  Hold steady rather than reset: a genuinely blocked tile right
        //  after a turn or an interrupted step must still trip the guard on
        //  the next unexplained stall, not get a fresh budget forever.
        return CollisionDecision::NotACollision;
    }
    m_count++;
    if (m_count >= m_threshold){
        return CollisionDecision::Reroute;
    }
    return CollisionDecision::KeepTrying;
}


}
}
}
