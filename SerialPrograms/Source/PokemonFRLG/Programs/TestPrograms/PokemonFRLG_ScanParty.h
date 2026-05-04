/*  Scan Party
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Developer-only diagnostic program. Runs the full party scanner against
 *  the live console: opens the party menu, walks slots 1..N, OCR-reads
 *  each Pokemon's Summary, and logs everything. Used to validate the
 *  scanner's navigation flow and OCR coordinates end-to-end.
 *
 *  Start the program with the player on the overworld. Returns to the
 *  overworld when complete.
 */

#ifndef PokemonAutomation_PokemonFRLG_ScanParty_H
#define PokemonAutomation_PokemonFRLG_ScanParty_H

#include "Common/Cpp/Options/SimpleIntegerOption.h"
#include "CommonFramework/Tools/VideoStream.h"
#include "CommonTools/Options/LanguageOCROption.h"
#include "NintendoSwitch/Controllers/Procon/NintendoSwitch_ProController.h"
#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


class ScanParty_Descriptor : public SingleSwitchProgramDescriptor{
public:
    ScanParty_Descriptor();
};

class ScanParty : public SingleSwitchProgramInstance{
public:
    ScanParty();
    virtual void program(
        SingleSwitchProgramEnvironment& env,
        ProControllerContext& context
    ) override;
    virtual void start_program_border_check(
        VideoStream& stream, FeedbackType feedback_type
    ) override{}

private:
    OCR::LanguageOCROption LANGUAGE;
    SimpleIntegerOption<uint64_t> PARTY_SIZE;
};


}
}
}
#endif
