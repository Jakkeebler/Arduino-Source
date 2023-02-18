/*  Battles
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#ifndef PokemonAutomation_PokemonSV_Battles_H
#define PokemonAutomation_PokemonSV_Battles_H


namespace PokemonAutomation{
    struct ProgramInfo;
    class ConsoleHandle;
    class BotBaseContext;
namespace NintendoSwitch{
namespace PokemonSV{



//  Returns the # of attempts it took to run.
int run_from_battle(const ProgramInfo& info, ConsoleHandle& console, BotBaseContext& context);



}
}
}
#endif
