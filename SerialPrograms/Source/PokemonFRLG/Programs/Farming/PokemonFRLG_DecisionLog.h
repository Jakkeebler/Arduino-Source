/*  Pokemon FRLG Decision Log
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  FRO-227 (Phase 4): structured, diagnosable decision records for the XP
 *  Grinder. The grinder already narrates most of what it does with free-text
 *  env.log() lines; those stay (they're what a human watches live). This adds
 *  a second, machine-greppable line next to the decisions that most need to
 *  be reconstructed after the fact from a log file alone -- OCR confidence,
 *  move-learn outcomes, position-drift corrections, and evolution-policy
 *  calls -- without re-running the program live.
 *
 *  Format: one line per decision, always starting with the literal tag
 *  "DECISION" so `grep DECISION run.log` pulls every one, followed by
 *  space-separated key=value fields. The first field is always
 *  type=<DecisionType>. Values containing a space or '=' are wrapped in
 *  double quotes.
 *
 *  Example:
 *      DECISION type=MoveLearn pokemon=2 species=charmeleon move="Dragon Rage" outcome=accepted
 *
 *  Pure formatting -- no I/O. Callers still own writing the line to the
 *  program log (env.log(record.to_line(), color)), same as every other log
 *  call in this program.
 */

#ifndef PokemonAutomation_PokemonFRLG_DecisionLog_H
#define PokemonAutomation_PokemonFRLG_DecisionLog_H

#include <string>
#include <vector>
#include <utility>

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


enum class DecisionType{
    OcrConfidence,
    MoveLearn,
    PositionDrift,
    EvolutionPolicy,
    StopCondition,
};
const char* decision_type_name(DecisionType type);


class DecisionRecord{
public:
    explicit DecisionRecord(DecisionType type);

    //  Appends one key=value field, in call order. Returns *this so calls
    //  can be chained: DecisionRecord(...).field(...).field(...).outcome(...)
    DecisionRecord& field(const std::string& key, const std::string& value);
    DecisionRecord& field(const std::string& key, long long value);
    DecisionRecord& field(const std::string& key, double value);
    DecisionRecord& field(const std::string& key, bool value);

    //  Convenience: always-last "outcome=" field. Not required to be last by
    //  the format, just a readability convention -- call it whenever the
    //  decision has settled.
    DecisionRecord& outcome(const std::string& value);

    //  Render the full line, e.g. "DECISION type=MoveLearn pokemon=2 ...".
    std::string to_line() const;

private:
    DecisionType m_type;
    std::vector<std::pair<std::string, std::string>> m_fields;
};


}
}
}
#endif
