/*  Pokemon FRLG Decision Log
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include <sstream>
#include "PokemonFRLG_DecisionLog.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


const char* decision_type_name(DecisionType type){
    switch (type){
    case DecisionType::OcrConfidence:   return "OcrConfidence";
    case DecisionType::MoveLearn:       return "MoveLearn";
    case DecisionType::PositionDrift:   return "PositionDrift";
    case DecisionType::EvolutionPolicy: return "EvolutionPolicy";
    case DecisionType::StopCondition:   return "StopCondition";
    }
    return "Unknown";
}


namespace{
    //  Quote a value if it contains whitespace or '=', so the line stays
    //  parseable as space-separated key=value pairs. Internal quotes are
    //  escaped so a naive split on unescaped '"' still works.
    std::string quote_if_needed(const std::string& value){
        bool needs_quotes = value.empty();
        for (char c : value){
            if (c == ' ' || c == '=' || c == '"'){
                needs_quotes = true;
                break;
            }
        }
        if (!needs_quotes){
            return value;
        }
        std::string out = "\"";
        for (char c : value){
            if (c == '"'){
                out += "\\\"";
            }else{
                out += c;
            }
        }
        out += "\"";
        return out;
    }
}


DecisionRecord::DecisionRecord(DecisionType type)
    : m_type(type)
{}

DecisionRecord& DecisionRecord::field(const std::string& key, const std::string& value){
    m_fields.emplace_back(key, quote_if_needed(value));
    return *this;
}
DecisionRecord& DecisionRecord::field(const std::string& key, long long value){
    m_fields.emplace_back(key, std::to_string(value));
    return *this;
}
DecisionRecord& DecisionRecord::field(const std::string& key, double value){
    m_fields.emplace_back(key, std::to_string(value));
    return *this;
}
DecisionRecord& DecisionRecord::field(const std::string& key, bool value){
    m_fields.emplace_back(key, value ? "true" : "false");
    return *this;
}
DecisionRecord& DecisionRecord::outcome(const std::string& value){
    return field("outcome", value);
}

std::string DecisionRecord::to_line() const{
    std::ostringstream out;
    out << "DECISION type=" << decision_type_name(m_type);
    for (const auto& kv : m_fields){
        out << ' ' << kv.first << '=' << kv.second;
    }
    return out.str();
}


}
}
}
