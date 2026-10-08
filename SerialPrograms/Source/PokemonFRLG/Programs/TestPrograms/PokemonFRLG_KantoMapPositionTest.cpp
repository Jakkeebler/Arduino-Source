/*  Kanto Map Position Test
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include "Common/Cpp/Color.h"
#include "CommonFramework/Notifications/ProgramNotifications.h"
#include "CommonFramework/VideoPipeline/VideoFeed.h"
#include "Pokemon/Pokemon_Strings.h"
#include "PokemonFRLG/Inference/Map/PokemonFRLG_KantoMapDetector.h"
#include "PokemonFRLG_KantoMapPositionTest.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

KantoMapPositionTest_Descriptor::KantoMapPositionTest_Descriptor()
    : SingleSwitchProgramDescriptor(
        "PokemonFRLG:KantoMapPositionTest",
        Pokemon::STRING_POKEMON + " FRLG", "Kanto Map Position Test",
        "",
        "Drive the character with your keyboard. The program continuously "
        "matches the live video against the Pallet/Route1/Viridian map images "
        "and logs the detected map and tile coordinates so you can verify the "
        "position detector is accurate.",
        ProgramControllerClass::StandardController_NoRestrictions,
        FeedbackType::REQUIRED,
        AllowCommandsWhenRunning::ENABLE_COMMANDS,
        {}
    )
{}

KantoMapPositionTest::KantoMapPositionTest()
    : POLL_INTERVAL_MS(
        "<b>Poll interval (ms):</b><br>How often to take a snapshot and "
        "report position. Faster = more log spam.",
        LockMode::UNLOCK_WHILE_RUNNING,
        1000, 200, 10000
    )
    , NOTIFICATIONS({
        &NOTIFICATION_PROGRAM_FINISH,
        &NOTIFICATION_ERROR_FATAL,
    })
{
    PA_ADD_OPTION(POLL_INTERVAL_MS);
    PA_ADD_OPTION(NOTIFICATIONS);
}

void KantoMapPositionTest::program(SingleSwitchProgramEnvironment& env, ProControllerContext& context){
    env.log("Kanto Map Position Test started. Drive the character; positions "
            "will be logged on each poll.", COLOR_BLUE);

    const KantoMapDetector& detector = KantoMapDetector::instance();

    int last_x = -999, last_y = -999;
    bool first = true;

    while (true){
        VideoSnapshot snap = env.console.video().snapshot();
        if (snap){
            std::optional<KantoPosition> pos = detector.locate(*snap.frame);
            if (pos){
                if (first || pos->tile_x != last_x || pos->tile_y != last_y){
                    char buf[200];
                    std::snprintf(
                        buf, sizeof(buf),
                        "%s tile (%d, %d) conf=%.3f",
                        kanto_region_name(kanto_region_at(pos->tile_x, pos->tile_y)),
                        pos->tile_x, pos->tile_y, pos->confidence
                    );
                    env.log(buf, COLOR_BLUE);
                    last_x = pos->tile_x;
                    last_y = pos->tile_y;
                    first = false;
                }
            }else{
                std::optional<KantoPosition> raw = detector.locate(*snap.frame, -2.0);
                if (raw){
                    char buf[200];
                    std::snprintf(
                        buf, sizeof(buf),
                        "Off-map (best tile (%d,%d) conf=%.3f - below threshold).",
                        raw->tile_x, raw->tile_y, raw->confidence
                    );
                    env.log(buf, COLOR_YELLOW);
                }else{
                    env.log("Off-map (no snapshot).", COLOR_YELLOW);
                }
                last_x = -1;
                first = false;
            }
        }
        context.wait_for(std::chrono::milliseconds((uint32_t)POLL_INTERVAL_MS));
    }
}

}
}
}
