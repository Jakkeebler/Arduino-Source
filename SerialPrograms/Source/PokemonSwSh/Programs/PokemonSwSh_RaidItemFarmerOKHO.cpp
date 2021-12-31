/*  OHKO Raid Farmer
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include "Common/Cpp/PrettyPrint.h"
#include "NintendoSwitch/Commands/NintendoSwitch_Commands_Device.h"
#include "NintendoSwitch/Commands/NintendoSwitch_Commands_Routines.h"
#include "NintendoSwitch/NintendoSwitch_Settings.h"
#include "NintendoSwitch/FixedInterval.h"
#include "PokemonSwSh/Commands/PokemonSwSh_Commands_GameEntry.h"
#include "PokemonSwSh/Commands/PokemonSwSh_Commands_DateSpam.h"
#include "PokemonSwSh/Programs/Hosting/PokemonSwSh_DenTools.h"
#include "PokemonSwSh_RaidItemFarmerOKHO.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonSwSh{


RaidItemFarmerOHKO_Descriptor::RaidItemFarmerOHKO_Descriptor()
    : MultiSwitchProgramDescriptor(
        "PokemonSwSh:RaidItemFarmerOHKO",
        STRING_POKEMON + " SwSh", "Raid Item Farmer (OHKO)",
        "ComputerControl/blob/master/Wiki/Programs/PokemonSwSh/RaidItemFarmerOHKO.md",
        "Farm items from raids that can be OHKO'ed. (requires multiple Switches)",
        FeedbackType::NONE,
        PABotBaseLevel::PABOTBASE_12KB,
        2, 4, 2
    )
{}



RaidItemFarmerOHKO::RaidItemFarmerOHKO(const RaidItemFarmerOHKO_Descriptor& descriptor)
    : MultiSwitchProgramInstance(descriptor)
    , BACKUP_SAVE("<b>Load Backup Save:</b><br>For backup save soft-locking method.", false)
//    , m_advanced_options(
//        "<font size=4><b>Advanced Options:</b> You should not need to touch anything below here.</font>"
//    )
    , WAIT_FOR_STAMP_DELAY(
        "<b>Wait for Stamp Delay:</b><br>Wait this long for the stamp to show up.",
        "3 * TICKS_PER_SECOND"
    )
    , ENTER_STAMP_MASH_DURATION(
        "<b>Enter Stamp Mash Duration:</b><br>Mash A this long to enter a raid from its stamp.",
        "5 * TICKS_PER_SECOND"
    )
    , RAID_START_MASH_DURATION(
        "<b>Raid Start Mash Duration:</b><br>Mash A this long to start raid.",
        "10 * TICKS_PER_SECOND"
    )
    , RAID_START_TO_ATTACK_DELAY(
        "<b>Raid Start to Attack Delay:</b><br>Time from start raid to when the raiders attack.<br>"
        "Do not over-optimize this timing unless you are running with 4 Switches. The Wishiwashi NPC will break the program.",
        "30 * TICKS_PER_SECOND"
    )
    , ATTACK_TO_CATCH_DELAY(
        "<b>Attack to Catch Delay:</b><br>Time from when you attack to when the catch selection appears.<br>"
        "Do not over-optimize this timing unless you are running with 4 Switches. The Clefairy NPC's Follow Me will break the program.",
        "18 * TICKS_PER_SECOND"
    )
    , RETURN_TO_OVERWORLD_DELAY(
        "<b>Return to Overworld Delay:</b><br>Time from when you don't catch to when you return to the overworld.",
        "18 * TICKS_PER_SECOND"
    )
    , TOUCH_DATE_INTERVAL(
        "<b>Rollover Prevention:</b><br>Prevent the den from rolling over by periodically touching the date. If set to zero, this feature is disabled.",
        "4 * 3600 * TICKS_PER_SECOND"
    )
{
    PA_ADD_OPTION(BACKUP_SAVE);
//    PA_ADD_STATIC(m_advanced_options);
    PA_ADD_OPTION(WAIT_FOR_STAMP_DELAY);
    PA_ADD_OPTION(ENTER_STAMP_MASH_DURATION);
    PA_ADD_OPTION(RAID_START_MASH_DURATION);
    PA_ADD_OPTION(RAID_START_TO_ATTACK_DELAY);
    PA_ADD_OPTION(ATTACK_TO_CATCH_DELAY);
    PA_ADD_OPTION(RETURN_TO_OVERWORLD_DELAY);
    PA_ADD_OPTION(TOUCH_DATE_INTERVAL);
}

void RaidItemFarmerOHKO::program(MultiSwitchProgramEnvironment& env){
    BotBase& host = env.consoles[0];
    size_t switches = env.consoles.size();

    env.run_in_parallel(
        [](ConsoleHandle& console){
            grip_menu_connect_go_home(console);
        }
    );

    uint32_t last_touch = 0;
    if (TOUCH_DATE_INTERVAL > 0){
        touch_date_from_home(host, ConsoleSettings::instance().SETTINGS_TO_HOME_DELAY);
        last_touch = system_clock(host);
    }
    env.run_in_parallel(
        [](ConsoleHandle& console){
            if (console.index() == 0){
                resume_game_front_of_den_nowatts(console, ConsoleSettings::instance().TOLERATE_SYSTEM_UPDATE_MENU_SLOW);
            }else{
                resume_game_no_interact(console, ConsoleSettings::instance().TOLERATE_SYSTEM_UPDATE_MENU_SLOW);
            }
        }
    );


    for (uint32_t raids = 0;; raids++){
        env.log("Raids Completed: " + tostr_u_commas(raids));

        host.wait_for_all_requests();
        env.run_in_parallel(
            [&](ConsoleHandle& console){
                if (console.index() == 0){
                    enter_den(console, 0, false, false);
                }else{
                    pbf_press_button(console, BUTTON_Y, 10, GameSettings::instance().OPEN_YCOMM_DELAY);
                    pbf_press_dpad(console, DPAD_UP, 5, 0);
                    pbf_move_right_joystick(console, 128, 0, 5, 0);
                    pbf_press_button(console, BUTTON_X, 10, 10);
                }
            }
        );

        enter_lobby(host, 0, false, Catchability::ALWAYS_CATCHABLE);

        host.wait_for_all_requests();
        env.run_in_parallel(
            1, switches,
            [&](ConsoleHandle& console){
                pbf_wait(console, WAIT_FOR_STAMP_DELAY);
                pbf_press_button(console, BUTTON_X, 10, 10);
                pbf_press_dpad(console, DPAD_RIGHT, 10, 10);
                pbf_mash_button(console, BUTTON_A, ENTER_STAMP_MASH_DURATION);
            }
        );

        //  Start Raid
        pbf_press_dpad(host, DPAD_UP, 5, 45);

        host.wait_for_all_requests();
        env.run_in_parallel(
            [&](ConsoleHandle& console){
                pbf_mash_button(console, BUTTON_A, RAID_START_MASH_DURATION);
                pbf_wait(console, RAID_START_TO_ATTACK_DELAY);
                pbf_mash_button(console, BUTTON_A, 5 * TICKS_PER_SECOND);
                pbf_wait(console, ATTACK_TO_CATCH_DELAY);

                if (console.index() == 0){
                    //  Add a little extra wait time since correctness matters here.
                    ssf_press_button2(host, BUTTON_HOME, GameSettings::instance().GAME_TO_HOME_DELAY_SAFE, 10);

                    close_game(console);

                    //  Touch the date.
                    if (TOUCH_DATE_INTERVAL > 0 && system_clock(console) - last_touch >= TOUCH_DATE_INTERVAL){
                        touch_date_from_home(console, ConsoleSettings::instance().SETTINGS_TO_HOME_DELAY);
                        last_touch += TOUCH_DATE_INTERVAL;
                    }
                    start_game_from_home(
                        console,
                        ConsoleSettings::instance().TOLERATE_SYSTEM_UPDATE_MENU_SLOW,
                        0, 0,
                        BACKUP_SAVE
                    );
                }else{
                    pbf_press_dpad(console, DPAD_DOWN, 10, 10);
                    if (RETURN_TO_OVERWORLD_DELAY > 5 * TICKS_PER_SECOND){
                        pbf_mash_button(console, BUTTON_A, RETURN_TO_OVERWORLD_DELAY - 5 * TICKS_PER_SECOND);
                        pbf_mash_button(console, BUTTON_B, 5 * TICKS_PER_SECOND);
                    }else{
                        pbf_mash_button(console, BUTTON_A, RETURN_TO_OVERWORLD_DELAY);
                    }
                }
            }
        );


    }

}



}
}
}
