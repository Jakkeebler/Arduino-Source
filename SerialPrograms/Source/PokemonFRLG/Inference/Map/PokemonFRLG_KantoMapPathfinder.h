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

bool kanto_tile_walkable(int tile_x, int tile_y);

//  Runtime obstacle learning.
//
//  The generated walkable mask has systematic defects. Rather than requiring a
//  hand-written override for every one, the navigator calls
//  kanto_mark_tile_blocked() when it has proved the player cannot walk into a
//  tile, and A* routes around it for the rest of the process's life.
//  Process-lifetime only -- nothing is persisted to disk.
void kanto_mark_tile_blocked(int tile_x, int tile_y);
void kanto_clear_learned_blocks();
size_t kanto_learned_block_count();

}
}
}
#endif
