/*  Pokemon FRLG Learnsets
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Loads per-species level-up learnsets from
 *  Resources/PokemonFRLG/Data/Learnsets.json.
 *
 */

#ifndef PokemonAutomation_PokemonFRLG_Learnsets_H
#define PokemonAutomation_PokemonFRLG_Learnsets_H

#include <cstdint>
#include <string>
#include <vector>

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


struct LearnsetEntry{
    uint8_t level = 0;
    std::string move_slug;
};


//  Returns the level-up learnset for the given species in the order it appears
//  in the JSON. Returns an empty vector reference if the species has no entry.
const std::vector<LearnsetEntry>& learnset_for(const std::string& species_slug);


//  Returns the union of move slugs that any species in `species_slug`'s
//  evolution line can learn via level-up. Sorted alphabetically by the move's
//  English display name. Used by the team-table to filter the desired-move
//  dropdowns to a manageable list.
//
//  An unknown species_slug falls back to that species' own learnset only.
//  An empty species_slug returns the full move list (used by row's species
//  cell when no species is selected yet).
std::vector<std::string> learnable_moves_for_chain(const std::string& species_slug);


}
}
}
#endif
