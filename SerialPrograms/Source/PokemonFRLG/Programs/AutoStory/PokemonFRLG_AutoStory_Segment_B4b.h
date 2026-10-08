/*  Pokemon FRLG AutoStory - Segment B4b
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Navigate from Lavender Town through Route 7 to Celadon City, defeat
 *  Erika at the Celadon Gym, and win the Rainbow Badge (+ TM19 Giga Drain).
 */

#ifndef PokemonAutomation_PokemonFRLG_AutoStory_Segment_B4b_H
#define PokemonAutomation_PokemonFRLG_AutoStory_Segment_B4b_H

#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "PokemonFRLG/Programs/AutoStory/PokemonFRLG_AutoStoryTools.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

//  Start: in Lavender Town, heading for Celadon City (B4a's end state).
//  End:   Rainbow Badge won, standing outside the Celadon Gym
//         (KantoGoals::Extended::CeladonCityGymDoor).
//
//  Scope note (see Segment_B4a.h for the full explanation): this segment
//  implements the Phase::B4b slot already registered in
//  PokemonFRLG_AutoStory.cpp, which is "Lavender Town -> Celadon Gym ->
//  Rainbow Badge won". The FRO-183 issue text additionally asks for:
//    - Celadon Dept Store shopping / Game Corner (explicitly "optional" in
//      the issue, and not mentioned by the registered end_text) -- left
//      unimplemented for this first pass. The department-store and game
//      corner doors already exist as navigable goals
//      (KantoGoals::Extended::CeladonCityDepartmentStore1fDoor1/2,
//      CeladonCityGameCornerDoor), so a later milestone can add an actual
//      shopping/slots routine on top of this segment without re-deriving
//      navigation.
//    - Strength HM04 -- the registered phase table assigns HM acquisition
//      (Surf/Strength) to Phase::B6a ("Safari Zone -> HM acquisition"), not
//      B4b. Giving Strength here would duplicate/pre-empt that phase, so it
//      is intentionally left out; see PokemonFRLG_AutoStory.cpp's Phase::B6a
//      entry.
//
//  TM19 Giga Drain IS in scope here -- Erika hands it over as part of the
//  same post-battle dialog chain as the Rainbow Badge, so there is no way
//  to separate the two without re-deriving the exact dialog chain twice.
void run_B4b_lavender_to_celadon_erika(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats
);

}
}
}
#endif
