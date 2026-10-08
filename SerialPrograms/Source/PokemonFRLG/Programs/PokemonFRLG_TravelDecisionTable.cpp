/*  Pokemon FRLG Travel Decision Table
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include "PokemonFRLG_TravelDecisionTable.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


const char* travel_method_name(TravelMethod method){
    switch (method){
    case TravelMethod::Fly:      return "Fly";
    case TravelMethod::Teleport: return "Teleport";
    case TravelMethod::Walk:     return "Walk";
    case TravelMethod::Dig:      return "Dig";
    }
    return "?";
}

TravelMethod choose_travel_method(const TravelOptions& options){
    //  Explicit Dig escape pre-empts everything else. It is a deliberate,
    //  configured action, not a candidate weighed against Fly/Teleport.
    if (options.dig_explicit_escape_requested){
        return TravelMethod::Dig;
    }
    if (options.fly_spot_unlocked && options.fly_user_pp >= options.fly_pp_required){
        return TravelMethod::Fly;
    }
    if (options.teleport_available && options.destination_is_last_visited_pokecenter){
        return TravelMethod::Teleport;
    }
    return TravelMethod::Walk;
}


}
}
}
