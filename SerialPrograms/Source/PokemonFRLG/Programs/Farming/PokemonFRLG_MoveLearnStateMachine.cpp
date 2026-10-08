/*  Pokemon FRLG Move Learn State Machine
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include <map>
#include "PokemonFRLG_MoveLearnStateMachine.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


const char* move_learn_state_name(MoveLearnState state){
    switch (state){
    case MoveLearnState::AwaitingPrompt:       return "AwaitingPrompt";
    case MoveLearnState::ReadingNewMove:       return "ReadingNewMove";
    case MoveLearnState::ReadingMoveList:      return "ReadingMoveList";
    case MoveLearnState::AwaitingForgetChoice: return "AwaitingForgetChoice";
    case MoveLearnState::Confirmed:            return "Confirmed";
    }
    return "?";
}


MoveLearnStateMachine::MoveLearnStateMachine(size_t max_consecutive_failures)
    : m_max_failures(max_consecutive_failures == 0 ? 1 : max_consecutive_failures)
{}

MoveLearnStateMachine::Step MoveLearnStateMachine::report(Outcome outcome){
    if (m_state == MoveLearnState::Confirmed){
        return Step::Advance;
    }
    if (outcome == Outcome::Success){
        m_failures = 0;
        m_state = (MoveLearnState)((int)m_state + 1);
        return Step::Advance;
    }
    m_failures++;
    m_total_failures++;
    if (m_failures >= m_max_failures){
        m_failures = 0;
        return Step::Recover;
    }
    return Step::Retry;
}

void MoveLearnStateMachine::finish(){
    m_state = MoveLearnState::Confirmed;
    m_failures = 0;
}

void MoveLearnStateMachine::enter(MoveLearnState state){
    m_state = state;
    m_failures = 0;
}


size_t votes_required(size_t samples){
    if (samples <= 1){
        return 1;
    }
    return samples / 2 + 1;
}

std::string vote_string(const std::vector<std::string>& samples){
    if (samples.empty()){
        return std::string();
    }
    const size_t need = votes_required(samples.size());
    std::map<std::string, size_t> counts;
    for (const std::string& s : samples){
        if (!s.empty()){
            counts[s]++;
        }
    }
    for (const auto& item : counts){
        if (item.second >= need){
            return item.first;
        }
    }
    return std::string();
}

bool vote_move_list(
    const std::vector<std::array<std::string, 4>>& samples,
    std::array<std::string, 4>& out
){
    if (samples.empty()){
        return false;
    }
    std::array<std::string, 4> result;
    for (size_t slot = 0; slot < 4; slot++){
        std::vector<std::string> column;
        column.reserve(samples.size());
        for (const auto& sample : samples){
            column.push_back(sample[slot]);
        }
        result[slot] = vote_string(column);
        if (result[slot].empty()){
            return false;
        }
    }
    out = std::move(result);
    return true;
}


}
}
}
