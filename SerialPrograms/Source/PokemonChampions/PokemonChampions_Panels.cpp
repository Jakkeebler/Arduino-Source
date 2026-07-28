/*  Pokemon Champions Panels
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include "CommonFramework/Globals.h"
#include "CommonFramework/GlobalSettingsPanel.h"
#include "Pokemon/Pokemon_Strings.h"
#include "PokemonChampions_Panels.h"

#include "PokemonChampions_Settings.h"

//  Farming
#include "Programs/PokemonChampions_Autobattle.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonChampions{



PanelListFactory::PanelListFactory()
    : PanelListDescriptor(
        Pokemon::STRING_POKEMON + " Champions",
        RESOURCE_PATH() + "CategoryIcons/PokemonChampions.png"
    )
{}

std::vector<PanelEntry> PanelListFactory::make_panels() const{
    std::vector<PanelEntry> ret;

    ret.emplace_back("---- Settings ----");
    ret.emplace_back(make_settings<GameSettings_Descriptor, GameSettingsPanel>());

    ret.emplace_back("---- Untested/Beta/WIP ----");
    ret.emplace_back(make_single_switch_program<Autobattle_Descriptor, Autobattle>());

    return ret;
}



}
}
}
