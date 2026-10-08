/*  Pokemon FRLG Evolution Chains
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include <map>
#include "Common/Cpp/Exceptions.h"
#include "Common/Cpp/Json/JsonValue.h"
#include "Common/Cpp/Json/JsonArray.h"
#include "Common/Cpp/Json/JsonObject.h"
#include "CommonFramework/Globals.h"
#include "PokemonFRLG_Evolutions.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


namespace{

struct FrlgEvolutionDatabase{
    FrlgEvolutionDatabase();
    static const FrlgEvolutionDatabase& instance(){
        static FrlgEvolutionDatabase database;
        return database;
    }

    std::map<std::string, std::vector<std::string>> by_species;
    //  Cache for fallback "single-species chain" results so we can return
    //  stable references for unknown species.
    mutable std::map<std::string, std::vector<std::string>> singleton_cache;
};

FrlgEvolutionDatabase::FrlgEvolutionDatabase(){
    std::string path = RESOURCE_PATH() + "PokemonFRLG/Data/Evolutions.json";
    JsonValue json = load_json_file(path);
    JsonObject& root = json.to_object_throw(path);

    for (auto& item : root){
        const std::string& species_slug = item.first;
        const JsonArray& chain = item.second.to_array_throw(path);
        std::vector<std::string> list;
        list.reserve(chain.size());
        for (const JsonValue& v : chain){
            list.push_back(v.to_string_throw(path));
        }
        by_species.emplace(species_slug, std::move(list));
    }
}

const std::map<std::string, LevelEvolution>& level_evolution_table(){
    static const std::map<std::string, LevelEvolution> table{
        {"bulbasaur",  {"ivysaur",    16}}, {"ivysaur",    {"venusaur",   32}},
        {"charmander", {"charmeleon", 16}}, {"charmeleon", {"charizard",  36}},
        {"squirtle",   {"wartortle",  16}}, {"wartortle",  {"blastoise",  36}},
        {"caterpie",   {"metapod",     7}}, {"metapod",    {"butterfree", 10}},
        {"weedle",     {"kakuna",      7}}, {"kakuna",     {"beedrill",   10}},
        {"pidgey",     {"pidgeotto",  18}}, {"pidgeotto",  {"pidgeot",    36}},
        {"rattata",    {"raticate",   20}}, {"spearow",    {"fearow",     20}},
        {"ekans",      {"arbok",      22}}, {"sandshrew",  {"sandslash",  22}},
        {"nidoran-f",  {"nidorina",   16}}, {"nidoran-m",  {"nidorino",   16}},
        {"zubat",      {"golbat",     22}}, {"oddish",     {"gloom",      21}},
        {"paras",      {"parasect",   24}}, {"venonat",    {"venomoth",   31}},
        {"diglett",    {"dugtrio",    26}}, {"meowth",     {"persian",    28}},
        {"psyduck",    {"golduck",    33}}, {"mankey",     {"primeape",   28}},
        {"poliwag",    {"poliwhirl",  25}}, {"abra",       {"kadabra",    16}},
        {"machop",     {"machoke",    28}}, {"bellsprout", {"weepinbell", 21}},
        {"tentacool",  {"tentacruel", 30}}, {"geodude",    {"graveler",   25}},
        {"ponyta",     {"rapidash",   40}}, {"slowpoke",   {"slowbro",    37}},
        {"magnemite",  {"magneton",   30}}, {"doduo",      {"dodrio",     31}},
        {"seel",       {"dewgong",    34}}, {"grimer",     {"muk",        38}},
        {"gastly",     {"haunter",    25}}, {"drowzee",    {"hypno",      26}},
        {"krabby",     {"kingler",    28}}, {"voltorb",    {"electrode",  30}},
        {"koffing",    {"weezing",    35}}, {"rhyhorn",    {"rhydon",     42}},
        {"horsea",     {"seadra",     32}}, {"goldeen",    {"seaking",    33}},
        {"magikarp",   {"gyarados",   20}}, {"cubone",     {"marowak",    28}},
        {"omanyte",    {"omastar",    40}}, {"kabuto",     {"kabutops",   40}},
        {"dratini",    {"dragonair",  30}}, {"dragonair",  {"dragonite",  55}},
    };
    return table;
}

}  //  namespace


const std::vector<std::string>& evolution_chain_for(const std::string& species_slug){
    const FrlgEvolutionDatabase& db = FrlgEvolutionDatabase::instance();
    auto iter = db.by_species.find(species_slug);
    if (iter != db.by_species.end()){
        return iter->second;
    }
    //  Unknown species: cache and return a singleton chain {slug}. Empty
    //  slug returns a stable empty vector.
    static const std::vector<std::string> EMPTY;
    if (species_slug.empty()){
        return EMPTY;
    }
    auto cached = db.singleton_cache.find(species_slug);
    if (cached != db.singleton_cache.end()){
        return cached->second;
    }
    auto inserted = db.singleton_cache.emplace(species_slug, std::vector<std::string>{species_slug});
    return inserted.first->second;
}

const LevelEvolution* level_evolution_for(const std::string& species_slug){
    const auto& table = level_evolution_table();
    auto iter = table.find(species_slug);
    return iter == table.end() ? nullptr : &iter->second;
}


}
}
}
