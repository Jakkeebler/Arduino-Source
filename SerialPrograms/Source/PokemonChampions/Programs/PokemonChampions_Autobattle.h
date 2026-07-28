/*  Autobattle
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#ifndef PokemonAutomation_PokemonChampions_Autobattle_H
#define PokemonAutomation_PokemonChampions_Autobattle_H

#include "Common/Cpp/Options/ButtonOption.h"
#include "Common/Cpp/Options/SimpleIntegerOption.h"
#include "Common/Cpp/Options/EnumDropdownOption.h"
#include "CommonFramework/Notifications/EventNotificationsTable.h"
#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "NintendoSwitch/Options/NintendoSwitch_GoHomeWhenDoneOption.h"
#include "PokemonChampions/Inference/PokemonChampions_BattleDetectors.h"

namespace PokemonAutomation{

template <typename Type> class ControllerContext;

namespace NintendoSwitch{

class ProController;
using ProControllerContext = ControllerContext<ProController>;

namespace PokemonChampions{


//  How the program picks a move once it knows it is our turn.
enum class MoveSelectMode{
    MashA,
    FixedSlot,
    CycleSlots,
};
const EnumDropdownDatabase<MoveSelectMode>& MoveSelectMode_Database();


class Autobattle_Descriptor : public SingleSwitchProgramDescriptor{
public:
    Autobattle_Descriptor();

    class Stats;
    virtual std::unique_ptr<StatsTracker> make_stats() const override;
};


//  Repeatedly queue into online battles, play them out, and re-queue.
//
//  CALIBRATION STATUS: the Champions-specific detectors this program drives are
//  not yet calibrated (see the calibration blocks in
//  PokemonChampions/Inference/*.cpp). Every wait in here therefore has a timeout
//  and a fallback, and the fallback for an unrecognized screen is always to mash
//  A, which walks forward through confirm-style menus and plays the default move
//  each turn. That makes the program usable, but blind: win/loss stats stay
//  "Unknown" and menu navigation is not verified until frames are measured.
class Autobattle : public SingleSwitchProgramInstance{
public:
    Autobattle();

    virtual void program(SingleSwitchProgramEnvironment& env, ProControllerContext& context) override;

private:
    //  One full battle cycle: queue -> fight -> clear rewards -> back at a menu.
    void run_one_cycle(SingleSwitchProgramEnvironment& env, ProControllerContext& context);

    //  Navigate from the hub into the online queue and wait for an opponent.
    //  Returns true once the battle appears to have started.
    bool queue_for_battle(SingleSwitchProgramEnvironment& env, ProControllerContext& context);

    //  Play the battle out. Returns Unknown if the result banner could not be read.
    BattleResult run_battle(SingleSwitchProgramEnvironment& env, ProControllerContext& context);

    //  Advance the post-battle reward / rating screens back to a menu.
    void clear_post_battle(SingleSwitchProgramEnvironment& env, ProControllerContext& context);

    //  Commit to a move on the move select screen, per MOVE_MODE.
    void select_move(SingleSwitchProgramEnvironment& env, ProControllerContext& context, uint16_t turn);

    //  Best-effort return to a known state after a failed cycle.
    void recover_to_main_menu(SingleSwitchProgramEnvironment& env, ProControllerContext& context);

private:
    DeferredStopButtonOption STOP_AFTER_CURRENT;
    SimpleIntegerOption<uint64_t> BATTLE_LIMIT;
    EnumDropdownOption<MoveSelectMode> MOVE_MODE;
    SimpleIntegerOption<uint16_t> FIXED_MOVE_SLOT;
    SimpleIntegerOption<uint16_t> MATCHMAKING_TIMEOUT;
    SimpleIntegerOption<uint16_t> BATTLE_TIMEOUT;
    SimpleIntegerOption<uint16_t> MAX_CONSECUTIVE_FAILURES;
    GoHomeWhenDoneOption GO_HOME_WHEN_DONE;

    EventNotificationOption NOTIFICATION_STATUS_UPDATE;
    EventNotificationsOption NOTIFICATIONS;
};



}
}
}
#endif
