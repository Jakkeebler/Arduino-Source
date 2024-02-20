/*  Blueberry Catch Photo
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include "CommonFramework/Exceptions/OperationFailedException.h"
#include "CommonFramework/ImageTools/ImageFilter.h"
#include "CommonFramework/InferenceInfra/InferenceRoutines.h"
#include "CommonFramework/Tools/ErrorDumper.h"
#include "CommonFramework/OCR/OCR_NumberReader.h"
#include "CommonFramework/ImageTypes/ImageViewRGB32.h"
#include "CommonFramework/Exceptions/ProgramFinishedException.h"
#include "NintendoSwitch/Commands/NintendoSwitch_Commands_PushButtons.h"
#include "NintendoSwitch/Commands/NintendoSwitch_Commands_Superscalar.h"
#include "NintendoSwitch/NintendoSwitch_Settings.h"
#include "NintendoSwitch/Programs/NintendoSwitch_GameEntry.h"
#include "PokemonSV/Programs/PokemonSV_GameEntry.h"
#include "PokemonSV/Programs/PokemonSV_SaveGame.h"
#include "PokemonSwSh/Commands/PokemonSwSh_Commands_DateSpam.h"
#include "PokemonSwSh/Programs/PokemonSwSh_GameEntry.h"
#include "PokemonSV/PokemonSV_Settings.h"
#include "PokemonSV/Programs/PokemonSV_GameEntry.h"
#include "PokemonSV/Inference/Dialogs/PokemonSV_DialogDetector.h"
#include "PokemonSV/Inference/Battles/PokemonSV_NormalBattleMenus.h"
#include "PokemonSV/Inference/Battles/PokemonSV_EncounterWatcher.h"
#include "PokemonSV/Inference/PokemonSV_MainMenuDetector.h"
#include "PokemonSV/Inference/Dialogs/PokemonSV_DialogDetector.h"
#include "PokemonSV/Inference/Overworld/PokemonSV_OverworldDetector.h"
#include "PokemonSV/Programs/PokemonSV_Navigation.h"
#include "PokemonSwSh/Commands/PokemonSwSh_Commands_DateSpam.h"
#include "PokemonSV/Inference/PokemonSV_BlueberryQuestDetector.h"
#include "PokemonSV/Programs/Battles/PokemonSV_BasicCatcher.h"
#include "PokemonSV_BlueberryQuests.h"

#include<vector>

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonSV{

CameraAngle quest_photo_navi(const ProgramInfo& info, ConsoleHandle& console, BotBaseContext& context, BBQOption& BBQ_OPTIONS, BBQuests& current_quest) {
    CameraAngle angle = CameraAngle::none;

    //Navigate to target
    switch (current_quest) {
        case BBQuests::photo_fly: case BBQuests::photo_psychic:
            console.log("Photo: In-flight/Psychic");

            //Polar Rest Area - targeting Duosion fixed spawn
            open_map_from_overworld(info, console, context);
            pbf_move_left_joystick(context, 75, 0, 230, 20);
            pbf_press_button(context, BUTTON_ZL, 40, 100);
            fly_to_overworld_from_map(info, console, context);

            pbf_press_button(context, BUTTON_L, 10, 50);
            pbf_move_left_joystick(context, 128, 0, 230, 20);
            pbf_move_left_joystick(context, 0, 128, 250, 20);

            break;
        case BBQuests::photo_swim: case BBQuests::photo_water: case BBQuests::photo_polar:
            console.log("Photo: Swimming/Water/Polar");

            //Polar Outdoor Classroom 1 - fixed Horsea
            open_map_from_overworld(info, console, context);
            pbf_move_left_joystick(context, 0, 20, 150, 20);
            pbf_press_button(context, BUTTON_ZL, 40, 100);
            fly_to_overworld_from_map(info, console, context);

            pbf_wait(context, 200);
            context.wait_for_all_requests();

            pbf_press_button(context, BUTTON_L, 10, 50);
            pbf_move_left_joystick(context, 255, 50, 180, 20);

            angle = CameraAngle::down;

            break;
        case BBQuests::photo_coastal: case BBQuests::photo_grass: case BBQuests::photo_dragon:
            console.log("Photo: Coastal/Grass/Dragon");

            //Coastal Plaza - Exeggutor-A
            open_map_from_overworld(info, console, context);
            pbf_move_left_joystick(context, 180, 0, 200, 20);
            pbf_press_button(context, BUTTON_ZL, 40, 100);
            fly_to_overworld_from_map(info, console, context);

            pbf_move_left_joystick(context, 0, 115, 400, 20);

            //Jump down
            pbf_press_button(context, BUTTON_L | BUTTON_PLUS, 20, 105);
            ssf_press_button(context, BUTTON_B, 0, 100);
            ssf_press_button(context, BUTTON_B, 0, 20, 10);
            ssf_press_button(context, BUTTON_B, 0, 20);

            pbf_wait(context, 100);
            context.wait_for_all_requests();

            pbf_move_left_joystick(context, 128, 0, 150, 20);
            pbf_press_button(context, BUTTON_B, 20, 20);
            pbf_wait(context, 200);
            pbf_press_button(context, BUTTON_L | BUTTON_PLUS, 20, 105);
            context.wait_for_all_requests();

            break;
        case BBQuests::photo_canyon: case BBQuests::photo_ghost: case BBQuests::photo_ground:
            console.log("Photo: Canyon/Ghost/Ground");

            //Canyon Plaza - Golett
            open_map_from_overworld(info, console, context);
            pbf_move_left_joystick(context, 0, 255, 215, 20);
            pbf_press_button(context, BUTTON_ZL, 40, 100);
            fly_to_overworld_from_map(info, console, context);

            pbf_move_left_joystick(context, 210, 128, 10, 20);
            pbf_press_button(context, BUTTON_L | BUTTON_PLUS, 20, 105);

            //Jump, glide, fly
            ssf_press_button(context, BUTTON_B, 0, 100);
            ssf_press_button(context, BUTTON_B, 0, 20, 10);
            ssf_press_button(context, BUTTON_B, 0, 20);
            pbf_wait(context, 100);
            context.wait_for_all_requests();
            pbf_press_button(context, BUTTON_LCLICK, 50, 0);

            if (BBQ_OPTIONS.INVERTED_FLIGHT) {
                pbf_move_left_joystick(context, 128, 255, 600, 250);
            }
            else {
                pbf_move_left_joystick(context, 128, 0, 600, 250);
            }

            pbf_wait(context, 250);
            context.wait_for_all_requests();
            pbf_press_button(context, BUTTON_B, 20, 50);

            pbf_wait(context, 400);
            context.wait_for_all_requests();

            pbf_press_button(context, BUTTON_L | BUTTON_PLUS, 20, 105);

            break;
        case BBQuests::photo_savanna: case BBQuests::photo_normal: case BBQuests::photo_fire:
            console.log("Photo: Normal/Fire");

            //Savanna Plaza - Pride Rock
            open_map_from_overworld(info, console, context);
            pbf_move_left_joystick(context, 165, 255, 180, 20);
            pbf_press_button(context, BUTTON_ZL, 40, 100);
            fly_to_overworld_from_map(info, console, context);

            pbf_move_left_joystick(context, 255, 255, 10, 20);
            pbf_press_button(context, BUTTON_L | BUTTON_PLUS, 20, 105);

            ssf_press_button(context, BUTTON_B, 0, 100);
            ssf_press_button(context, BUTTON_B, 0, 20, 10);
            ssf_press_button(context, BUTTON_B, 0, 20);
            pbf_wait(context, 100);
            context.wait_for_all_requests();
            pbf_press_button(context, BUTTON_LCLICK, 50, 0);

            if (BBQ_OPTIONS.INVERTED_FLIGHT) {
                pbf_move_left_joystick(context, 128, 255, 600, 250);
            }
            else {
                pbf_move_left_joystick(context, 128, 0, 600, 250);
            }

            pbf_wait(context, 400);
            context.wait_for_all_requests();
            pbf_press_button(context, BUTTON_B, 20, 50);

            pbf_wait(context, 400);
            context.wait_for_all_requests();

            pbf_press_button(context, BUTTON_L | BUTTON_PLUS, 20, 105);
            pbf_move_left_joystick(context, 255, 128, 10, 20);

            angle = CameraAngle::up;

            break;
        case BBQuests::photo_bug: case BBQuests::photo_rock:
            console.log("Photo: Bug/Rock");

            //Kleavor
            open_map_from_overworld(info, console, context);
            pbf_move_left_joystick(context, 0, 255, 215, 20);
            pbf_press_button(context, BUTTON_ZL, 40, 100);
            fly_to_overworld_from_map(info, console, context);

            pbf_move_left_joystick(context, 205, 64, 20, 105);
            pbf_press_button(context, BUTTON_L | BUTTON_PLUS, 20, 105);

            //Jump, glide, fly
            ssf_press_button(context, BUTTON_B, 0, 100);
            ssf_press_button(context, BUTTON_B, 0, 20, 10);
            ssf_press_button(context, BUTTON_B, 0, 20);
            pbf_wait(context, 100);
            context.wait_for_all_requests();
            pbf_press_button(context, BUTTON_LCLICK, 50, 0);

            if (BBQ_OPTIONS.INVERTED_FLIGHT) {
                pbf_move_left_joystick(context, 128, 255, 1000, 250);
            }
            else {
                pbf_move_left_joystick(context, 128, 0, 1000, 250);
            }

            pbf_wait(context, 1500);
            context.wait_for_all_requests();
            pbf_press_button(context, BUTTON_B, 50, 375);
            pbf_wait(context, 300);
            context.wait_for_all_requests();

            pbf_press_button(context, BUTTON_L | BUTTON_PLUS, 20, 105);

            break;
        case BBQuests::photo_fairy:
            console.log("Photo: Fairy");

            /*
            //Polar Plaza - Snubbull (not the best area/target but not many fairies)
            open_map_from_overworld(info, console, context);
            pbf_move_left_joystick(context, 20, 25, 245, 20);
            pbf_press_button(context, BUTTON_ZL, 40, 100);
            fly_to_overworld_from_map(info, console, context);

            pbf_press_button(context, BUTTON_L | BUTTON_PLUS, 20, 105);

            ssf_press_button(context, BUTTON_B, 0, 100);
            ssf_press_button(context, BUTTON_B, 0, 20, 10);
            ssf_press_button(context, BUTTON_B, 0, 20);
            pbf_wait(context, 100);
            context.wait_for_all_requests();
            pbf_press_button(context, BUTTON_LCLICK, 50, 0);

            if (BBQ_OPTIONS.INVERTED_FLIGHT) {
                pbf_move_left_joystick(context, 128, 255, 2200, 250);
            }
            else {
                pbf_move_left_joystick(context, 128, 0, 2200, 250);
            }
            //Overshoot and then turn around.
            pbf_wait(context, 900);
            context.wait_for_all_requests();
            pbf_press_button(context, BUTTON_B, 20, 50);
            pbf_wait(context, 200);
            context.wait_for_all_requests();

            pbf_move_left_joystick(context, 128, 255, 20, 50);
            pbf_press_button(context, BUTTON_L | BUTTON_PLUS, 20, 105);
            //pbf_move_left_joystick(context, 110, 100, 20, 50);

            //angle_camera_down = true;
            */

            //Snubbull - Central plaza
            pbf_move_left_joystick(context, 0, 80, 10, 20);
            pbf_press_button(context, BUTTON_L | BUTTON_PLUS, 20, 105);

            //Jump, glide, fly
            ssf_press_button(context, BUTTON_B, 0, 100);
            ssf_press_button(context, BUTTON_B, 0, 20, 10);
            ssf_press_button(context, BUTTON_B, 0, 20);
            pbf_wait(context, 100);
            context.wait_for_all_requests();
            pbf_press_button(context, BUTTON_LCLICK, 50, 0);

            if (BBQ_OPTIONS.INVERTED_FLIGHT) {
                pbf_move_left_joystick(context, 128, 255, 2000, 250);
            }
            else {
                pbf_move_left_joystick(context, 128, 0, 2000, 250);
            }

            pbf_wait(context, 1500);
            context.wait_for_all_requests();
            pbf_press_button(context, BUTTON_B, 20, 50);

            pbf_wait(context, 200);
            context.wait_for_all_requests();

            pbf_press_button(context, BUTTON_PLUS, 20, 105);
            break;
        case BBQuests::photo_ice:
            console.log("Photo: Ice");

            //Snover - Start at central plaza, fly north-ish
            pbf_move_left_joystick(context, 0, 0, 10, 20);
            pbf_press_button(context, BUTTON_L | BUTTON_PLUS, 20, 105);

            //Jump, glide, fly
            ssf_press_button(context, BUTTON_B, 0, 100);
            ssf_press_button(context, BUTTON_B, 0, 20, 10);
            ssf_press_button(context, BUTTON_B, 0, 20);
            pbf_wait(context, 100);
            context.wait_for_all_requests();
            pbf_press_button(context, BUTTON_LCLICK, 50, 0);

            if (BBQ_OPTIONS.INVERTED_FLIGHT) {
                pbf_move_left_joystick(context, 128, 255, 1100, 250);
            }
            else {
                pbf_move_left_joystick(context, 128, 0, 1100, 250);
            }

            pbf_wait(context, 1700);
            context.wait_for_all_requests();
            pbf_press_button(context, BUTTON_B, 20, 50);

            pbf_wait(context, 200);
            context.wait_for_all_requests();

            pbf_press_button(context, BUTTON_PLUS, 20, 105);
            pbf_move_left_joystick(context, 0, 128, 20, 50);
            pbf_press_button(context, BUTTON_L, 20, 50);
        case BBQuests::photo_dark: case BBQuests::photo_fighting:
            console.log("Photo: Dark/Fighting");

            //Savanna Plaza - Fly towards rim of Canyon Biome
            open_map_from_overworld(info, console, context);
            pbf_move_left_joystick(context, 165, 255, 180, 20);
            pbf_press_button(context, BUTTON_ZL, 40, 100);
            fly_to_overworld_from_map(info, console, context);

            pbf_move_left_joystick(context, 255, 0, 10, 20);
            pbf_press_button(context, BUTTON_L | BUTTON_PLUS, 20, 105);

            ssf_press_button(context, BUTTON_B, 0, 100);
            ssf_press_button(context, BUTTON_B, 0, 20, 10);
            ssf_press_button(context, BUTTON_B, 0, 20);
            pbf_wait(context, 100);
            context.wait_for_all_requests();
            pbf_press_button(context, BUTTON_LCLICK, 50, 0);

            if (BBQ_OPTIONS.INVERTED_FLIGHT) {
                pbf_move_left_joystick(context, 128, 255, 1500, 250);
            }
            else {
                pbf_move_left_joystick(context, 128, 0, 1500, 250);
            }

            pbf_wait(context, 1900);
            context.wait_for_all_requests();

            pbf_move_left_joystick(context, 255, 128, 250, 20);
            pbf_press_button(context, BUTTON_L, 20, 50);

            pbf_wait(context, 300);
            context.wait_for_all_requests();

            pbf_press_button(context, BUTTON_B, 20, 50);
            pbf_wait(context, 300);
            context.wait_for_all_requests();

            pbf_press_button(context, BUTTON_PLUS, 20, 105);
            pbf_move_left_joystick(context, 0, 128, 10, 20);
            pbf_press_button(context, BUTTON_L, 20, 50);
            /*
            pbf_move_left_joystick(context, 0, 0, 200, 20);
            pbf_move_left_joystick(context, 128, 0, 200, 20);
            pbf_press_button(context, BUTTON_L, 20, 50);
            */
            break;
    }

    return angle;
}

