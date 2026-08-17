/*  Pokemon FRLG XP Grinder Team Table
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Per-Pokemon configuration table for the XP Grinder. One row per party
 *  slot, in lead order:
 *
 *      | Species | Desired Move 1..4 | On Unknown Offered |
 *
 *  Species is auto-populated from the party scanner but user-overridable
 *  (e.g. for nicknamed mons whose dex# couldn't be read). Desired moves are
 *  the user's intended final 4-move set, ordered by priority. On Unknown
 *  controls behaviour when a level-up offers a move whose name OCR fails.
 *
 */

#ifndef PokemonAutomation_PokemonFRLG_XpGrinderTeamTable_H
#define PokemonAutomation_PokemonFRLG_XpGrinderTeamTable_H

#include <array>
#include <string>
#include <vector>
#include "Common/Cpp/Options/EditableTableOption.h"
#include "Common/Cpp/Options/EnumDropdownOption.h"
#include "CommonTools/Options/StringSelectOption.h"
#include "PokemonFRLG/Programs/Farming/PokemonFRLG_MoveLearnDecider.h"
#include "PokemonFRLG/Programs/Farming/PokemonFRLG_MovePlan.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


const EnumDropdownDatabase<OnUnknownOffered>& OnUnknownOffered_Database();


class XpGrinderTeamRow : public EditableTableRow, public ConfigOption::Listener{
public:
    XpGrinderTeamRow(EditableTableOption& parent_table);
    virtual ~XpGrinderTeamRow();
    virtual std::unique_ptr<EditableTableRow> clone() const override;

    //  Listener hook: fires when the species cell changes. Rebuilds the
    //  per-row move database to only include moves the species' evolution
    //  chain can learn, then notifies each move cell's widget so the
    //  dropdown options are repopulated live.
    virtual void on_config_value_changed(void* object) override;

public:
    //  Per-row mutable move database. Holds "(none)" + the union of moves
    //  the row's currently-selected species' evolution chain can learn,
    //  alphabetized. Rebuilt on species change.
    StringSelectDatabase chain_move_db;

    StringSelectCell species;             //  slug; "" = unknown / not yet scanned
    StringSelectCell desired_move[4];     //  slug; "" = no preference for this slot
    EnumDropdownCell<OnUnknownOffered> on_unknown;

private:
    //  Rebuild chain_move_db's contents based on `species_slug`. Saves and
    //  restores each move cell's selected slug across the swap (drops to
    //  (none) for slugs no longer present).
    void rebuild_chain_db_for(const std::string& species_slug);
};


class XpGrinderTeamTable : public EditableTableOption_t<XpGrinderTeamRow>{
public:
    XpGrinderTeamTable();

    //  Number of rows currently configured.
    size_t row_count() const{ return current_rows(); }

    //  Read-only snapshots of one row's contents. Returns empty values
    //  (empty slugs / Decline) if `pokemon` is out of range.
    std::string                species_for(size_t pokemon) const;
    std::array<std::string, 4> desired_for(size_t pokemon) const;
    OnUnknownOffered           on_unknown_for(size_t pokemon) const;

    //  Auto-update a row's species cell. Used by the party scanner after
    //  detecting a new dex# (initial scan or post-evolution rescan). Returns
    //  true if the slug was applied, false if pokemon index is out of range
    //  or the slug isn't in the species dropdown database.
    bool set_species(size_t pokemon, const std::string& slug);

    //  Set a row's four desired-move cells. Used by the Team Scanner to seed
    //  the table from a party scan (the scanned current moves become the
    //  desired set). Call set_species first so the row's per-chain move
    //  database already contains these moves; any slug not in that database
    //  (or empty) collapses to "(none)". Returns false if pokemon is out of
    //  range.
    bool set_desired_moves(size_t pokemon, const std::array<std::string, 4>& move_slugs);

    //  Populate desired-move cells from MovePlan's suggestion: the strongest
    //  reachable level-up moves in the species' chain, coverage-biased, ordered
    //  strongest-first (slot 0 is the grinder's highest battle priority).
    //
    //  Rows the user has already filled are skipped unless `overwrite` is set,
    //  so this is safe to run right after a party scan without destroying manual
    //  picks. Rows with no species are skipped entirely.
    //
    //  `levels` and `current` are per-row party-scan data, used so the
    //  suggestion doesn't include moves whose level has already gone by. Pass
    //  empty vectors when unknown. Returns the number of rows changed.
    size_t autofill_desired_moves(
        bool overwrite,
        const std::vector<int>& levels = {},
        const std::vector<std::array<std::string, 4>>& current = {}
    );

    //  Plan a row against live party state: what is already known, what is
    //  coming and at what level, what has been missed for good, and whether
    //  evolving right now would forfeit something still wanted.
    MovePlan build_plan(
        size_t pokemon,
        int current_level,
        const std::array<std::string, 4>& current_moves
    ) const;

    //  Build a MoveLearnDecider from a row's snapshot. Caller owns the result.
    MoveLearnDecider make_decider(size_t pokemon) const;

    //  As above, but enables damage-based auto-ranking for moves the user did
    //  not pin in the table. `known_current` is the caller's cached view of the
    //  Pokemon's four current moves (needed to judge accept-vs-decline).
    //  Species typing for the STAB bonus is read from the row's species cell.
    MoveLearnDecider make_decider(
        size_t pokemon,
        bool auto_rank,
        const std::array<std::string, 4>& known_current
    ) const;

    //  Validate desired moves against the row's species evolution-chain
    //  learnset. Returns a list of human-readable warnings (one per invalid
    //  move). Empty list means everything is valid. Run at program start.
    std::vector<std::string> validate_against_learnsets() const;

private:
    virtual std::vector<std::string> make_header() const override;
    std::vector<std::unique_ptr<EditableTableRow>> make_defaults();
};


}
}
}
#endif
