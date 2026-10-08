/*  Pokemon FRLG AutoStory - Segment B8d
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Victory Road traversal (Strength boulder puzzles, Surf) -> Pokemon
 *  League gate, ready to enter the Elite Four.
 */

#ifndef PokemonAutomation_PokemonFRLG_AutoStory_Segment_B8d_H
#define PokemonAutomation_PokemonFRLG_AutoStory_Segment_B8d_H

#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "PokemonFRLG/Programs/AutoStory/PokemonFRLG_AutoStoryTools.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

//  Start: at the entrance to Victory Road.
//  End:   at the Pokemon League gate, ready to enter the Elite Four.
//
//  NOTE: best-effort first pass, not hardware-verified, and the highest-risk
//  segment in Phase B8. Victory Road is not part of the overworld A*
//  navigator's supported interior set (absent from the dungeon list in
//  PokemonFRLG_KantoMapNavigator.h's header comment), and this project has
//  no Strength-boulder-puzzle solver or boulder-push primitives yet (see
//  PokemonFRLG_AutoStoryTools.h -- no such helper exists). Only the 1F/2F
//  door tiles are known (KantoGoals::VictoryRoad1fDoor/2fDoor); the actual
//  boulder puzzles blocking progress on each floor are NOT solved here.
//  This walks a generally forward/north path per floor, fights off wild
//  encounters, and mashes through the Strength HM-use prompt once per
//  floor as a best guess -- it is expected to get stuck on a real cartridge
//  run. A follow-up ticket should add boulder-push primitives to
//  AutoStoryTools and a real captured route before this segment is relied
//  on unattended.
//
//  Also out of scope here: the Elite Four (Lorelei/Bruno/Agatha/Lance),
//  the Champion rival battle, and the Hall of Fame. Phase::B8d's own
//  registered end_text stops at "ready to enter the Elite Four" -- there
//  is no Phase enum value for anything past the League gate yet (see
//  PokemonFRLG_AutoStory.h). Extending the Phase enum and adding those
//  phases is separate follow-up work, not part of this segment.
void run_B8d_victory_road(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
);

}
}
}
#endif
