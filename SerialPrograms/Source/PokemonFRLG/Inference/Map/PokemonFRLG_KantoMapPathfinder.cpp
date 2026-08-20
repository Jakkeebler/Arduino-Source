/*  Kanto Map Pathfinder
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <queue>
#include <set>
#include <unordered_map>
#include <vector>
#include "PokemonFRLG_KantoLedges_Generated.h"
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

//  Hand-verified corrections layered on top of the generated mask.
//
//  KANTO_MASK is derived from the map image, so it gets building interiors right
//  and building *doorways* wrong: the generator sees roof/wall pixels and marks
//  the whole footprint blocked, including the tile the game stands you on when
//  you walk out of the door. A* refuses to plan from a blocked start tile (see
//  kanto_pathfind_next_step below), so a single wrong cell under a door makes
//  every route that begins at that door unplannable.
//
//  These live here rather than in the generated header on purpose: that file is
//  regenerated wholesale from the map image and would silently drop hand edits.
//
//  Each entry must be justified against the map image or against observed
//  behaviour -- never against a guess about what "looks" walkable.
//
//  Rectangles are inclusive on all four sides. A single tile is a 1x1 rect.
struct MaskOverride{
    int x0;
    int y0;
    int x1;
    int y1;
    bool walkable;
};
constexpr MaskOverride MASK_OVERRIDES[] = {
    //  --- Viridian City Poke Center doorway -------------------------------
    //  The blue door tile of the Poke Center, verified against
    //  Resources/PokemonFRLG/Maps/Kanto-Combined.png: the building occupies
    //  x=72..76, y=204..206 and its door is the tile at (74,206).
    //  leave_pokecenter() stands the player exactly here, and the generator
    //  marked the whole footprint blocked -- so A* could not plan from the tile
    //  the player was standing on, failed on every post-heal walk back, and
    //  handed the run to the greedy fallback.
    //  The goal one square south, ViridianPokeCenterEntrance{74,207}, was
    //  already correct.
    {74, 206, 74, 206, true},

    //  --- Route 1 north tree corridor -------------------------------------
    //  Rows 215..221 are a solid tree wall either side of a four-tile sand
    //  path. Verified against the map image: the path is EXACTLY x=70..73, with
    //  trees at x=66..69 to the west and x=74..82 to the east, canopy tops on
    //  215-216, body on 217-220 and trunks on 221.
    //
    //  The generator got rows 217-221 mostly right but marked rows 215 and 216
    //  entirely walkable -- it reads the pale highlight at the top of a tree
    //  sprite as open ground -- plus the inner-edge tiles (69,218), (74,218),
    //  (69,220) and (74,220) for the same reason.
    //
    //  Consequence, observed three times on 2026-08-18: A* treated row 215/216
    //  as an open highway and routed south down x=74 straight into a tree,
    //  where the player pressed south eight times without moving and the run
    //  died. The correct route is the one the outbound trip used unprompted --
    //  down x=73.
    //
    //  These are stated as the full tree block rather than as a diff against
    //  the generated values: most cells already agree, and spelling out the
    //  real geometry is what makes the entry checkable later.
    {66, 215, 69, 221, false},
    {74, 215, 82, 221, false},

    //  --- Route 22 mountain, east block -----------------------------------
    //  The mountain immediately south of the Route 22 sand path. Verified
    //  against the map image: solid rock occupies x=46..54, y=200..208, with
    //  the walkable sand corridor running down x=42..45 to its west.
    //
    //  The generator marked only a scattering of the rock's edge tiles and left
    //  the entire interior walkable, which handed A* a phantom route straight
    //  through the mountain that happened to tie the real route on length. On
    //  2026-08-19 the tie broke toward the phantom: the player walked correctly
    //  from Viridian to (52,199), then pressed south into the rock face eight
    //  times and the run died before the first encounter.
    {46, 200, 54, 208, false},

    //  --- Route 22 tree column, east side of the grass ---------------------
    //  Two columns of pines standing between the Route 22 east grass and the
    //  sand path, x=40..41, y=198..203. Classic alternating-row defect: the
    //  generator marks the trunk rows blocked and reads the pale canopy rows as
    //  open ground.
    //
    //  This is the only way the mask thinks you can leave the grass, so on
    //  2026-08-19 the program walked in, went Out of PP, and then could not
    //  path back to the Poke Center: every route it planned ran east through
    //  these trees, and it pressed east into them until the run died. The real
    //  exit is south to y=204 and east along it -- 54 steps against the 48 the
    //  mask believes, and verified reachable by flood fill with this rect in
    //  place.
    //
    //  Stated as a rect rather than as learned edges because learning it one
    //  edge at a time costs six separate three-press failures and blows the
    //  per-trip learning budget before it finds the way round.
    {40, 198, 41, 203, false},
};

//  Runtime-learned map facts.
//
//  The generated mask has systematic defects -- mountain interiors marked
//  walkable, tree columns blocked only on alternating rows -- and hand-patching
//  each one as it is discovered does not scale to a 408x400 map that is only
//  partly explored. So the navigator records what it learns by actually walking,
//  and that outranks both tables.
//
//  There are two kinds of fact, and conflating them is a bug I already shipped
//  once:
//
//    * A tile we have STOOD on is walkable. Nothing else is as reliable.
//    * Being unable to move in a direction is a fact about that EDGE, not about
//      the destination tile. Gen 3 ledges are one-way -- you hop south over them
//      and cannot walk back north -- so "could not go north from here" says
//      nothing about whether the tile to the north is walkable. On 2026-08-19 the
//      navigator hopped a Route 22 ledge, failed to walk back up, and recorded
//      the tile it had just walked through as unwalkable.
//
//  Deliberately process-lifetime and not persisted: a wrong entry (an NPC that
//  happened to be standing there) costs one slightly longer route and is gone on
//  the next launch, whereas a persisted wrong entry would be a permanent hole in
//  the map with no obvious cause.
std::mutex g_learned_lock;
std::set<uint32_t> g_learned_walkable;
std::set<uint32_t> g_learned_edges;

inline uint32_t key(int x, int y){
    return (uint32_t)(x & 0xFFFF) | ((uint32_t)(y & 0xFFFF) << 16);
}
inline int step_index(KantoStep s){
    switch (s){
    case KantoStep::North: return 0;
    case KantoStep::South: return 1;
    case KantoStep::East:  return 2;
    case KantoStep::West:  return 3;
    }
    return 0;
}
inline uint32_t edge_key(int x, int y, KantoStep dir){
    return ((uint32_t)((y * KANTO_MASK_COLS) + x) << 2) | (uint32_t)step_index(dir);
}

bool walkable(int x, int y){
    if (x < 0 || y < 0 || x >= KANTO_MASK_COLS || y >= KANTO_MASK_ROWS){
        return false;
    }
    {
        std::lock_guard<std::mutex> lg(g_learned_lock);
        if (g_learned_walkable.find(key(x, y)) != g_learned_walkable.end()){
            return true;
        }
    }
    //  You cannot stand on a ledge -- you hop over it. Handled as a special
    //  move in the search below rather than as an ordinary walkable tile.
    if (kanto_is_ledge_south(x, y)){
        return false;
    }
    for (const MaskOverride& o : MASK_OVERRIDES){
        if (x >= o.x0 && x <= o.x1 && y >= o.y0 && y <= o.y1){
            return o.walkable;
        }
    }
    return KANTO_MASK[y][x] == WALKABLE_CODE;
}

}  // namespace


void kanto_mark_tile_walkable(int tile_x, int tile_y){
    std::lock_guard<std::mutex> lg(g_learned_lock);
    g_learned_walkable.insert(key(tile_x, tile_y));
}
void kanto_mark_edge_blocked(int tile_x, int tile_y, KantoStep dir){
    std::lock_guard<std::mutex> lg(g_learned_lock);
    g_learned_edges.insert(edge_key(tile_x, tile_y, dir));
}
bool kanto_edge_blocked(int tile_x, int tile_y, KantoStep dir){
    std::lock_guard<std::mutex> lg(g_learned_lock);
    return g_learned_edges.find(edge_key(tile_x, tile_y, dir)) != g_learned_edges.end();
}
void kanto_clear_learned(){
    std::lock_guard<std::mutex> lg(g_learned_lock);
    g_learned_walkable.clear();
    g_learned_edges.clear();
}
size_t kanto_learned_edge_count(){
    std::lock_guard<std::mutex> lg(g_learned_lock);
    return g_learned_edges.size();
}


bool kanto_is_ledge_south(int tile_x, int tile_y){
    if (tile_x < 0 || tile_y < 0 || tile_x >= KANTO_LEDGE_COLS){
        return false;
    }
    const uint32_t k = (uint32_t)(tile_y * KANTO_LEDGE_COLS + tile_x);
    return std::binary_search(
        KANTO_LEDGE_SOUTH,
        KANTO_LEDGE_SOUTH + KANTO_LEDGE_SOUTH_COUNT,
        k
    );
}

bool kanto_ledge_hop_target(int tile_x, int tile_y, int* landing_y){
    if (!kanto_is_ledge_south(tile_x, tile_y + 1)){
        return false;
    }
    if (!walkable(tile_x, tile_y + 2)){
        return false;
    }
    if (landing_y) *landing_y = tile_y + 2;
    return true;
}

bool kanto_tile_walkable(int tile_x, int tile_y){
    return walkable(tile_x, tile_y);
}


namespace{

//  The full route as a step list, empty when there is none.
//
//  Both public entry points go through this. The navigator needs more than the
//  first step: it walks several tiles between position fixes, and to do that it
//  has to know how far the route continues in one direction.
std::vector<KantoStep> pathfind_route(
    int start_x, int start_y,
    int goal_x,  int goal_y
){
    std::vector<KantoStep> route;
    if (start_x == goal_x && start_y == goal_y){
        return route;
    }
    if (!walkable(start_x, start_y)){
        return route;
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
            //  An edge we have already proved impassable stays impassable even
            //  when both tiles are walkable -- that is exactly what a ledge is.
            if (kanto_edge_blocked(cur.x, cur.y, n.step)){
                continue;
            }
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

        //  Ledge hop: one press of South carries the player over the ledge tile
        //  and onto the ground two rows down. This is the ONLY way through a
        //  ledge, and it only goes one way -- walking north from below lands on
        //  the ledge tile, which walkable() rejects, so the asymmetry falls out
        //  for free rather than needing a rule.
        int landing_y = 0;
        if (kanto_ledge_hop_target(cur.x, cur.y, &landing_y) &&
            !kanto_edge_blocked(cur.x, cur.y, KantoStep::South)
        ){
            const int nx = cur.x;
            const int ny = landing_y;
            const int new_g = cur.g + 1;
            const uint32_t k = key(nx, ny);
            auto bg = best_g.find(k);
            if (bg == best_g.end() || new_g < bg->second){
                best_g[k] = new_g;
                came_from[k] = {cur.x, cur.y, KantoStep::South};
                open.push({nx, ny, new_g, new_g + manhattan(nx, ny)});
            }
        }
    }

    if (!found) return route;

    //  Walk the parent chain back from the goal, then reverse.
    int cx = goal_x, cy = goal_y;
    while (!(cx == start_x && cy == start_y)){
        auto it = came_from.find(key(cx, cy));
        if (it == came_from.end()){
            route.clear();
            return route;
        }
        route.push_back(it->second.step);
        cx = it->second.px;
        cy = it->second.py;
    }
    std::reverse(route.begin(), route.end());
    return route;
}

}  // namespace


std::optional<KantoStep> kanto_pathfind_next_step(
    int start_x, int start_y,
    int goal_x,  int goal_y
){
    std::vector<KantoStep> route = pathfind_route(start_x, start_y, goal_x, goal_y);
    if (route.empty()){
        return std::nullopt;
    }
    return route.front();
}

std::optional<KantoStep> kanto_pathfind_next_run(
    int start_x, int start_y,
    int goal_x,  int goal_y,
    int max_run,
    int* run_length
){
    if (run_length) *run_length = 0;
    std::vector<KantoStep> route = pathfind_route(start_x, start_y, goal_x, goal_y);
    if (route.empty()){
        return std::nullopt;
    }
    const KantoStep first = route.front();
    int n = 1;
    while (n < (int)route.size() && n < max_run && route[(size_t)n] == first){
        n++;
    }
    if (run_length) *run_length = n;
    return first;
}


}
}
}