void quest_photo(const ProgramInfo& info, ConsoleHandle& console, BotBaseContext& context, BBQOption& BBQ_OPTIONS, BBQuests& current_quest) {
    bool took_photo = false;
    CameraAngle move_camera = CameraAngle::none;

    while(!took_photo) {
        EncounterWatcher encounter_watcher(console, COLOR_RED);
        int ret = run_until(
            console, context,
            [&](BotBaseContext& context){

                move_camera = quest_photo_navi(info, console, context, BBQ_OPTIONS, current_quest);

                //Take photo.
                console.log("Taking photo.");
                PromptDialogWatcher photo_prompt(COLOR_RED);
                OverworldWatcher overworld(COLOR_BLUE);

                pbf_press_dpad(context, DPAD_DOWN, 50, 20);
                pbf_wait(context, 100);
                context.wait_for_all_requests();

                if (move_camera == CameraAngle::up) {
                    pbf_move_right_joystick(context, 128, 0, 50, 20);
                }
                else if (move_camera == CameraAngle::down) {
                    pbf_move_right_joystick(context, 128, 255, 50, 20);
                }
                context.wait_for_all_requests();

                pbf_press_button(context, BUTTON_A, 20, 50);

                int ret = wait_until(
                    console, context,
                    std::chrono::seconds(10),
                    { photo_prompt }
                );
                if (ret != 0) {
                    console.log("Photo not taken.");
                }

                //Mash B until overworld
                int exit = run_until(
                    console, context,
                    [&](BotBaseContext& context){
                        pbf_mash_button(context, BUTTON_B, 2000);
                    },
                    {{ overworld }}
                );
                if (exit == 0){
                    console.log("Overworld detected.");
                }
                took_photo = true;
                context.wait_for_all_requests();
            },
            {
                static_cast<VisualInferenceCallback&>(encounter_watcher),
                static_cast<AudioInferenceCallback&>(encounter_watcher),
            }
        );
        if (ret >= 0) {
            console.log("Battle menu detected.");
            encounter_watcher.throw_if_no_sound();

            bool is_shiny = (bool)encounter_watcher.shiny_screenshot();
            if (is_shiny) {
                console.log("Shiny detected!");
                pbf_press_button(context, BUTTON_CAPTURE, 2 * TICKS_PER_SECOND, 5 * TICKS_PER_SECOND);
                throw ProgramFinishedException();
            }
            else {
                console.log("Detected battle. Running from battle.");
                try{
                    //Smoke Ball or Flying type required due to Arena Trap
                    NormalBattleMenuWatcher battle_menu(COLOR_YELLOW);
                    battle_menu.move_to_slot(console, context, 3);
                    pbf_press_button(context, BUTTON_A, 10, 50);
                }catch (...){
                    console.log("Unable to flee.");
                    throw OperationFailedException(
                        ErrorReport::SEND_ERROR_REPORT, console,
                        "Unable to flee!",
                        true
                    );
                }
            }
        }
    }
    context.wait_for_all_requests();
    return_to_plaza(info, console, context);
}

