/*  XP Grinder
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#ifndef PokemonAutomation_PokemonFRLG_XPGrinder_H
#define PokemonAutomation_PokemonFRLG_XPGrinder_H

#include "Common/Cpp/Options/SimpleIntegerOption.h"
#include "Common/Cpp/Options/BooleanCheckBoxOption.h"
#include "Common/Cpp/Options/EnumDropdownOption.h"
#include "CommonFramework/Notifications/EventNotificationsTable.h"
#include "CommonTools/Options/LanguageOCROption.h"
#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "NintendoSwitch/Options/NintendoSwitch_GoHomeWhenDoneOption.h"
#include "PokemonFRLG_XpGrinderTeamTable.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

class XPGrinder_Descriptor : public SingleSwitchProgramDescriptor{
public:
    XPGrinder_Descriptor();
    struct Stats;
    virtual std::unique_ptr<StatsTracker> make_stats() const override;
};

class XPGrinder : public SingleSwitchProgramInstance{
public:
    enum class TravelMethod{
        fly,
        teleport,
        walk,
    };

    enum class RotationMode{
        disabled,
        per_battle,
        faint_triggered,
        pp_exhaustion,
    };

    XPGrinder();
    virtual void program(SingleSwitchProgramEnvironment& env, ProControllerContext& context) override;
    virtual void start_program_border_check(
        VideoStream& stream,
        FeedbackType feedback_type
    ) override{}

private:
    SimpleIntegerOption<uint64_t> MAX_BATTLES;

    BooleanCheckBoxOption PREVENT_EVOLUTION;
    BooleanCheckBoxOption IGNORE_SHINIES;

    EnumDropdownOption<RotationMode> ROTATION_MODE;
    SimpleIntegerOption<uint64_t> PARTY_SIZE;
    OCR::LanguageOCROption LANGUAGE;
    BooleanCheckBoxOption AUTO_SCAN_ON_START;
    XpGrinderTeamTable TEAM_TABLE;

    BooleanCheckBoxOption HEAL_ON_FAINT;
    BooleanCheckBoxOption HEAL_ON_OUT_OF_PP;
    SimpleIntegerOption<uint64_t> BATTLES_PER_HEAL_TRIP;
    EnumDropdownOption<TravelMethod> TRAVEL_METHOD;

    BooleanCheckBoxOption TAKE_VIDEO;
    GoHomeWhenDoneOption GO_HOME_WHEN_DONE;
    EventNotificationOption NOTIFICATION_SHINY;
    EventNotificationOption NOTIFICATION_STATUS_UPDATE;
    EventNotificationsOption NOTIFICATIONS;
};

}
}
}
#endif
