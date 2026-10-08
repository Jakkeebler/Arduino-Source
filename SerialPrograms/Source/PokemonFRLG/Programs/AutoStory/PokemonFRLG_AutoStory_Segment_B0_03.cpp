/*  Pokemon FRLG AutoStory - Segment B0_03
 *
 *  From: https://github.com/PokemonAutomation/
 */

#include "Common/Cpp/Color.h"
#include "PokemonFRLG/PokemonFRLG_Navigation.h"
#include "PokemonFRLG_AutoStory_Segment_B0_03.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

void run_B0_03_viridian_pc_heal(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
){
    env.log("B0_03: Viridian PC Heal", COLOR_BLUE);

    //  This is the mandatory first PC visit, not the opportunistic
    //  threshold-driven top-up ensure_healed() is built for, so navigate
    //  there and heal unconditionally instead of going through the
    //  battles-since-last-heal gate.
    navigate_to(env, context, stats, KantoGoals::ViridianPokeCenterEntrance);
    enter_building(env, context, BuildingType::POKECENTER);
    heal_at_pokecenter(env.console, context);
    exit_building(env, context, BuildingType::POKECENTER);

    env.log("Healed at the Viridian Pokemon Center.");
}

}
}
}
