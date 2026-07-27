/*  Team Scanner
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include "Common/Cpp/Color.h"
#include "Common/Cpp/Json/JsonValue.h"
#include "Common/Cpp/Exceptions.h"
#include "CommonFramework/Exceptions/OperationFailedException.h"
#include "CommonFramework/Notifications/ProgramNotifications.h"
#include "Pokemon/Pokemon_Strings.h"
#include "Pokemon/Inference/Pokemon_NameReader.h"
#include "PokemonFRLG/Programs/PokemonFRLG_PartyScanner.h"
#include "PokemonFRLG_XpGrinderTeamTable.h"
#include "PokemonFRLG_TeamScanner.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


TeamScanner_Descriptor::TeamScanner_Descriptor()
    : SingleSwitchProgramDescriptor(
        "PokemonFRLG:TeamScanner",
        Pokemon::STRING_POKEMON + " FRLG",
        "Team Scanner",
        "",
        "Scan the party (species + moves) and export a team file that the XP "
        "Grinder can import. Start on the overworld with the start menu CLOSED.",
        ProgramControllerClass::StandardController_NoRestrictions,
        FeedbackType::REQUIRED,
        AllowCommandsWhenRunning::DISABLE_COMMANDS
    )
{}

TeamScanner::TeamScanner()
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
    , OUTPUT_FILE(
        false,
        "<b>Team File:</b><br>Path to write the scanned team to. The XP Grinder "
        "imports this same file. Relative paths are resolved from the program's "
        "working directory.",
        LockMode::LOCK_WHILE_RUNNING,
        "FRLG_Team.json",
        "FRLG_Team.json"
    )
    , NOTIFICATIONS({
        &NOTIFICATION_PROGRAM_FINISH,
        &NOTIFICATION_ERROR_FATAL,
    })
{
    PA_ADD_OPTION(LANGUAGE);
    PA_ADD_OPTION(PARTY_SIZE);
    PA_ADD_OPTION(OUTPUT_FILE);
    PA_ADD_OPTION(NOTIFICATIONS);
}

void TeamScanner::program(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context
){
    const int party_size = (int)(uint64_t)PARTY_SIZE;
    env.log("Team Scanner: scanning " + std::to_string(party_size) + " slot(s).", COLOR_BLUE);

    std::vector<PartyScanResult> results = scan_party(env, context, LANGUAGE, party_size);

    //  Build a Team Table from the scan results. We reuse the XP Grinder's own
    //  table type so its JSON serialization is identical and the grinder can
    //  load the file directly. One row per scanned slot, in lead order.
    XpGrinderTeamTable table;
    table.clear();
    for (size_t i = 0; i < results.size(); i++){
        table.append_row(table.make_row());
    }
    for (size_t i = 0; i < results.size(); i++){
        const PartyScanResult& r = results[i];
        //  Species first: setting it rebuilds the row's chain move database so
        //  the desired-move slugs below can resolve against it.
        if (!r.species_slug.empty()){
            table.set_species(i, r.species_slug);
        }
        table.set_desired_moves(i, r.read.move_slugs);
    }

    std::string path = OUTPUT_FILE;
    if (path.empty()){
        path = "FRLG_Team.json";
    }
    try{
        table.to_json().dump(path);
    }catch (const Exception& e){
        OperationFailedException::fire(
            ErrorReport::NO_ERROR_REPORT,
            "Team Scanner: failed to write team file '" + path + "': " + e.message(),
            env.console
        );
    }

    env.log(
        "Team Scanner: wrote " + std::to_string(results.size()) +
        " Pokemon to '" + path + "'.",
        COLOR_BLUE
    );
    send_program_finished_notification(env, NOTIFICATION_PROGRAM_FINISH);
}


}
}
}