void quest_catch_navi(const ProgramInfo& info, ConsoleHandle& console, BotBaseContext& context, BBQOption& BBQ_OPTIONS, BBQuests& current_quest) {
    switch (current_quest) {
        case BBQuests::catch_any: case BBQuests::catch_normal: case BBQuests::catch_fire:
            console.log("Catch: Any/Normal/Fire.");

            //Savanna Plaza - Pride Rock
            open_map_from_overworld(info, console, context);
            pbf_move_left_joystick(context, 165, 255, 180, 20);
            pbf_press_button(context, BUTTON_ZL, 40, 100);
            fly_to_overworld_from_map(info, console, context);

            //pbf_move_left_joystick(context, 255, 255, 10, 20);
            pbf_move_left_joystick(context, 220, 255, 10, 20);
            pbf_press_button(context, BUTTON_L | BUTTON_PLUS, 20, 105);

            ssf_press_button(context, BUTTON_B, 0, 100);
            ssf_press_button(context, BUTTON_B, 0, 20, 10);
            ssf_press_button(context, BUTTON_B, 0, 20);
            pbf_wait(context, 100);
            context.wait_for_all_requests();
            pbf_press_button(context, BUTTON_LCLICK, 50, 0);

            if (BBQ_OPTIONS.INVERTED_FLIGHT) {
                pbf_move_left_joystick(context, 130, 255, 600, 250);
            }
            else {
                pbf_move_left_joystick(context, 130, 0, 600, 250);
            }

            pbf_wait(context, 400);
            context.wait_for_all_requests();
            pbf_press_button(context, BUTTON_B, 20, 50);
            pbf_wait(context, 400);
            context.wait_for_all_requests();

            pbf_press_button(context, BUTTON_PLUS, 20, 105);
            pbf_move_left_joystick(context, 255, 128, 20, 50);

            pbf_press_button(context, BUTTON_L, 20, 50);
            pbf_move_left_joystick(context, 128, 0, 100, 50);
            pbf_move_left_joystick(context, 0, 0, 20, 50);
            pbf_press_button(context, BUTTON_L, 20, 50);

            ssf_press_button(context, BUTTON_ZR, 0, 200);
            ssf_press_button(context, BUTTON_ZL, 100, 50);
            ssf_press_button(context, BUTTON_ZL, 150, 50);

            pbf_wait(context, 300);
            context.wait_for_all_requests();
            break;

        case BBQuests::catch_psychic:
            console.log("Catch: Psychic.");
            //Polar Rest Area - targeting Duosion fixed spawn
            open_map_from_overworld(info, console, context);
            pbf_move_left_joystick(context, 75, 0, 230, 20);
            pbf_press_button(context, BUTTON_ZL, 40, 100);
            fly_to_overworld_from_map(info, console, context);

            pbf_press_button(context, BUTTON_L, 10, 50);
            pbf_move_left_joystick(context, 128, 0, 230, 20);
            pbf_move_left_joystick(context, 0, 128, 250, 20);

            pbf_press_button(context, BUTTON_L, 20, 50);
            pbf_move_left_joystick(context, 128, 0, 150, 20);

            ssf_press_button(context, BUTTON_ZR, 0, 200);
            ssf_press_button(context, BUTTON_ZL, 100, 50);
            ssf_press_button(context, BUTTON_ZL, 150, 50);

            pbf_wait(context, 300);
            context.wait_for_all_requests();
            break;

        case BBQuests::catch_grass: case BBQuests::catch_dragon:
            console.log("Catch: Grass/Dragon");

            //Coastal Plaza - Exeggutor-A
            open_map_from_overworld(info, console, context);
            pbf_move_left_joystick(context, 180, 0, 200, 20);
            pbf_press_button(context, BUTTON_ZL, 40, 100);
            fly_to_overworld_from_map(info, console, context);

            pbf_move_left_joystick(context, 0, 115, 400, 20);

            //Jump down
            pbf_press_button(context, BUTTON_L | BUTTON_PLUS, 20, 105);
            ssf_press_button(context, BUTTON_B, 0, 100);
            ssf_press_button(context, BUTTON_B, 0, 20, 10);
            ssf_press_button(context, BUTTON_B, 0, 20);

            pbf_wait(context, 100);
            context.wait_for_all_requests();

            pbf_move_left_joystick(context, 128, 0, 350, 20);
            pbf_press_button(context, BUTTON_B, 20, 20);
            pbf_wait(context, 200);
            pbf_press_button(context, BUTTON_L | BUTTON_PLUS, 20, 105);

            ssf_press_button(context, BUTTON_ZR, 0, 200);
            ssf_press_button(context, BUTTON_ZL, 100, 50);
            ssf_press_button(context, BUTTON_ZL, 150, 50);

            pbf_wait(context, 300);
            context.wait_for_all_requests();

            break;
        case BBQuests::catch_ghost: case BBQuests::catch_ground:
            console.log("Catch: Ghost/Ground");

            //Canyon Plaza - Golett
            open_map_from_overworld(info, console, context);
            pbf_move_left_joystick(context, 0, 255, 215, 20);
            pbf_press_button(context, BUTTON_ZL, 40, 100);
            fly_to_overworld_from_map(info, console, context);

            pbf_move_left_joystick(context, 210, 128, 10, 20);
            pbf_press_button(context, BUTTON_L | BUTTON_PLUS, 20, 105);

            //Jump, glide, fly
            ssf_press_button(context, BUTTON_B, 0, 100);
            ssf_press_button(context, BUTTON_B, 0, 20, 10);
            ssf_press_button(context, BUTTON_B, 0, 20);
            pbf_wait(context, 100);
            context.wait_for_all_requests();
            pbf_press_button(context, BUTTON_LCLICK, 50, 0);

            if (BBQ_OPTIONS.INVERTED_FLIGHT) {
                pbf_move_left_joystick(context, 128, 255, 600, 250);
            }
            else {
                pbf_move_left_joystick(context, 128, 0, 600, 250);
            }

            pbf_wait(context, 300);
            context.wait_for_all_requests();
            pbf_press_button(context, BUTTON_B, 20, 50);

            pbf_wait(context, 400);
            context.wait_for_all_requests();

            pbf_press_button(context, BUTTON_L | BUTTON_PLUS, 20, 105);

            pbf_move_left_joystick(context, 0, 0, 10, 20);
            pbf_press_button(context, BUTTON_L, 20, 50);
            pbf_move_left_joystick(context, 128, 0, 50, 20);

            ssf_press_button(context, BUTTON_ZR, 0, 200);
            ssf_press_button(context, BUTTON_ZL, 100, 50);
            ssf_press_button(context, BUTTON_ZL, 150, 50);

            pbf_wait(context, 300);
            context.wait_for_all_requests();

            break;
        case BBQuests::catch_fairy:
            console.log("Catch: Fairy");

            //Polar Plaza - Snubbull
            open_map_from_overworld(info, console, context);
            pbf_move_left_joystick(context, 20, 25, 245, 20);
            pbf_press_button(context, BUTTON_ZL, 40, 100);
            fly_to_overworld_from_map(info, console, context);

            pbf_press_button(context, BUTTON_L | BUTTON_PLUS, 20, 105);

            ssf_press_button(context, BUTTON_B, 0, 100);
            ssf_press_button(context, BUTTON_B, 0, 20, 10);
            ssf_press_button(context, BUTTON_B, 0, 20);
            pbf_wait(context, 100);
            context.wait_for_all_requests();
            pbf_press_button(context, BUTTON_LCLICK, 50, 0);

            if (BBQ_OPTIONS.INVERTED_FLIGHT) {
                pbf_move_left_joystick(context, 128, 255, 2100, 250);
            }
            else {
                pbf_move_left_joystick(context, 128, 0, 2100, 250);
            }

            //Overshoot and then turn around.
            pbf_wait(context, 900);
            context.wait_for_all_requests();
            pbf_press_button(context, BUTTON_B, 20, 50);
            pbf_wait(context, 200);
            context.wait_for_all_requests();

            pbf_move_left_joystick(context, 128, 255, 20, 50);
            pbf_press_button(context, BUTTON_L | BUTTON_PLUS, 20, 105);

            pbf_move_left_joystick(context, 128, 0, 50, 50);

            ssf_press_button(context, BUTTON_ZR, 0, 400);
            ssf_press_button(context, BUTTON_ZL, 100, 50);
            ssf_press_button(context, BUTTON_ZL, 150, 50);

            pbf_wait(context, 300);
            context.wait_for_all_requests();
        case BBQuests::catch_ice:
            console.log("Catch: Ice");

            //Start at central plaza, fly north-ish
            pbf_move_left_joystick(context, 0, 0, 10, 20);
            pbf_press_button(context, BUTTON_L | BUTTON_PLUS, 20, 105);

            //Jump, glide, fly
            ssf_press_button(context, BUTTON_B, 0, 100);
            ssf_press_button(context, BUTTON_B, 0, 20, 10);
            ssf_press_button(context, BUTTON_B, 0, 20);
            pbf_wait(context, 100);
            context.wait_for_all_requests();
            pbf_press_button(context, BUTTON_LCLICK, 50, 0);

            if (BBQ_OPTIONS.INVERTED_FLIGHT) {
                pbf_move_left_joystick(context, 128, 255, 1100, 250);
            }
            else {
                pbf_move_left_joystick(context, 128, 0, 1100, 250);
            }

            //pbf_wait(context, 1500); photo
            pbf_wait(context, 1700);
            context.wait_for_all_requests();
            pbf_press_button(context, BUTTON_B, 20, 50);

            pbf_wait(context, 200);
            context.wait_for_all_requests();

            pbf_press_button(context, BUTTON_PLUS, 20, 105);
            pbf_move_left_joystick(context, 0, 128, 20, 50);
            pbf_press_button(context, BUTTON_L, 20, 50);

            ssf_press_button(context, BUTTON_ZR, 0, 200);
            ssf_press_button(context, BUTTON_ZL, 100, 50);
            ssf_press_button(context, BUTTON_ZL, 150, 50);

            break;

        case BBQuests::catch_dark: case BBQuests::catch_fighting:
            console.log("Catch: Dark/Fighting");

            //Savanna Plaza - Fly towards rim of Canyon Biome
            open_map_from_overworld(info, console, context);
            pbf_move_left_joystick(context, 165, 255, 180, 20);
            pbf_press_button(context, BUTTON_ZL, 40, 100);
            fly_to_overworld_from_map(info, console, context);

            pbf_move_left_joystick(context, 255, 0, 10, 20);
            pbf_press_button(context, BUTTON_L | BUTTON_PLUS, 20, 105);

            ssf_press_button(context, BUTTON_B, 0, 100);
            ssf_press_button(context, BUTTON_B, 0, 20, 10);
            ssf_press_button(context, BUTTON_B, 0, 20);
            pbf_wait(context, 100);
            context.wait_for_all_requests();
            pbf_press_button(context, BUTTON_LCLICK, 50, 0);

            if (BBQ_OPTIONS.INVERTED_FLIGHT) {
                pbf_move_left_joystick(context, 128, 255, 1500, 250);
            }
            else {
                pbf_move_left_joystick(context, 128, 0, 1500, 250);
            }

            pbf_wait(context, 2800);
            context.wait_for_all_requests();

            /*
            pbf_move_left_joystick(context, 255, 128, 250, 20);
            pbf_press_button(context, BUTTON_L, 20, 50);
            pbf_wait(context, 300);
            context.wait_for_all_requests();
            */

            pbf_press_button(context, BUTTON_B, 20, 50);
            pbf_wait(context, 300);
            context.wait_for_all_requests();

            pbf_press_button(context, BUTTON_PLUS, 20, 105);
            //pbf_move_left_joystick(context, 0, 128, 10, 20);
            //pbf_press_button(context, BUTTON_L, 20, 50);
            /*
            pbf_move_left_joystick(context, 0, 0, 200, 20);
            pbf_move_left_joystick(context, 128, 0, 200, 20);
            pbf_press_button(context, BUTTON_L, 20, 50);
            */
            break;

        case BBQuests::catch_bug: case BBQuests::catch_rock:
            open_map_from_overworld(info, console, context);
            pbf_move_left_joystick(context, 0, 255, 215, 20);
            pbf_press_button(context, BUTTON_ZL, 40, 100);
            fly_to_overworld_from_map(info, console, context);

            pbf_move_left_joystick(context, 205, 64, 20, 105);
            pbf_press_button(context, BUTTON_L | BUTTON_PLUS, 20, 105);

            ssf_press_button(context, BUTTON_B, 0, 100);
            ssf_press_button(context, BUTTON_B, 0, 20, 10);
            ssf_press_button(context, BUTTON_B, 0, 20);
            pbf_wait(context, 100);
            context.wait_for_all_requests();
            pbf_press_button(context, BUTTON_LCLICK, 50, 0);

            if (BBQ_OPTIONS.INVERTED_FLIGHT) {
                pbf_move_left_joystick(context, 128, 255, 1000, 250);
            }
            else {
                pbf_move_left_joystick(context, 128, 0, 1000, 250);
            }
            pbf_wait(context, 1650);
            context.wait_for_all_requests();

            pbf_press_button(context, BUTTON_B, 50, 375);
            pbf_wait(context, 500);
            context.wait_for_all_requests();
            break;
    }
}

