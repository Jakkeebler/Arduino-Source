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
    //  The second field is persisted into user configs -- never rename one.
    static const EnumDropdownDatabase<GrindLocationId> database({
        {GrindLocationId::Route1NorthGrass, "route_1_north_grass", "Route 1 — North Grass (Viridian PC)"},
        {GrindLocationId::Route1SouthGrass, "route_1_south_grass", "Route 1 — South Grass (Viridian PC)"},
        {GrindLocationId::Route22EastGrass, "route_22_east_grass", "Route 22 — East Grass (Viridian PC)"},
        {GrindLocationId::Route22WestGrass, "route_22_west_grass", "Route 22 — West Grass (Viridian PC)"},
        {GrindLocationId::Route2SouthGrass, "route_2_south_grass", "Route 2 — South Grass (Viridian PC)"},
        {GrindLocationId::Route2NorthGrass, "route_2_north_grass", "Route 2 — North Grass (Pewter PC)"},
        {GrindLocationId::Route3WestGrass,  "route_3_west_grass",  "Route 3 — West Grass (Pewter PC)"},
        {GrindLocationId::Route3EastGrass,  "route_3_east_grass",  "Route 3 — East Grass (Route 4 PC)"},
        {GrindLocationId::Route4Grass,      "route_4_grass",       "Route 4 — Grass (Cerulean PC)"},
        {GrindLocationId::Route9Grass,      "route_9_grass",       "Route 9 — Grass (Cerulean PC)"},
        {GrindLocationId::Route10Grass,     "route_10_grass",      "Route 10 — Grass (Route 10 PC)"},
        {GrindLocationId::Route6Grass,      "route_6_grass",       "Route 6 — Grass (Vermilion PC)"},
        {GrindLocationId::Route7Grass,      "route_7_grass",       "Route 7 — Grass (Celadon PC)"},
        {GrindLocationId::Route8Grass,      "route_8_grass",       "Route 8 — Grass (Lavender PC)"},
        {GrindLocationId::Route16Grass,     "route_16_grass",      "Route 16 — Grass (Celadon PC)"},
    });
    return database;
}
const KantoGoal& goal_for_grind_location(GrindLocationId id){
    switch (id){
    case GrindLocationId::Route1NorthGrass: return KantoGoals::Route1NorthGrass;
    case GrindLocationId::Route1SouthGrass: return KantoGoals::Route1SouthGrass;
    case GrindLocationId::Route22EastGrass: return KantoGoals::Route22EastGrass;
    case GrindLocationId::Route22WestGrass: return KantoGoals::Route22WestGrass;
    case GrindLocationId::Route2SouthGrass: return KantoGoals::Route2SouthGrass;
    case GrindLocationId::Route2NorthGrass: return KantoGoals::Route2NorthGrass;
    case GrindLocationId::Route3WestGrass:  return KantoGoals::Route3WestGrass;
    case GrindLocationId::Route3EastGrass:  return KantoGoals::Route3EastGrass;
    case GrindLocationId::Route4Grass:      return KantoGoals::Route4Grass;
    case GrindLocationId::Route9Grass:      return KantoGoals::Route9Grass;
    case GrindLocationId::Route10Grass:     return KantoGoals::Route10Grass;
    case GrindLocationId::Route6Grass:      return KantoGoals::Route6Grass;
    case GrindLocationId::Route7Grass:      return KantoGoals::Route7Grass;
    case GrindLocationId::Route8Grass:      return KantoGoals::Route8Grass;
    case GrindLocationId::Route16Grass:     return KantoGoals::Route16Grass;
    }
    throw InternalProgramError(nullptr, PA_CURRENT_FUNCTION, "Unknown GrindLocationId.");
}


