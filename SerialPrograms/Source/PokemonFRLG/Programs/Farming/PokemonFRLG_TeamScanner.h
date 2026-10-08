/*  Team Scanner
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Walks the party menu, scans each Pokemon's Summary (species via sprite
 *  match + dex# OCR cross-check, four moves via OCR), and writes the result
 *  to a JSON team file. The XP Grinder can import that file to pre-fill its
 *  Team Table instead of the user entering species/moves by hand.
 *
 *  The file format is exactly the XP Grinder's Team Table serialization, so
 *  the grinder loads it directly. Start on the overworld with the start menu
 *  CLOSED; returns to the overworld when complete.
 */

#ifndef PokemonAutomation_PokemonFRLG_TeamScanner_H
#define PokemonAutomation_PokemonFRLG_TeamScanner_H

#include "Common/Cpp/Options/SimpleIntegerOption.h"
#include "Common/Cpp/Options/StringOption.h"
#include "CommonFramework/Notifications/EventNotificationsTable.h"
#include "CommonFramework/Tools/VideoStream.h"
#include "CommonTools/Options/LanguageOCROption.h"
#include "NintendoSwitch/Controllers/Procon/NintendoSwitch_ProController.h"
#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


class TeamScanner_Descriptor : public SingleSwitchProgramDescriptor{
public:
    TeamScanner_Descriptor();
};

class TeamScanner : public SingleSwitchProgramInstance{
public:
    TeamScanner();
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
    StringOption OUTPUT_FILE;
    EventNotificationsOption NOTIFICATIONS;
};


}
}
}
#endif