void quest_catch_handle_battle(const ProgramInfo& info, ConsoleHandle& console, BotBaseContext& context, BBQOption& BBQ_OPTIONS, BBQuests& current_quest) {
    console.log("Catching Pokemon.");
    AdvanceDialogWatcher advance_dialog(COLOR_MAGENTA);
    OverworldWatcher overworld(COLOR_BLUE);
    PromptDialogWatcher add_to_party(COLOR_PURPLE);

    uint8_t switch_party_slot = 1;
    bool quickball_thrown = false;

    int ret2 = run_until(
        console, context,
        [&](BotBaseContext& context) {
            while (true) {
                //Check that battle menu appears - this is in case of swapping pokemon
                NormalBattleMenuWatcher menu_before_throw(COLOR_YELLOW);
                int bMenu = wait_until(
                    console, context,
                    std::chrono::seconds(15),
                    { menu_before_throw }
                );
                if (bMenu < 0) {
                    console.log("Unable to find menu_before_throw.");
                    throw OperationFailedException(
                        ErrorReport::SEND_ERROR_REPORT, console,
                        "Unable to find menu_before_throw.",
                        true
                    );
                }
                    
                //Quick ball occurs before anything else in battle.
                //Do not throw if target is a tera pokemon
                if (BBQ_OPTIONS.QUICKBALL && !quickball_thrown) {
                    console.log("Quick Ball option checked. Throwing Quick Ball.");
                    BattleBallReader reader(console, BBQ_OPTIONS.LANGUAGE);
                    std::string ball_reader = "";
                    WallClock start = current_time();

                    console.log("Opening ball menu...");
                    while (ball_reader == "") {
                        if (current_time() - start > std::chrono::minutes(2)) {
                            console.log("Timed out trying to read ball after 2 minutes.", COLOR_RED);
                            throw OperationFailedException(
                                ErrorReport::SEND_ERROR_REPORT, console,
                                "Timed out trying to read ball after 2 minutes.",
                                true
                            );
                        }

                        //Mash B to exit anything else
                        pbf_mash_button(context, BUTTON_B, 125);
                        context.wait_for_all_requests();

                        //Press X to open Ball menu
                        pbf_press_button(context, BUTTON_X, 20, 100);
                        context.wait_for_all_requests();

                        VideoSnapshot screen = console.video().snapshot();
                        ball_reader = reader.read_ball(screen);
                    }

                    console.log("Selecting Quick Ball.");
                    int quantity = move_to_ball(reader, console, context, "quick-ball");
                    if (quantity == 0) {
                        //Stop so user can check they have quick balls.
                        console.log("Unable to find Quick Ball on turn 1.");
                        throw OperationFailedException(
                            ErrorReport::SEND_ERROR_REPORT, console,
                            "Unable to find Quick Ball on turn 1.",
                            true
                        );
                    }
                    if (quantity < 0) {
                        console.log("Unable to read ball quantity.", COLOR_RED);
                    }

                    //Throw ball
                    console.log("Throwing Quick Ball.");
                    pbf_mash_button(context, BUTTON_A, 150);
                    context.wait_for_all_requests();

                    quickball_thrown = true;
                    pbf_mash_button(context, BUTTON_B, 900);
                    context.wait_for_all_requests();
                }
                else {
                    //if (tera_target) {



                            
                    //}




                    BattleBallReader reader(console, BBQ_OPTIONS.LANGUAGE);
                    std::string ball_reader = "";
                    WallClock start = current_time();

                    console.log("Opening ball menu...");
                    while (ball_reader == "") {
                        if (current_time() - start > std::chrono::minutes(2)) {
                            console.log("Timed out trying to read ball after 2 minutes.", COLOR_RED);
                            throw OperationFailedException(
                                ErrorReport::SEND_ERROR_REPORT, console,
                                "Timed out trying to read ball after 2 minutes.",
                                true
                            );
                        }

                        //Mash B to exit anything else
                        pbf_mash_button(context, BUTTON_B, 125);
                        context.wait_for_all_requests();

                        //Press X to open Ball menu
                        pbf_press_button(context, BUTTON_X, 20, 100);
                        context.wait_for_all_requests();

                        VideoSnapshot screen = console.video().snapshot();
                        ball_reader = reader.read_ball(screen);
                    }

                    console.log("Selecting ball.");
                    int quantity = move_to_ball(reader, console, context, BBQ_OPTIONS.BALL_SELECT.slug());
                    if (quantity == 0) {
                        console.log("Unable to find appropriate ball/out of balls.");
                        break;
                    }
                    if (quantity < 0) {
                        console.log("Unable to read ball quantity.", COLOR_RED);
                    }

                    //Throw ball
                    console.log("Throwing selected ball.");
                    pbf_mash_button(context, BUTTON_A, 150);
                    context.wait_for_all_requests();

                    //Check for battle menu
                    //If found use _fourth_ attack this turn!
                    NormalBattleMenuWatcher battle_menu(COLOR_YELLOW);
                    int ret = wait_until(
                        console, context,
                        std::chrono::seconds(4),
                        { battle_menu }
                    );
                    if (ret == 0) {
                        console.log("Battle menu detected early. Using fourth attack.");
                        pbf_mash_button(context, BUTTON_A, 250);
                        context.wait_for_all_requests();

                        MoveSelectWatcher move_watcher(COLOR_BLUE);
                        MoveSelectDetector move_select(COLOR_BLUE);

                        int ret_move_select = run_until(
                        console, context,
                        [&](BotBaseContext& context) {
                            pbf_press_button(context, BUTTON_A, 10, 50);
                            pbf_wait(context, 100);
                            context.wait_for_all_requests();
                        },
                        { move_watcher }
                        );
                        if (ret_move_select != 0) {
                            console.log("Could not find move select.");
                        }
                        else {
                            console.log("Move select found!");
                        }

                        context.wait_for_all_requests();
                        move_select.move_to_slot(console, context, 3);
                        pbf_mash_button(context, BUTTON_A, 150);
                        pbf_wait(context, 100);
                        context.wait_for_all_requests();

                        //Check for battle menu
                        //If found after a second, assume out of PP and stop as this is a setup issue
                        //None of the target pokemon for this program have disable, taunt, etc.
                        NormalBattleMenuWatcher battle_menu2(COLOR_YELLOW);
                        int ret3 = wait_until(
                            console, context,
                            std::chrono::seconds(4),
                            { battle_menu2 }
                        );
                        if (ret3 == 0) {
                            console.log("Battle menu detected early. Out of PP/No move in slot, please check your setup.");
                            throw OperationFailedException(
                                ErrorReport::SEND_ERROR_REPORT, console,
                                "Battle menu detected early. Out of PP, please check your setup.",
                                true
                            );
                        }
                    }
                    else {
                        //Wild pokemon's turn/wait for catch animation
                        pbf_mash_button(context, BUTTON_B, 900);
                        context.wait_for_all_requests();
                    }
                }

                NormalBattleMenuWatcher battle_menu(COLOR_YELLOW);
                OverworldWatcher overworld(COLOR_BLUE);
                SwapMenuWatcher fainted(COLOR_YELLOW);
                int ret2 = wait_until(
                    console, context,
                    std::chrono::seconds(60),
                    { battle_menu, fainted }
                );
                switch (ret2) {
                case 0:
                    console.log("Battle menu detected, continuing.");
                    break;
                case 1:
                    console.log("Detected fainted Pokemon. Switching to next living Pokemon...");
                    if (fainted.move_to_slot(console, context, switch_party_slot)) {
                        pbf_mash_button(context, BUTTON_A, 3 * TICKS_PER_SECOND);
                        context.wait_for_all_requests();
                        switch_party_slot++;
                    }
                    break;
                default:
                    console.log("Invalid state ret2 run_battle.");
                    throw OperationFailedException(
                        ErrorReport::SEND_ERROR_REPORT, console,
                        "Invalid state ret2 run_battle.",
                        true
                    );
                }

            }
        },
        { advance_dialog, overworld, add_to_party }
        );

    switch (ret2) {
    case 0:
        console.log("Advance Dialog detected.");
        press_Bs_to_back_to_overworld(info, console, context);
        break;
    case 1:
        console.log("Overworld detected. Fainted?");
        break;
    case 2:
        console.log("Prompt dialog detected.");
        press_Bs_to_back_to_overworld(info, console, context);
        break;
    default:
        console.log("Invalid state in run_battle().");
        throw OperationFailedException(
            ErrorReport::SEND_ERROR_REPORT, console,
            "Invalid state in run_battle().",
            true
        );
    }
}

