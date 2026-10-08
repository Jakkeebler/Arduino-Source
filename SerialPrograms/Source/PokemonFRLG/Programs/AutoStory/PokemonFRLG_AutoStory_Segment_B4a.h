/*  Pokemon FRLG AutoStory - Segment B4a
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Navigate from the Vermilion Gym (Thunder Badge in hand) through Route 6,
 *  the Rock Tunnel, and out the far side into Lavender Town.
 */

#ifndef PokemonAutomation_PokemonFRLG_AutoStory_Segment_B4a_H
#define PokemonAutomation_PokemonFRLG_AutoStory_Segment_B4a_H

#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "PokemonFRLG/Programs/AutoStory/PokemonFRLG_AutoStoryTools.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

//  Start: outside the Vermilion Gym, Thunder Badge in hand (B3c's end state).
//  End:   in Lavender Town, having passed through Rock Tunnel
//         (KantoGoals::LavenderPokeCenterEntrance).
//
//  This phase boundary matches the Phase table already registered in
//  PokemonFRLG_AutoStory.cpp (Phase::B4a / Phase::B4b) -- that is the
//  reviewed source of truth for where B4a ends and B4b begins, which does
//  not line up 1:1 with the FRO-183 issue text's "B4a_Route7_Celadon" /
//  "B4b_Erika" naming. Per that table, B4a owns the Vermilion -> Lavender
//  leg (Route 6 + Rock Tunnel); B4b (see Segment_B4b) owns Lavender ->
//  Celadon -> Erika -> Rainbow Badge.
//
//  The whole Route 6 / Rock Tunnel / Lavender Town stretch is covered by
//  the combined Kanto map asset (FRO-176), so this is a single A*-pathed
//  walk via navigate_to(), the same shape as B1a's Route 2 -> Pewter City
//  walk.
//
//  NOTE: Rock Tunnel is pitch black in-game without Flash (HM05 -- not
//  obtained until Pokemon Tower / later in the story per this plan's phase
//  order). The combined-map navigator positions the player via template
//  matching the overworld screen, not by reading in-game light state, so a
//  dark tunnel is not expected to block matching the way it would block a
//  human player's vision -- but this has NOT been hardware-verified. If
//  Rock Tunnel's interior turns out to need its own captured/verified path
//  (like Viridian Forest in B0_05), that is the first thing to revisit here.
void run_B4a_vermilion_to_lavender(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
);

}
}
}
#endif
