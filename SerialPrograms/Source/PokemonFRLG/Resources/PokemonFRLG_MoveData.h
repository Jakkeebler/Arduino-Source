/*  Pokemon FRLG Move Data
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Loads the FRLG move database from Resources/PokemonFRLG/Data/Moves.json.
 *  Exposes lookup helpers and a StringSelectDatabase suitable for UI dropdowns.
 *
 */

#ifndef PokemonAutomation_PokemonFRLG_MoveData_H
#define PokemonAutomation_PokemonFRLG_MoveData_H

#include <cstdint>
#include <string>
#include <vector>

namespace PokemonAutomation{

class StringSelectDatabase;

namespace NintendoSwitch{
namespace PokemonFRLG{


struct MoveData{
    std::string slug;
    std::string display_eng;
    std::string type;       //  "normal", "fire", "water", ...
    uint8_t max_pp = 0;
    std::string category;   //  "physical", "special", "status"
};


//  Look up by slug. Throws InternalProgramError if not in the database.
const MoveData& get_move(const std::string& slug);
//  Look up by slug. Returns nullptr if not in the database.
const MoveData* get_move_nothrow(const std::string& slug);

//  Reverse lookup: English display name -> slug. Empty string if not found.
const std::string& parse_move_display_name(const std::string& display_name);

//  Returns true if `slug` exists in the database and its category is
//  "physical" or "special" (i.e. deals damage). Returns false for status
//  moves, unknown slugs, and empty slugs. Used by the XP Grinder to skip
//  pure stat-debuff moves (Growl, Tail Whip, Leer, etc.) when picking which
//  move to use against a wild Pokemon.
bool is_damaging_move(const std::string& slug);

//  All moves in slug order.
const std::vector<MoveData>& all_moves();

//  StringSelectDatabase suitable for UI dropdowns, lazily built on first call.
//  Entries are slug + English display name. (Type icons can be added later.)
const StringSelectDatabase& move_select_database();


}
}
}
#endif
