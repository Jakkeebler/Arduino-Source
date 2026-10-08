/*  Pokemon FRLG Evolution Policy
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Per-party-slot evolution policy for the XP Grinder (FRO-226). Pure logic,
 *  no I/O -- the grind loop (PokemonFRLG_XPGrinder.cpp) and the team table
 *  (PokemonFRLG_XpGrinderTeamTable) are the only callers.
 *
 */

#ifndef PokemonAutomation_PokemonFRLG_EvolutionPolicy_H
#define PokemonAutomation_PokemonFRLG_EvolutionPolicy_H

#include <string>
#include "Common/Cpp/Options/EnumDropdownDatabase.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


//  What should happen to this party slot's Pokemon when it is able to evolve.
enum class EvolutionPolicyType{
    //  Let a level-up evolution proceed automatically, same as the grinder's
    //  historical behavior. This is the only policy that allows the grind
    //  loop to auto-confirm a level-up evolution prompt.
    LevelUp,
    //  Evolves only via an explicit, user-invoked "use item on this Pokemon"
    //  action (see EvolutionStoneItem below and XPGrinder's
    //  USE_EVOLUTION_STONE_BUTTON) -- never as a side effect of a level-up
    //  dialog. The grind loop always declines an automatic evolution prompt
    //  for this slot (most Stone-policy species have no level-up evolution
    //  at all, but the decline still applies defensively if one exists).
    Stone,
    //  This Pokemon's only evolution is by trade, which the grind loop cannot
    //  perform. Flagged so the config is explicit about it rather than silently
    //  behaving like Block; logged once so the user knows it's unsupported.
    Trade,
    //  Never evolve this Pokemon under any path (e.g. keep a Pikachu a
    //  Pikachu). The grind loop always declines automatic evolution prompts
    //  AND stops the run one level before a level-up evolution would fire
    //  rather than relying on an indefinite auto-decline (see
    //  should_stop_grind_before_blocked_evolution()).
    Block,
};
const EnumDropdownDatabase<EvolutionPolicyType>& EvolutionPolicyType_Database();


//  Evolution stones available in FRLG. "none" is the sentinel for "no stone
//  configured" -- a Stone-policy row with no stone selected can't be used by
//  USE_EVOLUTION_STONE_BUTTON (the caller must check for this).
enum class EvolutionStoneItem{
    None,
    FireStone,
    WaterStone,
    Thunderstone,
    LeafStone,
    MoonStone,
};
const EnumDropdownDatabase<EvolutionStoneItem>& EvolutionStoneItem_Database();

//  The in-bag item name the game expects, e.g. "Fire Stone". Empty for None.
std::string evolution_stone_item_name(EvolutionStoneItem item);


//  ---- Pure helpers (no data tables beyond the caller-supplied level, no I/O,
//  unit-testable) ----

//  True if `type` must never be allowed to go through an automatic (level-up
//  dialog) evolution prompt -- i.e. the caller should pass
//  prevent_evolution=true to exit_wild_battle for this Pokemon's battles.
//  Only LevelUp returns false.
bool policy_blocks_automatic_evolution(EvolutionPolicyType type);

//  True only for Stone: the one policy whose evolution is meant to happen
//  through an explicit, separately-invoked action rather than ever being
//  inferred from a level-up dialog.
bool policy_uses_explicit_stone_step(EvolutionPolicyType type);

//  Pre-level-up evolution guard (FRO-226 scope item 2). Returns true when the
//  very next level-up would trigger a level-up evolution that this policy
//  must never allow to fire automatically -- i.e. the grind loop should stop
//  itself and surface this to the user instead of relying on exit_wild_battle
//  to silently decline the prompt forever.
//
//  `current_level`: the Pokemon's level before its next battle win.
//  `evolution_level`: the species' level-up evolution level from
//  level_evolution_for() (PokemonFRLG_Evolutions.h), or <= 0 if the species
//  has no level-up evolution (fully evolved, or evolves by stone/trade/
//  friendship instead).
//
//  LevelUp never triggers this guard (it is allowed to evolve). Stone is
//  excluded even though it blocks automatic evolution: a Stone-policy species
//  essentially never also has a level-up evolution in FRLG, and if one somehow
//  exists, the per-battle decline in exit_wild_battle already covers it without
//  needing to halt the whole run.
bool should_stop_grind_before_blocked_evolution(
    EvolutionPolicyType type, int current_level, int evolution_level
);


}
}
}
#endif
