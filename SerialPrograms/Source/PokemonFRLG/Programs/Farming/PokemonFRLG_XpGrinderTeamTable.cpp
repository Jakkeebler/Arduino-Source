/*  Pokemon FRLG XP Grinder Team Table
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include <algorithm>
#include <set>
#include "PokemonFRLG/Resources/PokemonFRLG_MoveData.h"
#include "PokemonFRLG/Resources/PokemonFRLG_SpeciesData.h"
#include "PokemonFRLG/Resources/PokemonFRLG_Learnsets.h"
#include "PokemonFRLG_XpGrinderTeamTable.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


const EnumDropdownDatabase<OnUnknownOffered>& OnUnknownOffered_Database(){
    static const EnumDropdownDatabase<OnUnknownOffered> database({
        {OnUnknownOffered::Decline, "decline", "Decline (skip unrecognised moves)"},
        {OnUnknownOffered::Stop,    "stop",    "Stop the program"},
    });
    return database;
}


namespace{

//  Lazily-built dropdown database that prepends a "(none)" entry to the
//  full FRLG move list, sorted alphabetically by display name. Used for the
//  desired-move cells so users can leave a slot empty to mean "no preference".
//  Live per-species filtering would require framework changes to
//  StringSelectCell (the database reference is frozen at construction);
//  for now we ship a sorted full list and validate the user's picks against
//  the species' chain learnset at program start.
const StringSelectDatabase& move_select_database_with_none(){
    static const StringSelectDatabase database = []{
        std::vector<StringSelectEntry> sorted(
            move_select_database().case_list().begin(),
            move_select_database().case_list().end()
        );
        std::sort(sorted.begin(), sorted.end(),
            [](const StringSelectEntry& a, const StringSelectEntry& b){
                return a.display_name < b.display_name;
            }
        );
        StringSelectDatabase db;
        db.add_entry(StringSelectEntry("", "(none)"));
        for (const StringSelectEntry& e : sorted){
            db.add_entry(StringSelectEntry(e.slug, e.display_name, e.icon, e.text_color));
        }
        return db;
    }();
    return database;
}

//  Same idea for species: prepend "(unknown)" so freshly-added rows display
//  cleanly before the party scanner fills them in. Alphabetized by display name.
const StringSelectDatabase& species_select_database_with_unknown(){
    static const StringSelectDatabase database = []{
        std::vector<StringSelectEntry> sorted(
            species_select_database().case_list().begin(),
            species_select_database().case_list().end()
        );
        std::sort(sorted.begin(), sorted.end(),
            [](const StringSelectEntry& a, const StringSelectEntry& b){
                return a.display_name < b.display_name;
            }
        );
        StringSelectDatabase db;
        db.add_entry(StringSelectEntry("", "(unknown)"));
        for (const StringSelectEntry& e : sorted){
            db.add_entry(StringSelectEntry(e.slug, e.display_name, e.icon, e.text_color));
        }
        return db;
    }();
    return database;
}

}  //  namespace


XpGrinderTeamRow::XpGrinderTeamRow(EditableTableOption& parent_table)
    : EditableTableRow(parent_table)
    , species(species_select_database_with_unknown(), LockMode::LOCK_WHILE_RUNNING, std::string(""))
    , desired_move{
        StringSelectCell(move_select_database_with_none(), LockMode::LOCK_WHILE_RUNNING, std::string("")),
        StringSelectCell(move_select_database_with_none(), LockMode::LOCK_WHILE_RUNNING, std::string("")),
        StringSelectCell(move_select_database_with_none(), LockMode::LOCK_WHILE_RUNNING, std::string("")),
        StringSelectCell(move_select_database_with_none(), LockMode::LOCK_WHILE_RUNNING, std::string("")),
    }
    , on_unknown(OnUnknownOffered_Database(), LockMode::LOCK_WHILE_RUNNING, OnUnknownOffered::Decline)
{
    PA_ADD_OPTION(species);
    PA_ADD_OPTION(desired_move[0]);
    PA_ADD_OPTION(desired_move[1]);
    PA_ADD_OPTION(desired_move[2]);
    PA_ADD_OPTION(desired_move[3]);
    PA_ADD_OPTION(on_unknown);
}
std::unique_ptr<EditableTableRow> XpGrinderTeamRow::clone() const{
    std::unique_ptr<XpGrinderTeamRow> ret(new XpGrinderTeamRow(parent()));
    ret->species.set_by_slug(species.slug());
    for (int i = 0; i < 4; i++){
        ret->desired_move[i].set_by_slug(desired_move[i].slug());
    }
    ret->on_unknown.set(on_unknown);
    return ret;
}


std::vector<std::unique_ptr<EditableTableRow>> XpGrinderTeamTable::make_defaults(){
    std::vector<std::unique_ptr<EditableTableRow>> ret;
    ret.emplace_back(std::make_unique<XpGrinderTeamRow>(*this));
    return ret;
}
XpGrinderTeamTable::XpGrinderTeamTable()
    : EditableTableOption_t<XpGrinderTeamRow>(
        "<b>Team Table:</b><br>"
        "One row per party Pokémon, in the order they will lead. For each row "
        "set the species (auto-filled by the party scan; override here for "
        "nicknamed Pokémon whose dex# can't be read) and the four moves you "
        "want this Pokémon to end up with, ordered by priority (1st = most "
        "wanted). Leave a slot as <i>(none)</i> if you don't care.<br><br>"
        "<b>On Unknown:</b> what to do when a level-up offers a move whose "
        "name OCR fails. Default <i>Decline</i> skips the unknown move.",
        LockMode::LOCK_WHILE_RUNNING,
        make_defaults()
    )
{}
std::vector<std::string> XpGrinderTeamTable::make_header() const{
    return std::vector<std::string>{
        "Species",
        "Desired Move 1",
        "Desired Move 2",
        "Desired Move 3",
        "Desired Move 4",
        "On Unknown",
    };
}

std::string XpGrinderTeamTable::species_for(size_t pokemon) const{
    auto table = copy_snapshot();
    if (pokemon >= table.size()){
        return std::string();
    }
    return table[pokemon]->species.slug();
}
std::array<std::string, 4> XpGrinderTeamTable::desired_for(size_t pokemon) const{
    std::array<std::string, 4> result;
    auto table = copy_snapshot();
    if (pokemon >= table.size()){
        return result;
    }
    for (int i = 0; i < 4; i++){
        result[i] = table[pokemon]->desired_move[i].slug();
    }
    return result;
}
OnUnknownOffered XpGrinderTeamTable::on_unknown_for(size_t pokemon) const{
    auto table = copy_snapshot();
    if (pokemon >= table.size()){
        return OnUnknownOffered::Decline;
    }
    return table[pokemon]->on_unknown;
}
MoveLearnDecider XpGrinderTeamTable::make_decider(size_t pokemon) const{
    return MoveLearnDecider(desired_for(pokemon), on_unknown_for(pokemon));
}

bool XpGrinderTeamTable::set_species(size_t pokemon, const std::string& slug){
    bool applied = false;
    size_t i = 0;
    run_on_all_rows([&](XpGrinderTeamRow& row){
        if (i == pokemon){
            std::string err = row.species.set_by_slug(slug);
            applied = err.empty();
            return true;  //  found and applied; stop iterating
        }
        i++;
        return false;
    });
    return applied;
}

std::vector<std::string> XpGrinderTeamTable::validate_against_learnsets() const{
    std::vector<std::string> warnings;
    auto table = copy_snapshot();
    for (size_t i = 0; i < table.size(); i++){
        const XpGrinderTeamRow& row = *table[i];
        const std::string species = row.species.slug();
        if (species.empty()){
            continue;  //  no species set — can't validate
        }
        std::vector<std::string> learnable = learnable_moves_for_chain(species);
        std::set<std::string> learnable_set(learnable.begin(), learnable.end());
        for (int m = 0; m < 4; m++){
            const std::string& move = row.desired_move[m].slug();
            if (move.empty()){
                continue;  //  (none) is valid
            }
            if (learnable_set.find(move) == learnable_set.end()){
                const MoveData* md = get_move_nothrow(move);
                std::string move_display = md ? md->display_eng : move;
                warnings.push_back(
                    "Row " + std::to_string(i + 1) + " (" + species +
                    "): desired move slot " + std::to_string(m + 1) +
                    " '" + move_display + "' is not in the chain learnset."
                );
            }
        }
    }
    return warnings;
}


}
}
}
