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
    //  ---- Pokemon Center entrances ----
    //
    //  Every Pokemon Center in Kanto uses the same 5-tile-wide building sprite
    //  with the door in the middle column. These tiles were derived by template
    //  matching that sprite across Resources/PokemonFRLG/Maps/Kanto-Combined.png:
    //  the goal tile is always (door_x, door_y + 1), i.e. the walkable tile
    //  directly south of the door. All were checked against KANTO_MASK to
    //  confirm they are walkable.
    //
    //  Tolerance 0 throughout: enter_pokecenter walks straight north into the
    //  door, so landing one tile off would bump into a wall column instead.
    //
    //  Pallet Town has no Pokemon Center, and the Indigo Plateau center is
    //  inside the League building rather than a standalone sprite, so neither
    //  appears here.
    constexpr KantoGoal ViridianPokeCenterEntrance  { 74, 207, 0};
    constexpr KantoGoal PewterPokeCenterEntrance    { 65,  86, 0};
    constexpr KantoGoal Route4PokeCenterEntrance    {168,  56, 0};
    constexpr KantoGoal CeruleanPokeCenterEntrance  {286,  60, 0};
    constexpr KantoGoal Route10PokeCenterEntrance   {397,  71, 0};
    constexpr KantoGoal CeladonPokeCenterEntrance   {228, 132, 0};
    constexpr KantoGoal LavenderPokeCenterEntrance  {390, 136, 0};
    constexpr KantoGoal SaffronPokeCenterEntrance   {278, 152, 0};
    constexpr KantoGoal VermilionPokeCenterEntrance {279, 207, 0};
    constexpr KantoGoal FuchsiaPokeCenterEntrance   {217, 332, 0};
    constexpr KantoGoal CinnabarPokeCenterEntrance  { 74, 392, 0};

    //  ---- Encounter-grass spots ----
    //
    //  Derived by matching the tall-grass tile across the combined map,
    //  grouping the matches into connected patches, and picking the walkable
    //  tile with the most grass in its surrounding 5x5 window so that
    //  grass_spin has grass on every side. Tolerance 1 - any tile in the
    //  middle of the patch works.
    //
    //  NOTE: the old Route1NorthGrassCorner{62,222} is NOT an encounter-grass
    //  tile (it is plain path with decorative sprigs); Route1NorthGrass below
    //  replaces it and sits inside the actual grass.
    constexpr KantoGoal Route1NorthGrass  { 72, 228, 1};
    constexpr KantoGoal Route1SouthGrass  { 78, 235, 1};
    constexpr KantoGoal Route2NorthGrass  { 64, 104, 1};
    constexpr KantoGoal Route2SouthGrass  { 68, 158, 1};
    constexpr KantoGoal Route3WestGrass   {133,  83, 1};
    constexpr KantoGoal Route3EastGrass   {169,  84, 1};
    constexpr KantoGoal Route4Grass       {233,  63, 1};
    constexpr KantoGoal Route6Grass       {287, 176, 1};
    constexpr KantoGoal Route7Grass       {253, 134, 1};
    constexpr KantoGoal Route8Grass       {351, 142, 1};
    constexpr KantoGoal Route9Grass       {321,  53, 1};
    constexpr KantoGoal Route10Grass      {392,  58, 1};
    constexpr KantoGoal Route16Grass      {149, 163, 1};
    constexpr KantoGoal Route22EastGrass  { 36, 201, 1};
    constexpr KantoGoal Route22WestGrass  { 17, 201, 1};

    //  Retained so existing saved configs and any other callers keep building.
    //  Prefer Route1NorthGrass for grinding.
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
