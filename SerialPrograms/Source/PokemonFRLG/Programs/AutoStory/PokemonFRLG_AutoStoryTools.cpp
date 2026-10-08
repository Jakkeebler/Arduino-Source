/*  Pokemon FRLG AutoStory Tools
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include "Common/Cpp/Exceptions.h"
#include "CommonFramework/Exceptions/OperationFailedException.h"
#include "CommonFramework/Notifications/EventNotificationOption.h"
#include "CommonFramework/Notifications/ProgramNotifications.h"
#include "PokemonFRLG_AutoStoryTools.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


void navigate_to(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    AutoStoryStats& stats,
    const KantoGoal& goal,
    const KantoGoal* start_hint,
    int max_steps
){
    if (start_hint != nullptr){
        kanto_navigate_to(env, context, goal, *start_hint, max_steps);
    }else{
        kanto_navigate_to(env, context, goal, max_steps);
    }
    stats.m_steps_walked++;
}


void enter_building(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    BuildingType building
){
    switch (building){
    case BuildingType::POKECENTER:
        enter_pokecenter(env.console, context);
        return;
    }
    OperationFailedException::fire(
        ErrorReport::SEND_ERROR_REPORT,
        "enter_building(): unsupported building type.",
        env.console
    );
}

void exit_building(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    BuildingType building
){
    switch (building){
    case BuildingType::POKECENTER:
        leave_pokecenter(env.console, context);
        return;
    }
    OperationFailedException::fire(
        ErrorReport::SEND_ERROR_REPORT,
        "exit_building(): unsupported building type.",
        env.console
    );
}


void fly_to(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    KantoFlyLocation destination
){
    open_fly_map_from_overworld(env.console, context);
    fly_from_kanto_map(env.console, context, destination);
}


bool ensure_healed(
    SingleSwitchProgramEnvironment& env,
    ProControllerContext& context,
    const AutoStoryOptions& options,
    AutoStoryStats& stats,
    uint16_t battles_since_last_heal,
    const KantoGoal& heal_pc_entrance,
    const KantoGoal* start_hint
){
    if (options.auto_heal_threshold == 0){
        return false;
    }
    if (battles_since_last_heal < options.auto_heal_threshold){
        return false;
    }

    env.log("AutoStoryTools: auto-heal threshold reached. Starting heal trip.");
    send_program_status_notification(
        env, options.notif_status_update,
        "Auto-heal threshold reached. Heading to the Pokemon Center."
    );

    navigate_to(env, context, stats, heal_pc_entrance, start_hint);
    enter_building(env, context, BuildingType::POKECENTER);
    heal_at_pokecenter(env.console, context);
    exit_building(env, context, BuildingType::POKECENTER);

    stats.m_healing_trips++;
    env.log("AutoStoryTools: heal trip complete.");
    return true;
}


void track_battle_won(AutoStoryStats& stats){
    stats.m_battles_won++;
}

void track_phase_complete(
    SingleSwitchProgramEnvironment& env,
    const AutoStoryOptions& options,
    AutoStoryStats& stats,
    const std::string& phase_name
){
    stats.m_phases_complete++;
    send_program_status_notification(
        env, options.notif_status_update,
        "Phase complete: " + phase_name
    );
}


}
}
}