const EnumDropdownDatabase<HealLocationId>& HealLocationId_Database(){
    static const EnumDropdownDatabase<HealLocationId> database({
        {HealLocationId::ViridianCity,   "viridian_city",   "Viridian City Pokémon Center"},
        {HealLocationId::PewterCity,     "pewter_city",     "Pewter City Pokémon Center"},
        {HealLocationId::Route4,         "route_4",         "Route 4 Pokémon Center (Mt. Moon)"},
        {HealLocationId::CeruleanCity,   "cerulean_city",   "Cerulean City Pokémon Center"},
        {HealLocationId::Route10,        "route_10",        "Route 10 Pokémon Center (Rock Tunnel)"},
        {HealLocationId::CeladonCity,    "celadon_city",    "Celadon City Pokémon Center"},
        {HealLocationId::LavenderTown,   "lavender_town",   "Lavender Town Pokémon Center"},
        {HealLocationId::SaffronCity,    "saffron_city",    "Saffron City Pokémon Center"},
        {HealLocationId::VermilionCity,  "vermilion_city",  "Vermilion City Pokémon Center"},
        {HealLocationId::FuchsiaCity,    "fuchsia_city",    "Fuchsia City Pokémon Center"},
        {HealLocationId::CinnabarIsland, "cinnabar_island", "Cinnabar Island Pokémon Center"},
    });
    return database;
}
const KantoGoal& pc_entrance_for_heal_location(HealLocationId id){
    switch (id){
    case HealLocationId::ViridianCity:   return KantoGoals::ViridianPokeCenterEntrance;
    case HealLocationId::PewterCity:     return KantoGoals::PewterPokeCenterEntrance;
    case HealLocationId::Route4:         return KantoGoals::Route4PokeCenterEntrance;
    case HealLocationId::CeruleanCity:   return KantoGoals::CeruleanPokeCenterEntrance;
    case HealLocationId::Route10:        return KantoGoals::Route10PokeCenterEntrance;
    case HealLocationId::CeladonCity:    return KantoGoals::CeladonPokeCenterEntrance;
    case HealLocationId::LavenderTown:   return KantoGoals::LavenderPokeCenterEntrance;
    case HealLocationId::SaffronCity:    return KantoGoals::SaffronPokeCenterEntrance;
    case HealLocationId::VermilionCity:  return KantoGoals::VermilionPokeCenterEntrance;
    case HealLocationId::FuchsiaCity:    return KantoGoals::FuchsiaPokeCenterEntrance;
    case HealLocationId::CinnabarIsland: return KantoGoals::CinnabarPokeCenterEntrance;
    }
    throw InternalProgramError(nullptr, PA_CURRENT_FUNCTION, "Unknown HealLocationId.");
}
KantoFlyLocation fly_for_heal_location(HealLocationId id){
    switch (id){
    case HealLocationId::ViridianCity:   return KantoFlyLocation::viridiancity;
    case HealLocationId::PewterCity:     return KantoFlyLocation::pewtercity;
    case HealLocationId::Route4:         return KantoFlyLocation::route4;
    case HealLocationId::CeruleanCity:   return KantoFlyLocation::ceruleancity;
    case HealLocationId::Route10:        return KantoFlyLocation::route10;
    case HealLocationId::CeladonCity:    return KantoFlyLocation::celadoncity;
    case HealLocationId::LavenderTown:   return KantoFlyLocation::lavendertown;
    case HealLocationId::SaffronCity:    return KantoFlyLocation::saffroncity;
    case HealLocationId::VermilionCity:  return KantoFlyLocation::vermilioncity;
    case HealLocationId::FuchsiaCity:    return KantoFlyLocation::fuschiacity;
    case HealLocationId::CinnabarIsland: return KantoFlyLocation::cinnabarisland;
    }
    throw InternalProgramError(nullptr, PA_CURRENT_FUNCTION, "Unknown HealLocationId.");
}

HealLocationId nearest_heal_location(GrindLocationId id){
    //  Hand-assigned rather than computed: straight-line distance picks the
    //  wrong center where mountains or water sit between the two, so each
    //  grind spot names the center you would actually walk or fly back to.
    switch (id){
    case GrindLocationId::Route1NorthGrass:
    case GrindLocationId::Route1SouthGrass:
    case GrindLocationId::Route22EastGrass:
    case GrindLocationId::Route22WestGrass:
    case GrindLocationId::Route2SouthGrass: return HealLocationId::ViridianCity;
    case GrindLocationId::Route2NorthGrass:
    case GrindLocationId::Route3WestGrass:  return HealLocationId::PewterCity;
    case GrindLocationId::Route3EastGrass:  return HealLocationId::Route4;
    case GrindLocationId::Route4Grass:
    case GrindLocationId::Route9Grass:      return HealLocationId::CeruleanCity;
    case GrindLocationId::Route10Grass:     return HealLocationId::Route10;
    case GrindLocationId::Route6Grass:      return HealLocationId::VermilionCity;
    case GrindLocationId::Route7Grass:
    case GrindLocationId::Route16Grass:     return HealLocationId::CeladonCity;
    case GrindLocationId::Route8Grass:      return HealLocationId::LavenderTown;
    }
    throw InternalProgramError(nullptr, PA_CURRENT_FUNCTION, "Unknown GrindLocationId.");
}


}
}
}
