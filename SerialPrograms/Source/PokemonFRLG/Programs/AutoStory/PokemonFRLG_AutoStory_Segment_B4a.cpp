/*  Pokemon FRLG AutoStory - Segment B4a
 *
 *  From: https://github.com/PokemonAutomation/
 */

#include "Common/Cpp/Color.h"
#include "PokemonFRLG/Programs/PokemonFRLG_KantoMapNavigator.h"
#include "PokemonFRLG/Inference/Map/PokemonFRLG_KantoGoals_Extended.h"
#include "PokemonFRLG_AutoStory_Segment_B4a.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

void run_B4a_vermilion_to_lavender(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
){
    (void)options;

    env.log("B4a: Vermilion Gym -> Route 6 -> Rock Tunnel -> Lavender Town", COLOR_BLUE);

    //  Single map-covered walk. Generous step budget: this is a long
    //  multi-screen trip (Route 6, the Rock Tunnel interior, then Lavender
    //  Town), substantially longer than the Route 2 -> Pewter City walk in
    //  B1a. navigate_to() throws OperationFailedException on step-budget
    //  exhaustion / unrecoverable encounters, so no extra failure handling
    //  is needed here.
    navigate_to(
        env, context, stats,
        KantoGoals::LavenderPokeCenterEntrance,
        &KantoGoals::Extended::VermilionCityGymDoor,
        700
    );

    env.log("Arrived in Lavender Town, having passed through Rock Tunnel.");
}

}
}
}
