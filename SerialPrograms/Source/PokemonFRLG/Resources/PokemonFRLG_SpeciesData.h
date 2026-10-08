/*  Pokemon FRLG Species Data
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Loads the FRLG species list (Pokedex 1-151 + a few extras) from
 *  Resources/PokemonFRLG/Data/Species.json. Display names come from the
 *  shared Pokemon::get_pokemon_name() database, not duplicated here.
 *
 */

#ifndef PokemonAutomation_PokemonFRLG_SpeciesData_H
#define PokemonAutomation_PokemonFRLG_SpeciesData_H

#include <cstdint>
#include <string>
#include <vector>

namespace PokemonAutomation{

class StringSelectDatabase;

namespace NintendoSwitch{
namespace PokemonFRLG{


struct SpeciesData{
    std::string slug;
    uint16_t dex_no = 0;
    //  Gen-3 typing: 1 or 2 entries ("fire", "flying", ...). Note these are
    //  the FRLG-era types, not modern ones -- Clefairy/Jigglypuff lines are
    //  pure Normal (no Fairy type in Gen 3).
    std::vector<std::string> types;
};


//  Look up by slug. Throws InternalProgramError if not in the database.
const SpeciesData& get_species(const std::string& slug);
//  Look up by slug. Returns nullptr if not in the database.
const SpeciesData* get_species_nothrow(const std::string& slug);

//  Look up by Pokedex number. Returns nullptr if no FRLG species has that dex#.
const SpeciesData* get_species_by_dex(uint16_t dex_no);

//  All species in dex order.
const std::vector<SpeciesData>& all_species();

//  StringSelectDatabase suitable for UI dropdowns. Display names come from
//  the shared Pokemon::get_pokemon_name() lookup.
const StringSelectDatabase& species_select_database();


}
}
}
#endif
