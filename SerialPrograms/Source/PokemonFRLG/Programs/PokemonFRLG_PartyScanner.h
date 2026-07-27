/*  Pokemon FRLG Party Scanner
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Walks the in-game party menu and OCR-reads each slot's Summary screen
 *  (dex#, nickname, 4 moves). Returns one PartySummaryRead per scanned slot.
 *
 *  Two entry points:
 *    - scan_party()        — full walk over the first N party slots.
 *    - scan_party_slot()   — single-slot rescan (used after level-up/evolution).
 *
 *  Both expect to start from the overworld and return to the overworld.
 */

#ifndef PokemonAutomation_PokemonFRLG_PartyScanner_H
#define PokemonAutomation_PokemonFRLG_PartyScanner_H

#include <vector>
#include "CommonFramework/Language.h"
#include "CommonFramework/Tools/VideoStream.h"
#include "NintendoSwitch/Controllers/Procon/NintendoSwitch_ProController.h"
#include "PokemonFRLG/Inference/PokemonFRLG_PartySummaryReader.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
    class ConsoleHandle;
    class ProController;
    class SingleSwitchProgramEnvironment;
    using ProControllerContext = ControllerContext<ProController>;
namespace PokemonFRLG{


struct PartyScanResult{
    int slot_1indexed = 0;       //  1..6
    PartySummaryRead read;       //  dex#, nickname, 4 move slugs
    std::string species_slug;    //  resolved from dex_no via FRLG species table; empty if not in table
};


//  Open the party menu, count how many slots are occupied, and return to the
//  overworld. Returns a value in [1, 6]. Use this instead of asking the user
//  for their party size.
int detect_party_size(
    SingleSwitchProgramEnvironment& env, ProControllerContext& context
);


//  Scan slots [1..party_size] starting from the overworld. Walks the party
//  menu, opens each Pokemon's Summary, reads page 1 and page 3, and returns
//  to the overworld. Throws OperationFailedException if menu navigation
//  cannot be completed.
//
//  Callers should pass party_size in [1, 6], or 0 to auto-detect the party
//  size from the menu (results.size() is then the detected size). Auto-detect
//  reuses the same menu-open, so it costs nothing extra over passing a size.
std::vector<PartyScanResult> scan_party(
    SingleSwitchProgramEnvironment& env, ProControllerContext& context,
    Language language, int party_size
);


//  Re-scan a single slot's Summary. Use this after a level-up or evolution
//  to refresh the in-memory state for one Pokemon without re-walking the
//  whole party. Starts from the overworld and returns to the overworld.
PartyScanResult scan_party_slot(
    SingleSwitchProgramEnvironment& env, ProControllerContext& context,
    Language language, int slot_1indexed
);


}
}
}
#endif
