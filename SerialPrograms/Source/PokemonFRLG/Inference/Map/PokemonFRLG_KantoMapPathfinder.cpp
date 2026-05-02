/*  Kanto Map Pathfinder
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include <cstdint>
#include <cstdlib>
#include <queue>
#include <unordered_map>
#include <vector>
#include "PokemonFRLG_KantoMapMasks_Generated.h"
#include "PokemonFRLG_KantoMapPathfinder.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

const char* kanto_step_name(KantoStep s){
    switch (s){
    case KantoStep::North: return "north";
    case KantoStep::South: return "south";
    case KantoStep::East:  return "east";
    case KantoStep::West:  return "west";
    }
    return "?";
}

namespace{

constexpr uint8_t WALKABLE_CODE = 0;

bool walkable(int x, int y){
    if (x < 0 || y < 0 || x >= KANTO_MASK_COLS || y >= KANTO_MASK_ROWS){
        return false;
    }
    return KANTO_MASK[y][x] == WALKABLE_CODE;
}

inline uint32_t key(int x, int y){
    return (uint32_t)(x & 0xFFFF) | ((uint32_t)(y & 0xFFFF) << 16);
}

}  // namespace


bool kanto_tile_walkable(int tile_x, int tile_y){
    return walkable(tile_x, tile_y);
}


std::optional<KantoStep> kanto_pathfind_next_step(
    int start_x, int start_y,
    int goal_x,  int goal_y
){
    if (start_x == goal_x && start_y == goal_y){
        return std::nullopt;
    }
    if (!walkable(start_x, start_y)){
        return std::nullopt;
    }

    struct Node{
        int x, y;
        int g, f;
    };
    auto cmp = [](const Node& a, const Node& b){ return a.f > b.f; };
    std::priority_queue<Node, std::vector<Node>, decltype(cmp)> open(cmp);

    auto manhattan = [&](int x, int y){
        return std::abs(x - goal_x) + std::abs(y - goal_y);
    };

    open.push({start_x, start_y, 0, manhattan(start_x, start_y)});

    struct Came{
        int px, py;
        KantoStep step;
    };
    std::unordered_map<uint32_t, Came> came_from;
    std::unordered_map<uint32_t, int> best_g;
    best_g[key(start_x, start_y)] = 0;

    const struct{ int dx, dy; KantoStep step; } NEIGHBORS[4] = {
        { 0, -1, KantoStep::North},
        { 0, +1, KantoStep::South},
        {+1,  0, KantoStep::East},
        {-1,  0, KantoStep::West},
    };

    bool found = false;
    while (!open.empty()){
        Node cur = open.top();
        open.pop();
        if (cur.x == goal_x && cur.y == goal_y){
            found = true;
            break;
        }
        auto it = best_g.find(key(cur.x, cur.y));
        if (it != best_g.end() && cur.g > it->second){
            continue;
        }
        for (const auto& n : NEIGHBORS){
            int nx = cur.x + n.dx;
            int ny = cur.y + n.dy;
            if (!walkable(nx, ny)){
                if (!(nx == goal_x && ny == goal_y)){
                    continue;
                }
            }
            int new_g = cur.g + 1;
            uint32_t k = key(nx, ny);
            auto bg = best_g.find(k);
            if (bg == best_g.end() || new_g < bg->second){
                best_g[k] = new_g;
                came_from[k] = {cur.x, cur.y, n.step};
                open.push({nx, ny, new_g, new_g + manhattan(nx, ny)});
            }
        }
    }

    if (!found) return std::nullopt;

    int cx = goal_x, cy = goal_y;
    KantoStep first_step = KantoStep::North;
    while (true){
        auto it = came_from.find(key(cx, cy));
        if (it == came_from.end()) return std::nullopt;
        if (it->second.px == start_x && it->second.py == start_y){
            first_step = it->second.step;
            break;
        }
        cx = it->second.px;
        cy = it->second.py;
    }
    return first_step;
}


}
}
}
