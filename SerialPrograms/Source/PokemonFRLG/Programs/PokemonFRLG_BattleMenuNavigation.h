/*  Battle Menu Navigation
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#ifndef PokemonAutomation_PokemonFRLG_BattleMenuNavigation_H
#define PokemonAutomation_PokemonFRLG_BattleMenuNavigation_H

#include "PokemonFRLG/Inference/PokemonFRLG_BattleSelectionArrowDetector.h"

namespace PokemonAutomation{

template <typename Type> class ControllerContext;

namespace NintendoSwitch{

class ConsoleHandle;
class ProController;
using ProControllerContext = ControllerContext<ProController>;

namespace PokemonFRLG{


// Starting from the normal battle command menu, move the selection arrow to the specified option.
// Returns true if successful, false otherwise (e.g. if selection arrow is not detected).
bool move_cursor_to_option(
    ConsoleHandle& console, ProControllerContext& context,
    BattleMenuOption destination
);

// Starting from the Safari Zone battle command menu, move the selection arrow to the specified option.
// Returns true if successful, false otherwise (e.g. if selection arrow is not detected).
bool move_cursor_to_option(
    ConsoleHandle& console, ProControllerContext& context,
    SafariBattleMenuOption destination
);


// In-battle move list (FIGHT submenu), 2x2 grid:
//   Move1 Move2
//   Move3 Move4
enum class MoveSlot{
    Move1 = 0,
    Move2 = 1,
    Move3 = 2,
    Move4 = 3,
};

// Starting from the FIGHT move list with the selection arrow somewhere on a move slot,
// move the cursor to `destination`. Returns true if successful, false if the arrow
// could not be detected (the float-box coordinates may need calibration).
bool move_cursor_to_move_slot(
    ConsoleHandle& console, ProControllerContext& context,
    MoveSlot destination
);


}
}
}
#endif
