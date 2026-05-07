/*  Pokemon FRLG Navigation
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Soft reset, menus, etc.
 *
 */

#ifndef PokemonAutomation_PokemonFRLG_Navigation_H
#define PokemonAutomation_PokemonFRLG_Navigation_H

#include <vector>
#include "CommonFramework/Language.h"
#include "CommonFramework/Tools/VideoStream.h"
#include "NintendoSwitch/Controllers/Procon/NintendoSwitch_ProController.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
    class ConsoleHandle;
    class ProController;
    using ProControllerContext = ControllerContext<ProController>;
namespace PokemonFRLG{

enum class BattleResult{
    opponentfainted,
    playerfainted,
    outofpp,
    unknown
};

class MoveLearnDecider;

//  Outcome of exit_wild_battle. Distinguishing these is critical: in the
//  StopBattleStuck case the move-learn dialog is still on-screen and the
//  caller MUST NOT attempt overworld navigation; the only safe action is
//  to halt the program for manual review.
enum class WildBattleExit{
    NoLearn,           //  Battle exited cleanly. No move-learn dialog appeared.
    LearnHandled,      //  Move-learn dialog appeared and was handled (declined or
                       //  replaced) per the decider; battle exited cleanly.
    StopBattleStuck,   //  Stop signal fired (stop_on_move_learn=true OR decider
                       //  returned Stop). exit_wild_battle returned WITHOUT
                       //  exiting the battle — the dialog is still active.
};

enum class KantoFlyLocation{
    pallettown,
    viridiancity,
    pewtercity,
    route4,
    ceruleancity,
    vermilioncity,
    route10,
    lavendertown,
    celadoncity,
    saffroncity,
    fuschiacity,
    cinnabarisland,
    indigoplateau,
};


// Press A+B+Select+Start at the same time to soft reset, then re-enters the game.
// There are two random waits, one before pressing start and another after loading in the game.
// This is to prevent repeatedly getting the same pokemon, due to FRLG's RNG.
uint64_t soft_reset(ConsoleHandle& console, ProControllerContext &context);

// From the overworld, open the summary of the Pokemon in slot 6. This assumes the menu cursor is in the top slot (POKEDEX)
uint64_t open_slot_six(ConsoleHandle& console, ProControllerContext& context);

// After press A/walking up to enter a battle, run this handle the battle start and to check if opponent is shiny.
// Set send_out_lead to true and then use flee_battle() after if for run away resets
// For soft resets, send_out_lead as false and then soft_reset() to save time.
bool handle_encounter(ConsoleHandle& console, ProControllerContext& context, bool send_out_lead);

// Use a move each turn until a Pokemon faints (either the player's or the opponent).
// `move_priority` is a list of 0-based move slot indices in the order to try
// when the highest-priority slot is out of PP. Default {0} preserves the
// historical "always use move 1" behavior.
// Flees the battle if every priority slot is out of PP.
// Returns a BattleResult indicating how the battle ended.
BattleResult spam_first_move(
    ConsoleHandle& console, ProControllerContext& context,
    const std::vector<size_t>& move_priority = std::vector<size_t>{0}
);

// Run from battle. Cursor must start on the FIGHT button. Assumes fleeing will always work. (Smoke Ball)
void flee_battle(ConsoleHandle& console, ProControllerContext& context);

// Exit a wild battle after winning. Checks if a Pokemon is learning a new move.
// If stop_on_move_learn is true, this returns StopBattleStuck early when a
// move-learn dialog appears (battle is NOT exited). Otherwise, behaviour is
// determined by `decider`:
//   nullptr   - always decline new moves (preserves existing 4 moves).
//   non-null  - OCR the offered move's name, ask the decider whether to accept;
//               if accepting, OCR the forget screen and ask the decider which
//               slot to forget. `language` selects the OCR dictionary.
//
// Returns:
//   NoLearn          - no move-learn dialog appeared, battle exited.
//   LearnHandled     - move-learn dialog handled (declined or replaced),
//                      battle exited cleanly. Caller may rescan the slot.
//   StopBattleStuck  - Stop fired (via stop_on_move_learn or decider). The
//                      dialog is STILL ACTIVE. Caller must halt the program
//                      and NOT attempt any further navigation.
WildBattleExit exit_wild_battle(
    ConsoleHandle& console, ProControllerContext& context,
    bool stop_on_move_learn, bool prevent_evolution,
    const MoveLearnDecider* decider = nullptr,
    Language language = Language::English
);

// Starting from the start menu, a sub-screen of the start menu, or the overworld, navigate to the party screen
enum class StartMenuContext {
    STANDARD,
    SAFARI_ZONE
};
void open_party_menu_from_overworld(ConsoleHandle& console, ProControllerContext& context, StartMenuContext menu_context = StartMenuContext::STANDARD);

// Swap the Pokémon at game_slot_1indexed (2–6) into the lead (slot 1) via the overworld party menu SWITCH command.
// Assumes the party menu is closed and the player is in the overworld.
void switch_party_lead_overworld(ConsoleHandle& console, ProControllerContext& context, int game_slot_1indexed);

// After a player Pokémon faints in battle the game shows the forced-switch party screen.
// This function navigates to game_slot_1indexed (2–6) and sends it out.
// Call after spam_first_move() returns BattleResult::playerfainted when alive allies remain.
void select_forced_switch_slot(ConsoleHandle& console, ProControllerContext& context, int game_slot_1indexed);

// Starting from the start menu, a sub-screen of the start menu, or the overworld, navigate to the bag
void open_bag_from_overworld(ConsoleHandle& console, ProControllerContext& context, StartMenuContext menu_context = StartMenuContext::STANDARD);

// Uses Teleport to return to a PokeCenter. 
// Assumes that Teleport is usable and the last party member has it learned
void use_teleport_from_overworld(ConsoleHandle& console, ProControllerContext& context);

// Navigates to the fly map. Assumes that Fly is usable and the last member of your party has it learned
void open_fly_map_from_overworld(ConsoleHandle& console, ProControllerContext& context);

// Starting from the Kanto Fly map, fly to a specified location.
void fly_from_kanto_map(ConsoleHandle& console, ProControllerContext& context, KantoFlyLocation destination);

// Enter a PokeCenter. Assumes the player is standing in front of its door
void enter_pokecenter(ConsoleHandle& console, ProControllerContext& context);

// Leave a PokeCenter. Assumes the player is standing directly north of the exit
void leave_pokecenter(ConsoleHandle& console, ProControllerContext& context);

// Approach the counter and heal at a PokeCenter. Assumed the player is directly south of the nurse
// Combine with enter_pokecenter, leave_pokecenter, and use_teleport_from_overworld for automating healing your party
void heal_at_pokecenter(ConsoleHandle& console, ProControllerContext& context);

// Trigger encounters in grass without moving by tapping the left thumbstick back and forth
// Can be used to alternate left/right and up/down. It is important that the player is not facing
// the same direction as the first thumbstick press.
// returns -1 if no encounter is triggered, 0 if a non-shiny is encounter, and 1 if a shiny is encountered
int grass_spin(ConsoleHandle& console, ProControllerContext& context, bool leftright, Seconds timeout = std::chrono::seconds(60));

// Go to home to check that scaling is 100%. Then resume game.
void home_black_border_check(ConsoleHandle& console, ProControllerContext& context);

}
}
}
#endif
