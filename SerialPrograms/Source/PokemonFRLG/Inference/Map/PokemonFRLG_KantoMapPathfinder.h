/*  Kanto Map Pathfinder
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Grid A* over the auto-classified walkable mask of the combined Kanto map.
 *  Uses global tile coordinates that match what KantoMapDetector returns.
 */

#ifndef PokemonAutomation_PokemonFRLG_KantoMapPathfinder_H
#define PokemonAutomation_PokemonFRLG_KantoMapPathfinder_H

#include <optional>

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

enum class KantoStep{
    North,  //  joystick (0, +1)
    South,  //  joystick (0, -1)
    East,   //  joystick (+1, 0)
    West,   //  joystick (-1, 0)
};

const char* kanto_step_name(KantoStep s);

//  Compute the next step from start to goal on the combined Kanto map.
//  Returns nullopt if start is the same as goal, the start tile is blocked,
//  or no walkable path exists.
std::optional<KantoStep> kanto_pathfind_next_step(
    int start_tile_x, int start_tile_y,
    int goal_tile_x,  int goal_tile_y
);

//  Like kanto_pathfind_next_step(), but also reports how many steps at the head
//  of the route share that direction, capped at max_run. The navigator uses this
//  to walk a whole straight segment on one held stick input instead of taking a
//  position fix after every single tile.
std::optional<KantoStep> kanto_pathfind_next_run(
    int start_tile_x, int start_tile_y,
    int goal_tile_x,  int goal_tile_y,
    int max_run,
    int* run_length
);

bool kanto_tile_walkable(int tile_x, int tile_y);

//  Runtime map learning. Process-lifetime only -- nothing is persisted to disk.
//
//  Two distinct facts, which must not be conflated:
//
//    * kanto_mark_tile_walkable() -- we have STOOD on this tile, so it is
//      walkable whatever the generated mask says.
//    * kanto_mark_edge_blocked() -- we could not move in this direction FROM
//      this tile. That is a fact about the edge, not about the destination:
//      Gen 3 ledges are one-way, so failing to walk north out of a tile says
//      nothing about whether the tile to the north is walkable.
void kanto_mark_tile_walkable(int tile_x, int tile_y);
void kanto_mark_edge_blocked(int tile_x, int tile_y, KantoStep dir);
bool kanto_edge_blocked(int tile_x, int tile_y, KantoStep dir);
void kanto_clear_learned();
size_t kanto_learned_edge_count();

}
}
}
#endif
