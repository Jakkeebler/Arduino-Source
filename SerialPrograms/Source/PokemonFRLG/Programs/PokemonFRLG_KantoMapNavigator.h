/*  Kanto Map Navigator
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  A*-driven navigation across the combined Kanto map: every outdoor city and
 *  route, plus key interior dungeons (Viridian Forest, Mt. Moon, Diglett's
 *  Cave, Rock Tunnel, the Underground Paths, Cerulean Cave) and the walled-off
 *  Saffron City, all stitched/packed into one 408x400 tile image. Tile
 *  coordinates are GLOBAL on the combined map. Handles encounters (flee +
 *  retry) and door-fade success.
 *
 *  See PokemonFRLG_KantoGoals_Extended.h for the full generated set of every
 *  building/cave/gym/pokecenter door and every Fly destination.
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

    //  ---- Phase B5 (Soul Badge) building doors ----
    //
    //  UNVERIFIED / ESTIMATED: unlike the Pokemon-Center/grass goals above
    //  (which were derived by template-matching their sprite across
    //  Kanto-Combined.png and confirmed walkable against KANTO_MASK), these
    //  four were placed by relative position to their already-verified
    //  neighboring PokeCenter anchor, because the interior-dungeon sprite
    //  set documented at the top of this file (Pokemon Tower, the Celadon
    //  Game Corner / Rocket Hideout, Fuchsia Gym, Safari Zone) hasn't been
    //  template-matched yet. Before relying on these for a real run, verify
    //  (and correct) them with Scripts/PokemonFRLG/kanto_zoom_crop.py (crop +
    //  grid the artwork) and Scripts/PokemonFRLG/kanto_reachability.py
    //  (confirm the goal tile is actually walkable / reachable).
    constexpr KantoGoal PokemonTowerEntrance        {383, 136, 0};
    constexpr KantoGoal CeladonGameCornerEntrance   {224, 132, 0};
    constexpr KantoGoal FuchsiaGymEntrance          {217, 340, 0};
    constexpr KantoGoal FuchsiaSafariZoneEntrance   {236, 332, 0};

    //  ---- Phase B6 (Marsh Badge) building doors ----
    //
    //  UNVERIFIED / ESTIMATED, same caveat as the Phase B5 doors immediately
    //  above: Silph Co. and the Saffron Gym are both interior-dungeon
    //  sprites that haven't been template-matched against
    //  Kanto-Combined.png yet, so these were placed by small relative
    //  offset from the already-verified SaffronPokeCenterEntrance anchor
    //  (Saffron City's layout puts both buildings near the city center,
    //  within a block or two of the Pokemon Center). Before relying on
    //  these for a real run, verify (and correct) them with
    //  Scripts/PokemonFRLG/kanto_zoom_crop.py (crop + grid the artwork) and
    //  Scripts/PokemonFRLG/kanto_reachability.py (confirm the goal tile is
    //  actually walkable / reachable).
    constexpr KantoGoal SilphCoEntrance             {268, 142, 0};
    constexpr KantoGoal SaffronGymEntrance          {290, 162, 0};

    //  ---- Phase B1 (Boulder Badge) building doors ----
    //
    //  Derived directly from the generated warp_events door tile
    //  (KantoGoals::Extended::PewterCityGymDoor, {63, 76}) using the same
    //  "entrance = (door_x, door_y + 1)" convention documented above for the
    //  Pokemon Center entrances: one tile south of the door, so navigate_to()
    //  stops in front of it instead of walking onto the warp tile and
    //  triggering the fade mid-navigation. Tolerance 0 for the same reason as
    //  the Pokemon Centers -- enter_gym() walks straight north into the door.
    constexpr KantoGoal PewterGymEntrance           { 63,  77, 0};
}


//  Navigate from current location to goal. Returns when goal reached or a
//  door fade fires. Throws on too many encounters / unknowns / step budget.
void kanto_navigate_to(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const KantoGoal& goal,
    int max_steps = 200
);

//  Same, but seeded with where the caller believes the player already is.
//
//  The first fix of a navigation run is otherwise a *cold* full-map match, and a
//  cold match is the only kind subject to the detector's ambiguity gate. In
//  repetitive terrain -- a field of identical tall-grass tiles, say -- that gate
//  correctly refuses to guess, so the run stalls before it takes a single step.
//  A hinted match is both far cheaper and not ambiguity-gated.
//
//  Pass the goal the program last navigated to, or the tile it has been standing
//  on. A wrong hint is self-correcting: the motion gate rejects detections that
//  disagree with it and drops to a cold fix after MAX_REJECTED_JUMPS.
void kanto_navigate_to(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const KantoGoal& goal,
    const KantoGoal& start_hint,
    int max_steps = 200
);


}
}
}
#endif
