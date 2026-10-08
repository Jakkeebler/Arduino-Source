/*  Pokemon FRLG AutoStory - Segment B0_00
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  New Game -> Oak Intro -> wake up in the player's bedroom.
 */

#ifndef PokemonAutomation_PokemonFRLG_AutoStory_Segment_B0_00_H
#define PokemonAutomation_PokemonFRLG_AutoStory_Segment_B0_00_H

#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "PokemonFRLG/Programs/AutoStory/PokemonFRLG_AutoStoryTools.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

//  Start: title screen on a brand new (never-played) save slot.
//  End:   in control, standing in the player's bedroom.
//
//  NOTE: this is the first AutoStory segment to automate the new-game
//  creation flow (title -> New Game -> gender select -> player name ->
//  rival name). No earlier program in this codebase does this -- every
//  other reset-based program (StarterRng, WildRng, etc.) assumes a save
//  already exists past this point. The sequence below is a best-effort
//  first pass built from documented FRLG menu structure and gated on
//  black-screen fades between the major scene transitions, not from
//  hardware-captured timings. Expect to retune delays after the first
//  real run, and confirm the "press START to accept the default name"
//  shortcut still behaves as expected on real hardware.
void run_B0_00_new_game_oak_intro(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
);

}
}
}
#endif
