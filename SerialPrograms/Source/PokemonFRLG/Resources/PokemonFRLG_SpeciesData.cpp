/*  Pokemon FRLG Species Data
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
#include "CommonTools/Options/StringSelectOption.h"
#include "Pokemon/Resources/Pokemon_PokemonNames.h"
#include "PokemonFRLG_SpeciesData.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


namespace{

struct FrlgSpeciesDatabase{
    FrlgSpeciesDatabase();
    static const FrlgSpeciesDatabase& instance(){
        static FrlgSpeciesDatabase database;
        return database;
    }

    std::vector<SpeciesData> ordered;
    std::map<std::string, size_t> slug_to_index;
    std::map<uint16_t, size_t> dex_to_index;
};

FrlgSpeciesDatabase::FrlgSpeciesDatabase(){
    std::string path = RESOURCE_PATH() + "PokemonFRLG/Data/Species.json";
    JsonValue json = load_json_file(path);
    JsonArray& root = json.to_array_throw(path);

    ordered.reserve(root.size());
    for (auto& item : root){
        const JsonObject& fields = item.to_object_throw(path);
        SpeciesData entry;
        entry.slug = fields.get_string_throw("slug", path);
        fields.read_integer(entry.dex_no, "dex", 0, 65535);
        //  Older Species.json files predate "types"; leave the list empty
        //  rather than failing to load. Callers must handle an empty typing
        //  (it just means "no STAB information available").
        const JsonArray* types = fields.get_array("types");
        if (types != nullptr){
            for (const JsonValue& t : *types){
                const std::string* s = t.to_string();
                if (s != nullptr && !s->empty()){
                    entry.types.push_back(*s);
                }
            }
        }

        size_t index = ordered.size();
        slug_to_index.emplace(entry.slug, index);
        if (entry.dex_no != 0){
            dex_to_index.emplace(entry.dex_no, index);
        }
        ordered.push_back(std::move(entry));
    }
}

}  //  namespace


const SpeciesData& get_species(const std::string& slug){
    const FrlgSpeciesDatabase& db = FrlgSpeciesDatabase::instance();
    auto iter = db.slug_to_index.find(slug);
    if (iter == db.slug_to_index.end()){
        throw InternalProgramError(nullptr, PA_CURRENT_FUNCTION, "FRLG species slug not found: " + slug);
    }
    return db.ordered[iter->second];
}
const SpeciesData* get_species_nothrow(const std::string& slug){
    const FrlgSpeciesDatabase& db = FrlgSpeciesDatabase::instance();
    auto iter = db.slug_to_index.find(slug);
    if (iter == db.slug_to_index.end()){
        return nullptr;
    }
    return &db.ordered[iter->second];
}
const SpeciesData* get_species_by_dex(uint16_t dex_no){
    const FrlgSpeciesDatabase& db = FrlgSpeciesDatabase::instance();
    auto iter = db.dex_to_index.find(dex_no);
    if (iter == db.dex_to_index.end()){
        return nullptr;
    }
    return &db.ordered[iter->second];
}
const std::vector<SpeciesData>& all_species(){
    return FrlgSpeciesDatabase::instance().ordered;
}

const StringSelectDatabase& species_select_database(){
    static const StringSelectDatabase database = []{
        StringSelectDatabase db;
        for (const SpeciesData& s : all_species()){
            const Pokemon::PokemonNames* names = Pokemon::get_pokemon_name_nothrow(s.slug);
            const std::string& display = names != nullptr ? names->display_name() : s.slug;
            db.add_entry(StringSelectEntry(s.slug, display));
        }
        return db;
    }();
    return database;
}


}
}
}
