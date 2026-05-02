/*  Kanto Map Navigator
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  A*-driven navigation across the combined Kanto map (Viridian + Route 1
 *  + Pallet stitched into one image). Tile coordinates are GLOBAL on the
 *  combined map. Handles encounters (flee + retry) and door-fade success.
 */

#ifndef PokemonAutomation_PokemonFRLG_KantoMapNavigator_H
#define PokemonAutomation_PokemonFRLG_KantoMapNavigator_H

#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

struct KantoGoal{
    int tile_x;
    int tile_y;
    int tolerance = 1;
};

//  Pre-defined goals on the full Kanto map.
//  Sub-region tile boundaries (approximate, used only for log labels):
//      Viridian: rows 180..215, cols 50..100
//      Route 1:  rows 215..260, cols 55..80
//      Pallet:   rows 260..285, cols 55..80
namespace KantoGoals{
    //  Tile in front of the Viridian Pokemon Center door.
    //  PC building footprint: cols 72-76, rows 202-206. Door tiles at
    //  cols 73 and 75 of row 205. We target (74, 207) - the courtyard tile
    //  directly south of the PC's center. Tolerance 0: must reach the EXACT
    //  tile so the downstream enter_pokecenter walks straight into the door
    //  rather than bumping into a wall column.
    constexpr KantoGoal ViridianPokeCenterEntrance{74, 207, 0};

    //  First walkable grass tile at the top of Route 1, where the player
    //  typically grass-spins. Tolerance 1 since exact starting position varies.
    constexpr KantoGoal Route1NorthGrassCorner{62, 222, 1};
}


//  Navigate from current location to goal. Returns when goal reached or a
//  door fade fires. Throws on too many encounters / unknowns / step budget.
void kanto_navigate_to(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const KantoGoal& goal,
    int max_steps = 200
);


}
}
}
#endif
