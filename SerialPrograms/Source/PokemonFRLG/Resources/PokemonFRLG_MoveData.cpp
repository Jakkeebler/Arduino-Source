/*  Pokemon FRLG Move Data
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include <map>
#include "Common/Cpp/Exceptions.h"
#include "Common/Cpp/Json/JsonValue.h"
#include "Common/Cpp/Json/JsonObject.h"
#include "CommonFramework/Globals.h"
#include "CommonTools/Options/StringSelectOption.h"
#include "PokemonFRLG_MoveData.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


namespace{

const std::string EMPTY_STRING;

struct FrlgMoveDatabase{
    FrlgMoveDatabase();
    static const FrlgMoveDatabase& instance(){
        static FrlgMoveDatabase database;
        return database;
    }

    std::vector<MoveData> ordered;
    std::map<std::string, size_t> slug_to_index;
    std::map<std::string, std::string> display_to_slug;
};

FrlgMoveDatabase::FrlgMoveDatabase(){
    std::string path = RESOURCE_PATH() + "PokemonFRLG/Data/Moves.json";
    JsonValue json = load_json_file(path);
    JsonObject& root = json.to_object_throw(path);

    ordered.reserve(root.size());
    for (auto& item : root){
        const std::string& slug = item.first;
        const JsonObject& fields = item.second.to_object_throw(path);

        MoveData entry;
        entry.slug = slug;
        entry.display_eng = fields.get_object_throw("displays", path).get_string_throw("eng", path);
        fields.read_string(entry.type, "type");
        fields.read_string(entry.category, "category");
        fields.read_integer(entry.max_pp, "pp", 0, 255);
        //  Older Moves.json files predate the "power" field; default to 0
        //  (unrankable) rather than failing to load the whole database.
        fields.read_integer(entry.power, "power", 0, 255);

        size_t index = ordered.size();
        display_to_slug.emplace(entry.display_eng, slug);
        ordered.push_back(std::move(entry));
        slug_to_index.emplace(slug, index);
    }
}

}  //  namespace


const MoveData& get_move(const std::string& slug){
    const FrlgMoveDatabase& db = FrlgMoveDatabase::instance();
    auto iter = db.slug_to_index.find(slug);
    if (iter == db.slug_to_index.end()){
        throw InternalProgramError(nullptr, PA_CURRENT_FUNCTION, "FRLG move slug not found: " + slug);
    }
    return db.ordered[iter->second];
}
const MoveData* get_move_nothrow(const std::string& slug){
    const FrlgMoveDatabase& db = FrlgMoveDatabase::instance();
    auto iter = db.slug_to_index.find(slug);
    if (iter == db.slug_to_index.end()){
        return nullptr;
    }
    return &db.ordered[iter->second];
}
const std::string& parse_move_display_name(const std::string& display_name){
    const FrlgMoveDatabase& db = FrlgMoveDatabase::instance();
    auto iter = db.display_to_slug.find(display_name);
    if (iter == db.display_to_slug.end()){
        return EMPTY_STRING;
    }
    return iter->second;
}
const std::vector<MoveData>& all_moves(){
    return FrlgMoveDatabase::instance().ordered;
}

bool is_damaging_move(const std::string& slug){
    if (slug.empty()){
        return false;
    }
    const MoveData* m = get_move_nothrow(slug);
    if (m == nullptr){
        return false;
    }
    //  PokeAPI's "damage_class" is one of "physical", "special", "status".
    //  Damaging moves are non-status.
    return m->category != "status";
}

const StringSelectDatabase& move_select_database(){
    static const StringSelectDatabase database = []{
        StringSelectDatabase db;
        for (const MoveData& m : all_moves()){
            db.add_entry(StringSelectEntry(m.slug, m.display_eng));
        }
        return db;
    }();
    return database;
}


}
}
}
