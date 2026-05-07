/*  Pokemon FRLG Grind / Heal Locations
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Registry of "grass spots to grind in" and "Pokemon Centers to heal at"
 *  for the XP Grinder. Each entry resolves to a KantoGoal tile that the
 *  map-driven navigator (`kanto_navigate_to`) can path to.
 *
 *  Adding a new location:
 *    1. Capture the in-game map for the new region and stitch it into
 *       Resources/PokemonFRLG/Maps/Kanto-Combined.png.
 *    2. Re-run classify_full_kanto.py to regenerate KantoMapMasks.
 *    3. Add a new KantoGoal constant in PokemonFRLG_KantoMapNavigator.h.
 *    4. Add a new enum value here + a row in the lookup tables.
 *    5. Rebuild — the new location auto-appears in the XPGrinder dropdowns.
 */

#ifndef PokemonAutomation_PokemonFRLG_GrindHealLocations_H
#define PokemonAutomation_PokemonFRLG_GrindHealLocations_H

#include "Common/Cpp/Options/EnumDropdownOption.h"
#include "PokemonFRLG/PokemonFRLG_Navigation.h"  //  for KantoFlyLocation

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

struct KantoGoal;


//  Grass spots the XP Grinder can spin in. Order is stable for serialization.
enum class GrindLocationId{
    Route1NorthGrass = 0,
    //  Add new entries below as their map-region masks get added to
    //  KantoMapMasks_Generated.h. Each new value also needs a row in
    //  goal_for_grind_location() and the dropdown database.
    //  Route22Grass,
    //  Route2SouthGrass,
    //  ViridianForestCentralGrass,
    //  ...
};

const EnumDropdownDatabase<GrindLocationId>& GrindLocationId_Database();

//  Returns the on-map tile the navigator should aim for when grinding at
//  this location.
const KantoGoal& goal_for_grind_location(GrindLocationId id);


//  Pokemon Centers the XP Grinder can heal at. Order is stable.
enum class HealLocationId{
    ViridianCity = 0,
    //  Add new PCs below once their entrance tiles are mapped + their fly
    //  destinations are confirmed.
    //  PalletTown,
    //  PewterCity,
    //  CeruleanCity,
    //  ...
};

const EnumDropdownDatabase<HealLocationId>& HealLocationId_Database();

//  Returns the tile in front of the PC's door (where kanto_navigate_to should
//  land before enter_pokecenter is called).
const KantoGoal& pc_entrance_for_heal_location(HealLocationId id);

//  The Fly map destination for this PC. Used by Travel Method = Fly.
KantoFlyLocation fly_for_heal_location(HealLocationId id);


}
}
}
#endif
