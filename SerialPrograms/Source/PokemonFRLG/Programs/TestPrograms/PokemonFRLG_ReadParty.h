/*  Read Party
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Developer-only diagnostic program. Reads the currently-displayed Summary
 *  screen (page 1: dex# + nickname; navigates to page 3: 4 moves) and logs
 *  everything to the console. Used to calibrate the FRLG Summary OCR boxes.
 *
 *  Start the program with the Pokemon's Summary page 1 visible.
 */

#ifndef PokemonAutomation_PokemonFRLG_ReadParty_H
#define PokemonAutomation_PokemonFRLG_ReadParty_H

#include "CommonFramework/Tools/VideoStream.h"
#include "CommonTools/Options/LanguageOCROption.h"
#include "NintendoSwitch/Controllers/Procon/NintendoSwitch_ProController.h"
#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


class ReadParty_Descriptor : public SingleSwitchProgramDescriptor{
public:
    ReadParty_Descriptor();
};

class ReadParty : public SingleSwitchProgramInstance{
public:
    ReadParty();
    virtual void program(
        SingleSwitchProgramEnvironment& env,
        ProControllerContext& context
    ) override;
    virtual void start_program_border_check(
        VideoStream& stream, FeedbackType feedback_type
    ) override{}

private:
    OCR::LanguageOCROption LANGUAGE;
};


}
}
}
#endif
