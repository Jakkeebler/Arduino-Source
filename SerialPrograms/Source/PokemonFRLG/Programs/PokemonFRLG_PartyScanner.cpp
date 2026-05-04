/*  Pokemon FRLG Party Scanner
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include <chrono>
#include "Common/Cpp/Color.h"
#include "CommonFramework/Exceptions/OperationFailedException.h"
#include "CommonFramework/VideoPipeline/VideoFeed.h"
#include "CommonFramework/VideoPipeline/VideoOverlayScopes.h"
#include "CommonTools/Async/InferenceRoutines.h"
#include "NintendoSwitch/Commands/NintendoSwitch_Commands_PushButtons.h"
#include "NintendoSwitch/Commands/NintendoSwitch_Commands_Superscalar.h"
#include "NintendoSwitch/NintendoSwitch_ConsoleHandle.h"
#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "PokemonFRLG/Inference/Dialogs/PokemonFRLG_PartyDialogs.h"
#include "PokemonFRLG/Inference/Menus/PokemonFRLG_PartyMenuDetector.h"
#include "PokemonFRLG/Inference/Menus/PokemonFRLG_SummaryDetector.h"
#include "PokemonFRLG/Inference/Menus/PokemonFRLG_StartMenuDetector.h"
#include "PokemonFRLG/Programs/PokemonFRLG_StartMenuNavigation.h"
#include "PokemonFRLG/PokemonFRLG_Navigation.h"
#include "PokemonFRLG/Resources/PokemonFRLG_SpeciesData.h"
#include "PokemonFRLG_PartyScanner.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{

using namespace std::chrono_literals;


namespace{

//  Move the party-menu cursor from `from` to `to` (both 1-indexed) using the
//  layout convention from switch_party_lead_overworld:
//    Slot 1 sits alone in the left column.
//    Slots 2..6 sit stacked in the right column (top -> bottom = 2..6).
//
//  Joystick convention (per PokemonFRLG_Navigation.cpp):
//    {+1, 0} = right,  {-1, 0} = left
//    {0, -1} = down (south),  {0, +1} = up (north)
void move_cursor_between_slots(ProControllerContext& context, int from, int to){
    if (from == to){
        return;
    }
    //  Right-column index 0 == slot 2, 1 == slot 3, ..., 4 == slot 6.
    auto right_col_index = [](int slot) -> int { return slot - 2; };

    if (from == 1){
        //  Move into the right column at slot 2.
        pbf_move_left_joystick(context, {+1, 0}, 200ms, 300ms);
        from = 2;
        if (from == to){
            return;
        }
    }
    if (to == 1){
        //  From the right column: move up to slot 2 if needed, then left.
        for (int i = right_col_index(from); i > 0; i--){
            pbf_move_left_joystick(context, {0, +1}, 200ms, 300ms);  //  up
        }
        pbf_move_left_joystick(context, {-1, 0}, 200ms, 300ms);      //  left
        return;
    }

    //  Both within the right column: move down or up by index delta.
    int delta = right_col_index(to) - right_col_index(from);
    if (delta > 0){
        for (int i = 0; i < delta; i++){
            pbf_move_left_joystick(context, {0, -1}, 200ms, 300ms);  //  down
        }
    }else{
        for (int i = 0; i < -delta; i++){
            pbf_move_left_joystick(context, {0, +1}, 200ms, 300ms);  //  up
        }
    }
}

//  Open the SUMMARY/SWITCH/ITEM/CANCEL submenu for the slot the cursor is on.
//  Cursor in the submenu defaults to the first option (SUMMARY in standard
//  party menus; SHIFT/SWITCH appears in some menu contexts but FRLG's lead
//  party menu starts on SUMMARY when accessed from the overworld start menu).
void open_slot_submenu(ConsoleHandle& console, ProControllerContext& context){
    PartySelectionWatcher submenu_open(COLOR_RED);
    context.wait_for_all_requests();
    int ret = run_until<ProControllerContext>(
        console, context,
        [](ProControllerContext& ctx){
            pbf_press_button(ctx, BUTTON_A, 200ms, 1800ms);
        },
        { submenu_open }
    );
    if (ret < 0){
        OperationFailedException::fire(
            ErrorReport::SEND_ERROR_REPORT,
            "scan_party: failed to open party slot submenu.",
            console
        );
    }
}

//  Press A on SUMMARY (default cursor position in the submenu) and wait for
//  the Summary page-1 detector to fire.
void enter_summary(ConsoleHandle& console, ProControllerContext& context){
    SummaryWatcher summary_page1(COLOR_RED);
    context.wait_for_all_requests();
    int ret = run_until<ProControllerContext>(
        console, context,
        [](ProControllerContext& ctx){
            pbf_press_button(ctx, BUTTON_A, 200ms, 2000ms);
        },
        { summary_page1 }
    );
    if (ret < 0){
        OperationFailedException::fire(
            ErrorReport::SEND_ERROR_REPORT,
            "scan_party: failed to enter Summary page 1.",
            console
        );
    }
}

//  From Summary page 1, advance to page 3 (Skills/Moves).
void navigate_to_page3(ConsoleHandle& console, ProControllerContext& context){
    SummaryPage3Watcher page3(COLOR_RED);
    context.wait_for_all_requests();
    int ret = run_until<ProControllerContext>(
        console, context,
        [](ProControllerContext& ctx){
            pbf_press_dpad(ctx, DPAD_RIGHT, 100ms, 600ms);
            pbf_press_dpad(ctx, DPAD_RIGHT, 100ms, 600ms);
        },
        { page3 }
    );
    if (ret < 0){
        OperationFailedException::fire(
            ErrorReport::SEND_ERROR_REPORT,
            "scan_party: failed to reach Summary page 3.",
            console
        );
    }
}

//  From the Summary screen, back out to the party menu.
void exit_summary_to_party(ConsoleHandle& console, ProControllerContext& context){
    PartyMenuWatcher party_menu(COLOR_RED);
    context.wait_for_all_requests();
    int ret = run_until<ProControllerContext>(
        console, context,
        [](ProControllerContext& ctx){
            pbf_press_button(ctx, BUTTON_B, 200ms, 1500ms);
        },
        { party_menu }
    );
    if (ret < 0){
        //  Some FRLG variants drop us into the submenu first; press B again.
        ret = run_until<ProControllerContext>(
            console, context,
            [](ProControllerContext& ctx){
                pbf_press_button(ctx, BUTTON_B, 200ms, 1500ms);
            },
            { party_menu }
        );
        if (ret < 0){
            OperationFailedException::fire(
                ErrorReport::SEND_ERROR_REPORT,
                "scan_party: failed to return to party menu after Summary.",
                console
            );
        }
    }
}

//  Read both pages of the Summary for the currently-selected slot, then
//  back out to the party menu. Cursor on the party menu must be on the
//  target slot before calling.
PartyScanResult read_current_slot(
    SingleSwitchProgramEnvironment& env, ProControllerContext& context,
    Language language, int slot_1indexed,
    const PartySummaryReader& reader
){
    open_slot_submenu(env.console, context);
    enter_summary(env.console, context);

    PartyScanResult result;
    result.slot_1indexed = slot_1indexed;

    env.log("Slot " + std::to_string(slot_1indexed) + ": reading Summary page 1.");
    {
        VideoSnapshot screen1 = env.console.video().snapshot();
        reader.read_page1(env.logger(), language, screen1, result.read);
    }

    env.log("Slot " + std::to_string(slot_1indexed) + ": navigating to page 3.");
    navigate_to_page3(env.console, context);

    env.log("Slot " + std::to_string(slot_1indexed) + ": reading Summary page 3 moves.");
    {
        VideoSnapshot screen3 = env.console.video().snapshot();
        reader.read_page3_moves(env.logger(), language, screen3, result.read);
    }

    if (result.read.dex_no >= 0){
        const SpeciesData* sp = get_species_by_dex((uint16_t)result.read.dex_no);
        if (sp != nullptr){
            result.species_slug = sp->slug;
        }
    }

    env.log("Slot " + std::to_string(slot_1indexed) + ": exiting Summary.");
    exit_summary_to_party(env.console, context);

    return result;
}

//  Close the party menu and return to the overworld. Defensive: any of these
//  states may be active depending on where exit_summary_to_party left us
//  (party menu, slot submenu still open, summary still mid-fade). Mash B
//  long enough to collapse all of them, then settle.
void close_party_menu(ConsoleHandle& console, ProControllerContext& context){
    pbf_mash_button(context, BUTTON_B, 3500ms);
    context.wait_for_all_requests();
    pbf_wait(context, 800ms);
    context.wait_for_all_requests();
    console.log("Party menu closed (mashed B + settle).");
}

}  //  namespace


std::vector<PartyScanResult> scan_party(
    SingleSwitchProgramEnvironment& env, ProControllerContext& context,
    Language language, int party_size
){
    if (party_size < 1) party_size = 1;
    if (party_size > 6) party_size = 6;

    env.log("scan_party: opening party menu (target slots 1.." + std::to_string(party_size) + ").");
    open_party_menu_from_overworld(env.console, context);

    PartySummaryReader reader;
    VideoOverlaySet overlays(env.console.overlay());
    reader.make_overlays(overlays);

    std::vector<PartyScanResult> results;
    results.reserve((size_t)party_size);

    int current_slot = 1;
    for (int slot = 1; slot <= party_size; slot++){
        if (slot != current_slot){
            move_cursor_between_slots(context, current_slot, slot);
            context.wait_for_all_requests();
            current_slot = slot;
        }
        PartyScanResult r = read_current_slot(env, context, language, slot, reader);
        env.log(
            "Slot " + std::to_string(slot) + " result: lv=" +
            (r.read.level  >= 0 ? std::to_string(r.read.level)  : "?") +
            " dex=" +
            (r.read.dex_no >= 0 ? std::to_string(r.read.dex_no) : "?") +
            " species='" + r.species_slug + "'" +
            " nickname='" + r.read.nickname + "'" +
            " moves={'" + r.read.move_slugs[0] + "','" + r.read.move_slugs[1] +
            "','" + r.read.move_slugs[2] + "','" + r.read.move_slugs[3] + "'}"
        );
        results.push_back(std::move(r));
    }

    env.log("scan_party: closing party menu.");
    close_party_menu(env.console, context);
    return results;
}


PartyScanResult scan_party_slot(
    SingleSwitchProgramEnvironment& env, ProControllerContext& context,
    Language language, int slot_1indexed
){
    if (slot_1indexed < 1) slot_1indexed = 1;
    if (slot_1indexed > 6) slot_1indexed = 6;

    env.log("scan_party_slot: opening party menu for slot " + std::to_string(slot_1indexed) + ".");
    open_party_menu_from_overworld(env.console, context);

    PartySummaryReader reader;
    VideoOverlaySet overlays(env.console.overlay());
    reader.make_overlays(overlays);

    if (slot_1indexed != 1){
        move_cursor_between_slots(context, 1, slot_1indexed);
        context.wait_for_all_requests();
    }
    PartyScanResult r = read_current_slot(env, context, language, slot_1indexed, reader);

    env.log("scan_party_slot: closing party menu.");
    close_party_menu(env.console, context);
    return r;
}


}
}
}
