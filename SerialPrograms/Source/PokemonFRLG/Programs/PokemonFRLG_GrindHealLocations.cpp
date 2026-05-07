/*  Pokemon FRLG Grind / Heal Locations
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include "Common/Cpp/Exceptions.h"
#include "PokemonFRLG/Programs/PokemonFRLG_KantoMapNavigator.h"
#include "PokemonFRLG_GrindHealLocations.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


const EnumDropdownDatabase<GrindLocationId>& GrindLocationId_Database(){
    static const EnumDropdownDatabase<GrindLocationId> database({
        {GrindLocationId::Route1NorthGrass, "route_1_north_grass", "Route 1 — North Grass"},
        //  When you add a new GrindLocationId, mirror the entry here.
    });
    return database;
}
const KantoGoal& goal_for_grind_location(GrindLocationId id){
    switch (id){
    case GrindLocationId::Route1NorthGrass:
        return KantoGoals::Route1NorthGrassCorner;
    }
    throw InternalProgramError(nullptr, PA_CURRENT_FUNCTION, "Unknown GrindLocationId.");
}


const EnumDropdownDatabase<HealLocationId>& HealLocationId_Database(){
    static const EnumDropdownDatabase<HealLocationId> database({
        {HealLocationId::ViridianCity, "viridian_city", "Viridian City Pokémon Center"},
        //  When you add a new HealLocationId, mirror the entry here.
    });
    return database;
}
const KantoGoal& pc_entrance_for_heal_location(HealLocationId id){
    switch (id){
    case HealLocationId::ViridianCity:
        return KantoGoals::ViridianPokeCenterEntrance;
    }
    throw InternalProgramError(nullptr, PA_CURRENT_FUNCTION, "Unknown HealLocationId.");
}
KantoFlyLocation fly_for_heal_location(HealLocationId id){
    switch (id){
    case HealLocationId::ViridianCity:
        return KantoFlyLocation::viridiancity;
    }
    throw InternalProgramError(nullptr, PA_CURRENT_FUNCTION, "Unknown HealLocationId.");
}


}
}
}
