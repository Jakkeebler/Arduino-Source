/*  Mapping Mode
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include <cstdio>
#include "Common/Cpp/Color.h"
#include "Common/Cpp/Filesystem.h"
#include "Common/Cpp/PrettyPrint.h"
#include "CommonFramework/ImageTypes/ImageRGB32.h"
#include "CommonFramework/Notifications/ProgramNotifications.h"
#include "CommonFramework/VideoPipeline/VideoFeed.h"
#include "Pokemon/Pokemon_Strings.h"
#include "PokemonFRLG_MappingMode.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

MappingMode_Descriptor::MappingMode_Descriptor()
    : SingleSwitchProgramDescriptor(
        "PokemonFRLG:MappingMode",
        Pokemon::STRING_POKEMON + " FRLG", "Mapping Mode",
        "",
        "Manual exploration tool. Drive the character with your keyboard while the "
        "program saves a screenshot every N milliseconds. Use the captured PNGs as "
        "landmark references for ImageMatchWatcher in other programs.",
        ProgramControllerClass::StandardController_NoRestrictions,
        FeedbackType::REQUIRED,
        AllowCommandsWhenRunning::ENABLE_COMMANDS,
        {}
    )
{}

MappingMode::MappingMode()
    : CAPTURE_INTERVAL_MS(
        "<b>Capture interval (ms):</b><br>How often to save a screenshot while exploring.",
        LockMode::UNLOCK_WHILE_RUNNING,
        2000, 250, 60000
    )
    , MAX_CAPTURES(
        "<b>Max captures:</b><br>Stop after this many frames have been saved.",
        LockMode::LOCK_WHILE_RUNNING,
        200, 1, 10000
    )
    , SUBFOLDER_NAME(
        false,
        "<b>Subfolder name:</b><br>Used in the output directory under TempFiles/route_mapping/. "
        "A timestamp is appended automatically.",
        LockMode::LOCK_WHILE_RUNNING,
        "route1",
        ""
    )
    , NOTIFICATIONS({
        &NOTIFICATION_PROGRAM_FINISH,
        &NOTIFICATION_ERROR_FATAL,
    })
{
    PA_ADD_OPTION(CAPTURE_INTERVAL_MS);
    PA_ADD_OPTION(MAX_CAPTURES);
    PA_ADD_OPTION(SUBFOLDER_NAME);
    PA_ADD_OPTION(NOTIFICATIONS);
}

void MappingMode::program(SingleSwitchProgramEnvironment& env, ProControllerContext& context){
    //  Build a unique per-session output directory.
    std::string subfolder = SUBFOLDER_NAME;
    if (subfolder.empty()){
        subfolder = "session";
    }
    //  Strip path separators from user input.
    for (char& c : subfolder){
        if (c == '/' || c == '\\'){
            c = '_';
        }
    }
    std::string dir = "TempFiles/route_mapping/" + subfolder + "_" + now_to_filestring() + "/";
    Filesystem::create_directories(Filesystem::Path(dir));

    env.log("Mapping Mode started. Output directory: " + dir, COLOR_BLUE);
    env.log(
        "Drive the character with your keyboard. Frames are captured every " +
        std::to_string((uint32_t)CAPTURE_INTERVAL_MS) + "ms (up to " +
        std::to_string((uint32_t)MAX_CAPTURES) + " frames). Press Stop when done.",
        COLOR_BLUE
    );

    uint32_t count = 0;
    while (count < MAX_CAPTURES){
        VideoSnapshot snap = env.console.video().snapshot();
        if (snap){
            char index_str[16];
            std::snprintf(index_str, sizeof(index_str), "%04u", count + 1);
            std::string path = dir + "frame_" + index_str + ".png";
            if (snap.frame->save(path)){
                count++;
                if (count == 1 || count % 10 == 0){
                    env.log(
                        "Captured " + std::to_string(count) + "/" +
                        std::to_string((uint32_t)MAX_CAPTURES) + " frames."
                    );
                }
            }else{
                env.log("Failed to save frame at " + path, COLOR_RED);
            }
        }
        context.wait_for(std::chrono::milliseconds((uint32_t)CAPTURE_INTERVAL_MS));
    }
    env.log(
        "Mapping Mode finished: captured " + std::to_string(count) +
        " frames in " + dir,
        COLOR_BLUE
    );
    send_program_finished_notification(env, NOTIFICATION_PROGRAM_FINISH);
}

}
}
}
