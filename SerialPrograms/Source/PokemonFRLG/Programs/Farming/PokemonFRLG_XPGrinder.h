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
#include "Common/Cpp/Options/GroupOption.h"
#include "Common/Cpp/Options/StringOption.h"
#include "Common/Cpp/Options/ButtonOption.h"
#include "CommonFramework/Notifications/EventNotificationsTable.h"
#include "CommonTools/Options/LanguageOCROption.h"
#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "NintendoSwitch/Options/NintendoSwitch_GoHomeWhenDoneOption.h"
#include "PokemonFRLG/Programs/PokemonFRLG_GrindHealLocations.h"
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

class XPGrinder : public SingleSwitchProgramInstance, public ButtonListener{
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
        //  Switch training: the lead is a Pokemon too weak to fight (a Magikarp,
        //  a freshly caught low-level). It is sent out so it counts as a battle
        //  participant, then immediately withdrawn for a Pokemon that can win.
        //  Gen 3 splits EXP among everyone sent out, so the trainee is paid for
        //  showing up without ever taking a hit.
        switch_training,
    };

    XPGrinder();
    ~XPGrinder();
    virtual void program(SingleSwitchProgramEnvironment& env, ProControllerContext& context) override;
    virtual void start_program_border_check(
        VideoStream& stream,
        FeedbackType feedback_type
    ) override{}

    //  IMPORT_TEAM_BUTTON listener: loads IMPORT_TEAM_FILE into TEAM_TABLE so
    //  the user can review the imported team in the UI before running.
    virtual void on_press(ButtonCell& button) override;

private:
    SimpleIntegerOption<uint64_t> MAX_BATTLES;

    BooleanCheckBoxOption PREVENT_EVOLUTION;
    BooleanCheckBoxOption IGNORE_SHINIES;

    EnumDropdownOption<GrindLocationId> GRIND_LOCATION;
    BooleanCheckBoxOption NAVIGATE_TO_GRIND_ON_START;
    BooleanCheckBoxOption AUTO_HEAL_LOCATION;
    EnumDropdownOption<HealLocationId> HEAL_LOCATION;

    EnumDropdownOption<RotationMode> ROTATION_MODE;
    SimpleIntegerOption<uint64_t> FIGHTER_SLOT;
    SimpleIntegerOption<uint64_t> PARTY_SIZE;
    OCR::LanguageOCROption LANGUAGE;
    BooleanCheckBoxOption AUTO_SCAN_ON_START;
    StringOption IMPORT_TEAM_FILE;
    ButtonOption IMPORT_TEAM_BUTTON;
    BooleanCheckBoxOption AUTO_RANK_MOVES;
    BooleanCheckBoxOption AUTOFILL_DESIRED_MOVES;
    BooleanCheckBoxOption HOLD_EVOLUTION_FOR_MOVES;
    BooleanCheckBoxOption STOP_WHEN_TEAM_COMPLETE;
    XpGrinderTeamTable TEAM_TABLE;

    BooleanCheckBoxOption HEAL_BEFORE_START;
    BooleanCheckBoxOption HEAL_ON_FAINT;
    BooleanCheckBoxOption HEAL_ON_OUT_OF_PP;
    SimpleIntegerOption<uint64_t> BATTLES_PER_HEAL_TRIP;
    EnumDropdownOption<TravelMethod> TRAVEL_METHOD;

    BooleanCheckBoxOption TAKE_VIDEO;
    GoHomeWhenDoneOption GO_HOME_WHEN_DONE;
    EventNotificationOption NOTIFICATION_SHINY;
    EventNotificationOption NOTIFICATION_STATUS_UPDATE;
    EventNotificationsOption NOTIFICATIONS;

    //  Presentation only.
    //
    //  These own nothing -- every option above stays exactly where it is, and the
    //  groups just claim them at registration time via add_option(). That keeps
    //  the panel readable without renaming the several hundred references to
    //  these options in the .cpp.
    //
    //  Declared last so the constructor's initializer list order stays valid.
    GroupOption GRIND_SETUP;
    GroupOption PARTY_SETUP;
    GroupOption HEALING;
    GroupOption TEAM_SETUP;
};

}
}
}
#endif
