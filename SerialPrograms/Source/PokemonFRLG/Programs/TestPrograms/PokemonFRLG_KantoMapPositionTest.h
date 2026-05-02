/*  Kanto Map Position Test
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#ifndef PokemonAutomation_PokemonFRLG_KantoMapPositionTest_H
#define PokemonAutomation_PokemonFRLG_KantoMapPositionTest_H

#include "Common/Cpp/Options/SimpleIntegerOption.h"
#include "CommonFramework/Notifications/EventNotificationsTable.h"
#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

class KantoMapPositionTest_Descriptor : public SingleSwitchProgramDescriptor{
public:
    KantoMapPositionTest_Descriptor();
};

class KantoMapPositionTest : public SingleSwitchProgramInstance{
public:
    KantoMapPositionTest();
    virtual void program(SingleSwitchProgramEnvironment& env, ProControllerContext& context) override;
    virtual void start_program_border_check(
        VideoStream& stream,
        FeedbackType feedback_type
    ) override{}

private:
    SimpleIntegerOption<uint32_t> POLL_INTERVAL_MS;

    EventNotificationsOption NOTIFICATIONS;
};

}
}
}
#endif
