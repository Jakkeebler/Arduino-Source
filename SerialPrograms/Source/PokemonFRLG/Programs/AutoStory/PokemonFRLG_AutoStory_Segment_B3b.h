/*  Pokemon FRLG AutoStory - Segment B3b
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Board the S.S. Anne, fight the rival on deck, find the ship's medicine
 *  and deliver it to the captain, receive HM01 Cut, then disembark back
 *  into Vermilion City.
 */

#ifndef PokemonAutomation_PokemonFRLG_AutoStory_Segment_B3b_H
#define PokemonAutomation_PokemonFRLG_AutoStory_Segment_B3b_H

#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "PokemonFRLG/Programs/AutoStory/PokemonFRLG_AutoStoryTools.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

//  Start: standing at the S.S. Anne gangplank in Vermilion City (B3a's end
//         state), about to board.
//  End:   off the S.S. Anne with Cut taught, back in Vermilion City
//         (standing on/near the gangplank goal tile).
//
//  NOTE: best-effort / NOT hardware-verified. The S.S. Anne's interior
//  (deck, cabins, engine room) is not part of the combined Kanto map asset
//  -- same situation as Viridian Forest in B0_05 -- so this whole segment
//  is scripted joystick bursts + dialogue mashing with a forced rival
//  battle in the middle, rather than kanto_navigate_to() pathing. Retune
//  every step count and the "find the panacea, deliver it to the captain"
//  beat after the first real capture; the exact room layout/NPC position
//  for the medicine fetch is a known gap here.
void run_B3b_ssanne_traversal(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
);

}
}
}
#endif
