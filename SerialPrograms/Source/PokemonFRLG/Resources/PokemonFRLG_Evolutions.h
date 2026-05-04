/*  Pokemon FRLG Evolution Chains
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Loads per-species evolution chains from
 *  Resources/PokemonFRLG/Data/Evolutions.json. Each species maps to the full
 *  list of species in its evolution line (including itself), so lookups are
 *  O(1) without traversal.
 *
 */

#ifndef PokemonAutomation_PokemonFRLG_Evolutions_H
#define PokemonAutomation_PokemonFRLG_Evolutions_H

#include <string>
#include <vector>

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


//  Returns every species in `slug`'s evolution line, including itself.
//  Returns a single-element vector containing only `slug` if no entry
//  exists in Evolutions.json for that species.
const std::vector<std::string>& evolution_chain_for(const std::string& species_slug);


}
}
}
#endif
