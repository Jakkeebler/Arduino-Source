/*  Pokemon FRLG Travel Decision Table
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Pure logic, no I/O. Picks which travel method to use for a given trip:
 *  Fly, Teleport, walking the pathfinder route, or Dig.
 *
 *  Priority (per FRO-161 plan / FRO-225 scope item 3):
 *    1. Fly, if an unlocked Fly spot exists for the destination AND the Fly
 *       user has enough PP.
 *    2. Else Teleport, if the move is usable AND this trip's destination is
 *       actually the last-visited Pokemon Center -- Teleport always returns
 *       there, so it is useless for any other destination, not merely
 *       non-preferred.
 *    3. Else walk the pathfinder route (kanto_navigate_to).
 *
 *  Dig is never selected by this priority chain, even when available: it is
 *  only ever returned when the caller explicitly asks for it as a configured
 *  escape action (dig_explicit_escape_requested). This is deliberate -- see
 *  FRO-225 scope item 3, "Dig only as an explicit configured escape (never
 *  implicit)" -- so an explicit Dig request pre-empts the whole Fly/Teleport/
 *  Walk chain rather than being weighed against it.
 *
 *  See FRO-225 (FRLG Auto-Train Phase 2).
 */

#ifndef PokemonAutomation_PokemonFRLG_TravelDecisionTable_H
#define PokemonAutomation_PokemonFRLG_TravelDecisionTable_H

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


enum class TravelMethod{
    Fly,
    Teleport,
    Walk,
    Dig,
};
const char* travel_method_name(TravelMethod method);


//  Inputs to the decision table. Holds no state and does no I/O -- the caller
//  is responsible for knowing/measuring every field before calling
//  choose_travel_method().
struct TravelOptions{
    //  Fly.
    bool fly_spot_unlocked = false;
    int fly_user_pp = 0;
    int fly_pp_required = 1;

    //  Teleport.
    bool teleport_available = false;
    //  Teleport always returns to the last-visited Pokemon Center. This must
    //  be true for Teleport to be useful for THIS trip's destination.
    bool destination_is_last_visited_pokecenter = false;

    //  Dig: the ONLY way choose_travel_method() ever returns
    //  TravelMethod::Dig. See the file comment above.
    bool dig_explicit_escape_requested = false;
};

TravelMethod choose_travel_method(const TravelOptions& options);


}
}
}
#endif
