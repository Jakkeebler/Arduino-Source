/*  Overworld Trigger
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#ifndef PokemonAutomation_PokemonBDSP_OverworldTrigger_H
#define PokemonAutomation_PokemonBDSP_OverworldTrigger_H

#include "Common/Cpp/Options/GroupOption.h"
#include "Common/Cpp/Options/EnumDropdownOption.h"
#include "Common/Cpp/Options/TimeExpressionOption.h"
#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "CommonFramework/Tools/ConsoleHandle.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonBDSP{

//  Program component to trigger an overworld encounter.
class OverworldTrigger : public GroupOption{
public:
    OverworldTrigger();

    //  Move character back and forth or use Sweet Scent to trigger an encounter.
    //  Return true if the start of a battle is detected.
    //  Return false if an unexpected battle happens where the battle menu is detected but
    //  not the starting animation.
    //  Throw exception if inference times out after Sweet Scent is used.
    bool find_encounter(ConsoleHandle& console, BotBaseContext& context) const;

private:
    //  Move character up and down or left and right once.
    void run_trigger(BotBaseContext& context) const;

public:
    enum class TriggerMethod{
        HORIZONTAL_NO_BIAS,
        HORIZONTAL_BIAS_LEFT,
        HORIZONTAL_BIAS_RIGHT,
        VERTICAL_NO_BIAS,
        VERTICAL_BIAS_UP,
        VERTICAL_BIAS_DOWN,
        SWEET_SCENT,
    };

    EnumDropdownOption<TriggerMethod> TRIGGER_METHOD;
    TimeExpressionOption<uint16_t> MOVE_DURATION;
    IntegerEnumDropdownOption SWEET_SCENT_POKEMON_LOCATION;
};



}
}
}
#endif
