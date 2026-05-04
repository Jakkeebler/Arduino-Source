/*  Pokemon FRLG Learnsets
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include <algorithm>
#include <map>
#include <set>
#include "Common/Cpp/Exceptions.h"
#include "Common/Cpp/Json/JsonValue.h"
#include "Common/Cpp/Json/JsonArray.h"
#include "Common/Cpp/Json/JsonObject.h"
#include "CommonFramework/Globals.h"
#include "PokemonFRLG_Evolutions.h"
#include "PokemonFRLG_MoveData.h"
#include "PokemonFRLG_Learnsets.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


namespace{

const std::vector<LearnsetEntry> EMPTY_LEARNSET;

struct FrlgLearnsetDatabase{
    FrlgLearnsetDatabase();
    static const FrlgLearnsetDatabase& instance(){
        static FrlgLearnsetDatabase database;
        return database;
    }

    std::map<std::string, std::vector<LearnsetEntry>> by_species;
};

FrlgLearnsetDatabase::FrlgLearnsetDatabase(){
    std::string path = RESOURCE_PATH() + "PokemonFRLG/Data/Learnsets.json";
    JsonValue json = load_json_file(path);
    JsonObject& root = json.to_object_throw(path);

    for (auto& item : root){
        const std::string& species_slug = item.first;
        const JsonArray& entries = item.second.to_array_throw(path);

        std::vector<LearnsetEntry> list;
        list.reserve(entries.size());
        for (const JsonValue& v : entries){
            const JsonArray& pair = v.to_array_throw(path);
            if (pair.size() != 2){
                throw FileException(
                    nullptr, PA_CURRENT_FUNCTION,
                    "Expected [level, move] pair for species: " + species_slug,
                    std::string(path)
                );
            }
            LearnsetEntry entry;
            int64_t level = pair[0].to_integer_throw(path);
            entry.level = (uint8_t)std::max<int64_t>(0, std::min<int64_t>(255, level));
            entry.move_slug = pair[1].to_string_throw(path);
            list.push_back(std::move(entry));
        }
        by_species.emplace(species_slug, std::move(list));
    }
}

}  //  namespace


const std::vector<LearnsetEntry>& learnset_for(const std::string& species_slug){
    const FrlgLearnsetDatabase& db = FrlgLearnsetDatabase::instance();
    auto iter = db.by_species.find(species_slug);
    if (iter == db.by_species.end()){
        return EMPTY_LEARNSET;
    }
    return iter->second;
}

std::vector<std::string> learnable_moves_for_chain(const std::string& species_slug){
    //  Empty slug = no species selected yet; return all moves so the dropdown
    //  isn't empty before the user has picked a species.
    if (species_slug.empty()){
        std::vector<std::string> all;
        all.reserve(all_moves().size());
        for (const MoveData& m : all_moves()){
            all.push_back(m.slug);
        }
        std::sort(all.begin(), all.end(),
            [](const std::string& a, const std::string& b){
                const MoveData* ma = get_move_nothrow(a);
                const MoveData* mb = get_move_nothrow(b);
                const std::string& da = ma ? ma->display_eng : a;
                const std::string& db = mb ? mb->display_eng : b;
                return da < db;
            }
        );
        return all;
    }

    //  Walk the evolution chain and union the level-up move slugs.
    std::set<std::string> set;
    for (const std::string& sp : evolution_chain_for(species_slug)){
        for (const LearnsetEntry& entry : learnset_for(sp)){
            set.insert(entry.move_slug);
        }
    }
    std::vector<std::string> out(set.begin(), set.end());
    //  Sort by display name for a friendlier dropdown.
    std::sort(out.begin(), out.end(),
        [](const std::string& a, const std::string& b){
            const MoveData* ma = get_move_nothrow(a);
            const MoveData* mb = get_move_nothrow(b);
            const std::string& da = ma ? ma->display_eng : a;
            const std::string& db = mb ? mb->display_eng : b;
            return da < db;
        }
    );
    return out;
}


}
}
}
