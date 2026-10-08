/*  Route Paths
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#ifndef PokemonAutomation_PokemonFRLG_RoutePaths_H
#define PokemonAutomation_PokemonFRLG_RoutePaths_H

#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

//  Walk from the Viridian City Pokemon Center exit to the Route 1 grass.
void walk_to_route1(SingleSwitchProgramEnvironment& env, ProControllerContext& context);

//  Walk from the Route 1 grass back to the Viridian City Pokemon Center entrance.
//  Inverse of walk_to_route1; assumes the same starting position walk_to_route1 ends at.
void walk_from_route1_to_pokecenter(SingleSwitchProgramEnvironment& env, ProControllerContext& context);

//  Walk from the Viridian City Pokemon Center exit to the Route 22 grass.
void walk_to_route22(SingleSwitchProgramEnvironment& env, ProControllerContext& context);

}
}
}
#endif
