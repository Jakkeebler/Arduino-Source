/*  Pokemon FRLG Evolution Policy
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include "PokemonFRLG_EvolutionPolicy.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


const EnumDropdownDatabase<EvolutionPolicyType>& EvolutionPolicyType_Database(){
    static const EnumDropdownDatabase<EvolutionPolicyType> database({
        {EvolutionPolicyType::LevelUp, "level-up", "Level-Up (evolve automatically)"},
        {EvolutionPolicyType::Stone,   "stone",    "Stone (explicit \"Use Item\" action only)"},
        {EvolutionPolicyType::Trade,   "trade",    "Trade (unsupported -- flag and skip)"},
        {EvolutionPolicyType::Block,   "block",    "Block (never evolve)"},
    });
    return database;
}

const EnumDropdownDatabase<EvolutionStoneItem>& EvolutionStoneItem_Database(){
    static const EnumDropdownDatabase<EvolutionStoneItem> database({
        {EvolutionStoneItem::None,         "none",          "(none)"},
        {EvolutionStoneItem::FireStone,    "fire-stone",    "Fire Stone"},
        {EvolutionStoneItem::WaterStone,   "water-stone",   "Water Stone"},
        {EvolutionStoneItem::Thunderstone, "thunderstone",  "Thunderstone"},
        {EvolutionStoneItem::LeafStone,    "leaf-stone",    "Leaf Stone"},
        {EvolutionStoneItem::MoonStone,    "moon-stone",    "Moon Stone"},
    });
    return database;
}

std::string evolution_stone_item_name(EvolutionStoneItem item){
    switch (item){
    case EvolutionStoneItem::FireStone:    return "Fire Stone";
    case EvolutionStoneItem::WaterStone:   return "Water Stone";
    case EvolutionStoneItem::Thunderstone: return "Thunderstone";
    case EvolutionStoneItem::LeafStone:    return "Leaf Stone";
    case EvolutionStoneItem::MoonStone:    return "Moon Stone";
    case EvolutionStoneItem::None:
    default:
        return std::string();
    }
}

bool policy_blocks_automatic_evolution(EvolutionPolicyType type){
    switch (type){
    case EvolutionPolicyType::LevelUp:
        return false;
    case EvolutionPolicyType::Stone:
    case EvolutionPolicyType::Trade:
    case EvolutionPolicyType::Block:
        return true;
    }
    //  Unknown enum value: fail safe toward "never auto-evolve" rather than
    //  risking an unwanted evolution.
    return true;
}

bool policy_uses_explicit_stone_step(EvolutionPolicyType type){
    return type == EvolutionPolicyType::Stone;
}

bool should_stop_grind_before_blocked_evolution(
    EvolutionPolicyType type, int current_level, int evolution_level
){
    if (evolution_level <= 0){
        return false;   //  No level-up evolution to guard against.
    }
    if (type == EvolutionPolicyType::LevelUp || type == EvolutionPolicyType::Stone){
        return false;
    }
    //  Block / Trade: stop as soon as the NEXT level-up would reach (or, from
    //  a multi-level grind step, pass) the evolution level.
    return current_level + 1 >= evolution_level;
}


}
}
}
