/*  Mapping Mode
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#ifndef PokemonAutomation_PokemonFRLG_MappingMode_H
#define PokemonAutomation_PokemonFRLG_MappingMode_H

#include "Common/Cpp/Options/SimpleIntegerOption.h"
#include "Common/Cpp/Options/StringOption.h"
#include "CommonFramework/Notifications/EventNotificationsTable.h"
#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

class MappingMode_Descriptor : public SingleSwitchProgramDescriptor{
public:
    MappingMode_Descriptor();
};

class MappingMode : public SingleSwitchProgramInstance{
public:
    MappingMode();
    virtual void program(SingleSwitchProgramEnvironment& env, ProControllerContext& context) override;
    virtual void start_program_border_check(
        VideoStream& stream,
        FeedbackType feedback_type
    ) override{}

private:
    SimpleIntegerOption<uint32_t> CAPTURE_INTERVAL_MS;
    SimpleIntegerOption<uint32_t> MAX_CAPTURES;
    StringOption SUBFOLDER_NAME;

    EventNotificationsOption NOTIFICATIONS;
};

}
}
}
#endif
