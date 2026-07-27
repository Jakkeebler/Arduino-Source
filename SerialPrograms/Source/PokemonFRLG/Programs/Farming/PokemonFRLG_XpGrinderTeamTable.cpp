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
#include "PokemonFRLG/Resources/PokemonFRLG_Evolutions.h"
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


namespace{

//  Build a fresh database with "(none)" + the union of moves the species'
//  evolution chain can learn (alphabetised by display name). Empty species
//  slug returns "(none)" + the full move list.
StringSelectDatabase build_chain_move_database(const std::string& species_slug){
    StringSelectDatabase db;
    db.add_entry(StringSelectEntry("", "(none)"));
    std::vector<std::string> learnable = learnable_moves_for_chain(species_slug);
    for (const std::string& slug : learnable){
        const MoveData* md = get_move_nothrow(slug);
        std::string display = md ? md->display_eng : slug;
        db.add_entry(StringSelectEntry(slug, display));
    }
    return db;
}

}  //  namespace


XpGrinderTeamRow::XpGrinderTeamRow(EditableTableOption& parent_table)
    : EditableTableRow(parent_table)
    , chain_move_db(build_chain_move_database(""))
    , species(species_select_database_with_unknown(), LockMode::LOCK_WHILE_RUNNING, std::string(""))
    , desired_move{
        StringSelectCell(chain_move_db, LockMode::LOCK_WHILE_RUNNING, std::string("")),
        StringSelectCell(chain_move_db, LockMode::LOCK_WHILE_RUNNING, std::string("")),
        StringSelectCell(chain_move_db, LockMode::LOCK_WHILE_RUNNING, std::string("")),
        StringSelectCell(chain_move_db, LockMode::LOCK_WHILE_RUNNING, std::string("")),
    }
    , on_unknown(OnUnknownOffered_Database(), LockMode::LOCK_WHILE_RUNNING, OnUnknownOffered::Decline)
{
    PA_ADD_OPTION(species);
    PA_ADD_OPTION(desired_move[0]);
    PA_ADD_OPTION(desired_move[1]);
    PA_ADD_OPTION(desired_move[2]);
    PA_ADD_OPTION(desired_move[3]);
    PA_ADD_OPTION(on_unknown);

    //  Cascading: when species changes, rebuild this row's move database so
    //  the four desired-move dropdowns only show moves the chain can learn.
    species.add_listener(*this);
}
XpGrinderTeamRow::~XpGrinderTeamRow(){
    species.remove_listener(*this);
}
void XpGrinderTeamRow::on_config_value_changed(void* object){
    //  Only react to species changes; move-cell value changes shouldn't
    //  re-trigger a database rebuild.
    if (object != &species){
        return;
    }
    rebuild_chain_db_for(species.slug());
}
void XpGrinderTeamRow::rebuild_chain_db_for(const std::string& species_slug){
    //  Capture current selections so we can restore those still valid in
    //  the new database; ones that aren't will collapse to "(none)".
    std::array<std::string, 4> saved;
    for (int i = 0; i < 4; i++){
        saved[i] = desired_move[i].slug();
    }

    //  Park each cell at index 0 ("(none)") so its cached index isn't
    //  out-of-bounds for the swapped-in database before we restore.
    for (int i = 0; i < 4; i++){
        desired_move[i].set_by_index(0);
    }

    //  Move-assign the new contents. The cells' const refs continue to
    //  point at this same object — the underlying Pimpl is what changes.
    chain_move_db = build_chain_move_database(species_slug);

    //  Restore each cell. set_by_slug returns an error string when the
    //  slug isn't found; the cell stays at index 0 in that case.
    for (int i = 0; i < 4; i++){
        if (!saved[i].empty()){
            desired_move[i].set_by_slug(saved[i]);
        }
    }

    //  Notify each cell's bound widget to rebuild its option list.
    for (int i = 0; i < 4; i++){
        desired_move[i].report_options_changed();
    }
}
std::unique_ptr<EditableTableRow> XpGrinderTeamRow::clone() const{
    std::unique_ptr<XpGrinderTeamRow> ret(new XpGrinderTeamRow(parent()));
    //  Set species first — its listener fires and rebuilds the move database
    //  for the new chain. Then setting move slugs has a populated database
    //  to find them in.
    ret->species.set_by_slug(species.slug());
    for (int i = 0; i < 4; i++){
        if (!desired_move[i].slug().empty()){
            ret->desired_move[i].set_by_slug(desired_move[i].slug());
        }
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
MoveLearnDecider XpGrinderTeamTable::make_decider(
    size_t pokemon,
    bool auto_rank,
    const std::array<std::string, 4>& known_current
) const{
    //  Species typing drives the STAB bonus. An unknown/unscanned species just
    //  means no STAB weighting; ranking by raw power still works.
    std::vector<std::string> types;
    const std::string slug = species_for(pokemon);
    if (!slug.empty()){
        const SpeciesData* sp = get_species_nothrow(slug);
        if (sp != nullptr){
            types = sp->types;
        }
    }
    return MoveLearnDecider(
        desired_for(pokemon), on_unknown_for(pokemon),
        auto_rank, std::move(types), known_current
    );
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

bool XpGrinderTeamTable::set_desired_moves(size_t pokemon, const std::array<std::string, 4>& move_slugs){
    bool applied = false;
    size_t i = 0;
    run_on_all_rows([&](XpGrinderTeamRow& row){
        if (i == pokemon){
            for (int m = 0; m < 4; m++){
                if (move_slugs[m].empty()){
                    row.desired_move[m].set_by_index(0);  //  "(none)"
                }else{
                    //  Falls back to "(none)" internally if the slug isn't in
                    //  this row's chain move database.
                    row.desired_move[m].set_by_slug(move_slugs[m]);
                }
            }
            applied = true;
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
