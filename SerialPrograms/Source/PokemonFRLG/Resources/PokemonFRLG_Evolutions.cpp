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


}
}
}