void quest_catch(const ProgramInfo& info, ConsoleHandle& console, BotBaseContext& context, BBQOption& BBQ_OPTIONS, BBQuests& current_quest) {
    EncounterWatcher encounter_watcher(console, COLOR_RED);

    //bool tera_target = false;

    //Navigate to target and start battle
    int ret = run_until(
        console, context,
        [&](BotBaseContext& context) {
            
            quest_catch_navi(info, console, context, BBQ_OPTIONS, current_quest);

            NormalBattleMenuWatcher battle_menu(COLOR_YELLOW);
            int ret2 = wait_until(
                console, context,
                std::chrono::seconds(25),
                { battle_menu }
            );
            if (ret2 != 0) {
                console.log("Did not enter battle. Did target spawn?");
            }
        },
        {
            static_cast<VisualInferenceCallback&>(encounter_watcher),
            static_cast<AudioInferenceCallback&>(encounter_watcher),
        }
        );
    if (ret == 0) {
        console.log("Battle menu detected.");
    }

    encounter_watcher.throw_if_no_sound();

    bool is_shiny = (bool)encounter_watcher.shiny_screenshot();
    if (is_shiny) {
        console.log("Shiny detected!");
        pbf_press_button(context, BUTTON_CAPTURE, 2 * TICKS_PER_SECOND, 5 * TICKS_PER_SECOND);
        throw ProgramFinishedException();
    } else {
        quest_catch_handle_battle(info, console, context, BBQ_OPTIONS, current_quest);
    }
    //pbf_press_button(context, BUTTON_PLUS, 20, 105);
    return_to_plaza(info, console, context);

    //Day skip and attempt to respawn fixed encounters
    pbf_press_button(context, BUTTON_HOME, 20, GameSettings::instance().GAME_TO_HOME_DELAY);
    home_to_date_time(context, true, true);
    PokemonSwSh::roll_date_forward_1(context, true);
    resume_game_from_home(console, context);

    //Heal up and then reset position again.
    OverworldWatcher done_healing(COLOR_BLUE);
    pbf_move_left_joystick(context, 128, 0, 100, 20);

    pbf_mash_button(context, BUTTON_A, 300);
    context.wait_for_all_requests();

    int exit = run_until(
        console, context,
        [&](BotBaseContext& context){
            pbf_mash_button(context, BUTTON_B, 2000);
        },
        {{ done_healing }}
    );
    if (exit == 0){
        console.log("Overworld detected.");
    }
    open_map_from_overworld(info, console, context);
    pbf_press_button(context, BUTTON_ZL, 40, 100);
    fly_to_overworld_from_map(info, console, context);
    context.wait_for_all_requests();
}

}
}
}
