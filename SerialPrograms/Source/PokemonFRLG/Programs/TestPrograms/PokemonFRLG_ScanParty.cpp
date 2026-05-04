/*  Scan Party
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include "Common/Cpp/Color.h"
#include "Pokemon/Pokemon_Strings.h"
#include "Pokemon/Inference/Pokemon_NameReader.h"
#include "PokemonFRLG/Programs/PokemonFRLG_PartyScanner.h"
#include "PokemonFRLG_ScanParty.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


ScanParty_Descriptor::ScanParty_Descriptor()
    : SingleSwitchProgramDescriptor(
        "PokemonFRLG:ScanParty",
        Pokemon::STRING_POKEMON + " FRLG",
        "Scan Party", "",
        "Diagnostic: walks the party menu and OCR-reads each Pokemon's Summary "
        "(dex#, nickname, 4 moves). Logs results per slot and returns to the "
        "overworld. Start from the overworld with the start menu CLOSED.",
        ProgramControllerClass::StandardController_NoRestrictions,
        FeedbackType::REQUIRED,
        AllowCommandsWhenRunning::DISABLE_COMMANDS
    )
{}

ScanParty::ScanParty()
    : LANGUAGE(
        "<b>Game Language:</b>",
        Pokemon::PokemonNameReader::instance().languages(),
        LockMode::LOCK_WHILE_RUNNING, true
    )
    , PARTY_SIZE(
        "<b>Party Size:</b><br>Number of party slots to scan (1-6).",
        LockMode::LOCK_WHILE_RUNNING,
        1, 1, 6
    )
{
    PA_ADD_OPTION(LANGUAGE);
    PA_ADD_OPTION(PARTY_SIZE);
}

void ScanParty::program(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context
){
    env.log("Scan Party: starting walk over " + std::to_string((uint64_t)PARTY_SIZE) + " slot(s).", COLOR_BLUE);

    auto results = scan_party(env, context, LANGUAGE, (int)(uint64_t)PARTY_SIZE);

    env.log("Scan Party: finished. Per-slot summary:", COLOR_BLUE);
    for (const PartyScanResult& r : results){
        env.log(
            "  Slot " + std::to_string(r.slot_1indexed) + ": " +
            "lv=" + (r.read.level >= 0 ? std::to_string(r.read.level) : std::string("?")) +
            " dex=" + (r.read.dex_no >= 0 ? std::to_string(r.read.dex_no) : std::string("?")) +
            " species='" + r.species_slug + "'" +
            " nickname='" + r.read.nickname + "'" +
            " moves=[" + r.read.move_slugs[0] + "|" + r.read.move_slugs[1] +
            "|" + r.read.move_slugs[2] + "|" + r.read.move_slugs[3] + "]"
        );
    }
}


}
}
}
