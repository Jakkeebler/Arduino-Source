/*  Read Party
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include <chrono>
#include "Common/Cpp/Color.h"
#include "CommonFramework/VideoPipeline/VideoFeed.h"
#include "CommonFramework/VideoPipeline/VideoOverlayScopes.h"
#include "NintendoSwitch/Commands/NintendoSwitch_Commands_PushButtons.h"
#include "Pokemon/Pokemon_Strings.h"
#include "Pokemon/Inference/Pokemon_NameReader.h"
#include "PokemonFRLG/Inference/PokemonFRLG_PartySummaryReader.h"
#include "PokemonFRLG/Resources/PokemonFRLG_SpeciesData.h"
#include "PokemonFRLG_ReadParty.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

using namespace std::chrono_literals;


ReadParty_Descriptor::ReadParty_Descriptor()
    : SingleSwitchProgramDescriptor(
        "PokemonFRLG:ReadParty",
        Pokemon::STRING_POKEMON + " FRLG",
        "Read Party Summary", "",
        "Diagnostic: read the currently-displayed Summary screen (page 1 dex# + "
        "nickname, then navigate to page 3 for the 4 move names). Use this to "
        "calibrate the Summary OCR float-boxes. Start with Summary page 1 visible.",
        ProgramControllerClass::StandardController_NoRestrictions,
        FeedbackType::REQUIRED,
        AllowCommandsWhenRunning::DISABLE_COMMANDS
    )
{}

ReadParty::ReadParty()
    : LANGUAGE(
        "<b>Game Language:</b>",
        Pokemon::PokemonNameReader::instance().languages(),
        LockMode::LOCK_WHILE_RUNNING, true
    )
{
    PA_ADD_OPTION(LANGUAGE);
}

void ReadParty::program(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context
){
    env.log("Starting Read Party. Ensure Summary page 1 is visible.");

    PartySummaryReader reader;
    VideoOverlaySet overlays(env.console.overlay());
    reader.make_overlays(overlays);

    PartySummaryRead out;

    env.log("Reading Summary page 1 (dex# + nickname)...");
    VideoSnapshot screen1 = env.console.video().snapshot();
    reader.read_page1(env.logger(), LANGUAGE, screen1, out);

    env.log("Level: " + (out.level >= 0 ? std::to_string(out.level) : std::string("???")));
    env.log("Dex#: " + (out.dex_no >= 0 ? std::to_string(out.dex_no) : std::string("???")));
    env.log("Nickname OCR: '" + out.nickname + "'");
    if (out.dex_no >= 0){
        const SpeciesData* sp = get_species_by_dex((uint16_t)out.dex_no);
        env.log("Species (from dex lookup): " + (sp != nullptr ? sp->slug : std::string("???")));
    }

    env.log("Navigating to Summary page 3 (Skills/Moves)...");
    pbf_press_dpad(context, DPAD_RIGHT, 100ms, 100ms);
    pbf_press_dpad(context, DPAD_RIGHT, 100ms, 100ms);
    context.wait_for_all_requests();
    pbf_wait(context, 700ms);
    context.wait_for_all_requests();

    env.log("Reading Summary page 3 (4 moves)...");
    VideoSnapshot screen3 = env.console.video().snapshot();
    reader.read_page3_moves(env.logger(), LANGUAGE, screen3, out);

    for (int i = 0; i < 4; i++){
        env.log("Move " + std::to_string(i + 1) + ": '" + out.move_slugs[i] + "'");
    }

    env.log("Finished. Verification overlays remain visible for 10s.", COLOR_BLUE);
    pbf_wait(context, 10s);
    context.wait_for_all_requests();
}


}
}
}
