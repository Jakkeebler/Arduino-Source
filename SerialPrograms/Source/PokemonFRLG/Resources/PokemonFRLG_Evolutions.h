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


struct LevelEvolution{
    std::string evolves_to;
    int level = 0;
};

//  The level-up evolution of `species_slug`, or nullptr if it has none --
//  fully evolved, or evolves by stone / trade / friendship (those have no level
//  to be "one away" from). Gen-3 levels, Kanto species only.
//
//  This is a built-in table rather than a field in Evolutions.json: that file
//  only lists chain membership, and the levels never change.
const LevelEvolution* level_evolution_for(const std::string& species_slug);


}
}
}
#endif
