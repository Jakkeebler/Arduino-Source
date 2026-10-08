/*  Pokemon FRLG AutoStory - Segment B3a
 *
 *  From: https://github.com/PokemonAutomation/
 */

#include "Common/Cpp/Color.h"
#include "PokemonFRLG/Inference/Map/PokemonFRLG_KantoGoals_Extended.h"
#include "PokemonFRLG/Programs/PokemonFRLG_KantoMapNavigator.h"
#include "PokemonFRLG_AutoStory_Segment_B3a.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

void run_B3a_cerulean_to_ssanne(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
){
    (void)options;

    env.log("B3a: Cerulean Gym -> Route 5 -> Underground Path -> Route 6 -> S.S. Anne gangplank", COLOR_BLUE);

    //  ---- Cerulean Gym door -> Route 5 south entrance ----
    //  No start_hint: this is the first navigation of the segment, same as
    //  every other segment entry point.
    navigate_to(env, context, stats, KantoGoals::Extended::Route5SouthEntranceDoor1, nullptr, 250);

    //  ---- Route 5 -> Underground Path north entrance ----
    navigate_to(
        env, context, stats,
        KantoGoals::Extended::UndergroundPathNorthEntranceDoor1,
        &KantoGoals::Extended::Route5SouthEntranceDoor1,
        100
    );

    //  ---- Underground Path tunnel traversal (north -> south) ----
    //  The tunnel interior is part of the combined map asset (it is one of
    //  the "key interior dungeons" the navigator doc lists), so this is a
    //  normal A* walk like any outdoor leg.
    navigate_to(
        env, context, stats,
        KantoGoals::Extended::UndergroundPathSouthEntranceDoor1,
        &KantoGoals::Extended::UndergroundPathNorthEntranceDoor1,
        150
    );

    //  ---- Underground Path south exit -> Route 6 north entrance ----
    navigate_to(
        env, context, stats,
        KantoGoals::Extended::Route6NorthEntranceDoor1,
        &KantoGoals::Extended::UndergroundPathSouthEntranceDoor1,
        100
    );

    //  ---- Route 6 -> Vermilion City -> S.S. Anne gangplank ----
    navigate_to(
        env, context, stats,
        KantoGoals::Extended::SsanneExteriorDoor1,
        &KantoGoals::Extended::Route6NorthEntranceDoor1,
        250
    );

    env.log("Arrived at the S.S. Anne gangplank in Vermilion City.");
}

}
}
}
