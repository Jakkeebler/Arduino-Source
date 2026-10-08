/*  Pokemon FRLG AutoStory - Segment B8b
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Route 20/Viridian City -> Viridian Gym -> Giovanni -> Earth Badge
 *  (all 8 badges obtained) + TM27 (Fissure).
 */

#ifndef PokemonAutomation_PokemonFRLG_AutoStory_Segment_B8b_H
#define PokemonAutomation_PokemonFRLG_AutoStory_Segment_B8b_H

#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "PokemonFRLG/Programs/AutoStory/PokemonFRLG_AutoStoryTools.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

//  Start: on Route 20/Viridian City, heading for the Viridian Gym.
//  End:   Earth Badge won, TM27 (Fissure) received. Outside the Viridian
//         Gym -- all 8 badges obtained.
//
//  NOTE: best-effort first pass, not hardware-verified. Like Koga's gym in
//  B5c, the Viridian Gym floor in FRLG is a hidden-warp-tile maze (step on
//  the wrong floor tile and get teleported back to the entrance) that must
//  be walked via a memorized tile path; no such path has been captured
//  against real hardware for this project yet. The placeholder below just
//  walks forward repeatedly -- bumping a wrong warp tile just relocates the
//  player rather than soft-locking, so this is a correctness gap, not a
//  crash risk. Replace with a captured step sequence before relying on
//  this in a real run.
void run_B8b_viridian_gym(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
);

}
}
}
#endif
