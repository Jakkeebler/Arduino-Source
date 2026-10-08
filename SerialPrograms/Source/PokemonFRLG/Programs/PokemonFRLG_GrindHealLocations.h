/*  Pokemon FRLG Grind / Heal Locations
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Registry of "grass spots to grind in" and "Pokemon Centers to heal at"
 *  for the XP Grinder. Each entry resolves to a KantoGoal tile that the
 *  map-driven navigator (`kanto_navigate_to`) can path to.
 *
 *  The tiles behind these entries were derived from the combined Kanto map
 *  image rather than measured by hand: Pokemon Centers by template-matching
 *  the shared PC building sprite, grass spots by matching the tall-grass tile
 *  and picking the most enclosed walkable tile of each patch. See the comments
 *  in PokemonFRLG_KantoMapNavigator.h.
 *
 *  Adding a new location:
 *    1. Add a KantoGoal constant in PokemonFRLG_KantoMapNavigator.h.
 *    2. Add an enum value here + a row in the lookup tables in the .cpp.
 *    3. Rebuild - the new location auto-appears in the XPGrinder dropdowns.
 */

#ifndef PokemonAutomation_PokemonFRLG_GrindHealLocations_H
#define PokemonAutomation_PokemonFRLG_GrindHealLocations_H

#include "Common/Cpp/Options/EnumDropdownOption.h"
#include "PokemonFRLG/PokemonFRLG_Navigation.h"  //  for KantoFlyLocation

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

struct KantoGoal;


//  Grass spots the XP Grinder can spin in. Order is stable for serialization
//  -- the internal id strings in the dropdown database are what get persisted,
//  so never rename those; appending new values is safe.
//
//  Roughly ordered by where you reach them in a normal playthrough.
enum class GrindLocationId{
    Route1NorthGrass = 0,
    Route1SouthGrass,
    Route22EastGrass,
    Route22WestGrass,
    Route2SouthGrass,
    Route2NorthGrass,
    Route3WestGrass,
    Route3EastGrass,
    Route4Grass,
    Route9Grass,
    Route10Grass,
    Route6Grass,
    Route7Grass,
    Route8Grass,
    Route16Grass,
};

const EnumDropdownDatabase<GrindLocationId>& GrindLocationId_Database();

//  Returns the on-map tile the navigator should aim for when grinding at
//  this location.
const KantoGoal& goal_for_grind_location(GrindLocationId id);


//  Pokemon Centers the XP Grinder can heal at. Order is stable.
//
//  Pallet Town has no Pokemon Center. Indigo Plateau's is inside the League
//  building rather than a standalone one, so it is not navigable the same way
//  and is omitted.
enum class HealLocationId{
    ViridianCity = 0,
    PewterCity,
    Route4,
    CeruleanCity,
    Route10,
    CeladonCity,
    LavenderTown,
    SaffronCity,
    VermilionCity,
    FuchsiaCity,
    CinnabarIsland,
};

const EnumDropdownDatabase<HealLocationId>& HealLocationId_Database();

//  Returns the tile in front of the PC's door (where kanto_navigate_to should
//  land before enter_pokecenter is called).
const KantoGoal& pc_entrance_for_heal_location(HealLocationId id);

//  The Fly map destination for this PC. Used by Travel Method = Fly.
KantoFlyLocation fly_for_heal_location(HealLocationId id);

//  The Pokemon Center closest to a grind spot, used as the default heal
//  target so the user only has to pick where to grind. "Closest" is by
//  straight-line tile distance, not walking distance.
HealLocationId nearest_heal_location(GrindLocationId id);


}
}
}
#endif
