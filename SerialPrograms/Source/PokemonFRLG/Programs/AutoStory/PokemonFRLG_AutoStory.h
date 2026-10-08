/*  Pokemon FRLG AutoStory
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *  Main program descriptor and orchestrator for the FRLG AutoStory project.
 *
 *  Phases are keyed to badges: B0 covers new-game through the Viridian
 *  Forest exit (Oak intro, starter pick, rival battle 1, first heal,
 *  Route 1). B1..B8 are Boulder, Cascade, Thunder, Rainbow, Soul, Marsh,
 *  Volcano, and Earth badge, each ending at the gym win + checkpoint save.
 *  Long dungeons get lettered sub-phases (e.g. B2a/b/c for Mt. Moon) so a
 *  stopped run can resume partway through instead of replaying the whole
 *  badge phase.
 *
 *  This file only lays out the framework: the Phase enum, the abstract
 *  Segment/Checkpoint classes segments will implement, the program
 *  descriptor/instance with START_PHASE/END_PHASE dropdowns, and the main
 *  loop that runs the selected phase range. The segments themselves (the
 *  actual navigation/battle logic per phase) are implemented in later
 *  milestones; for now each phase is backed by a stub segment that
 *  documents its start/end state and throws OperationFailedException when
 *  run, so the framework is honest about what is and is not implemented
 *  yet while still proving out the dropdown/orchestrator plumbing end to
 *  end.
 */

#ifndef PokemonAutomation_PokemonFRLG_AutoStory_H
#define PokemonAutomation_PokemonFRLG_AutoStory_H

#include <memory>
#include <string>
#include <vector>
#include "Common/Cpp/Options/StaticTextOption.h"
#include "Common/Cpp/Options/EnumDropdownOption.h"
#include "CommonFramework/Notifications/EventNotificationsTable.h"
#include "CommonTools/Options/StringSelectOption.h"
#include "CommonTools/Options/LanguageOCROption.h"
#include "NintendoSwitch/NintendoSwitch_SingleSwitchProgram.h"
#include "NintendoSwitch/Options/NintendoSwitch_GoHomeWhenDoneOption.h"
#include "PokemonFRLG/Programs/AutoStory/PokemonFRLG_AutoStoryTools.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


//  Badge-keyed story phase. Dungeon badge phases that span multiple
//  sittings (Mt. Moon, S.S. Anne, Pokemon Tower, Silph Co., Seafoam
//  Islands, Victory Road) get lettered sub-phases; short badge phases
//  (Thunder, Soul) get a single entry.
enum class Phase{
    B0,
    B1a, B1b,
    B2a, B2b, B2c,
    B3a, B3b, B3c,
    B4a, B4b,
    B5a, B5b, B5c,
    B6a, B6b,
    B7a, B7b,
    B8a, B8b, B8c, B8d,
};

//  Human-readable phase name, used for the dropdown labels and for
//  AutoStory_Segment::name() of the phase's stub/real segment.
std::string phase_name(Phase phase);


//  A Segment runs the story forward from one known game state to the
//  next. start_text()/end_text() document those states (what the player
//  should see on screen) so a user can jump into the middle of the story
//  and resume correctly via the START_PHASE dropdown.
class AutoStory_Segment{
public:
    virtual ~AutoStory_Segment() = default;
    virtual std::string name() const = 0;
    virtual std::string start_text() const = 0;
    virtual std::string end_text() const = 0;
    virtual void run_segment(
        SingleSwitchProgramEnvironment& env,
        ProControllerContext& context,
        const AutoStoryOptions& options,
        AutoStoryStats& stats
    ) const = 0;
};

//  A Checkpoint is a finer-grained re-entry point than a Segment (e.g. a
//  sub-step inside a long dungeon phase). Not every phase needs one;
//  checkpoints exist for phases where crash-resume needs tighter
//  granularity than "redo the whole phase".
class AutoStory_Checkpoint{
public:
    virtual ~AutoStory_Checkpoint() = default;
    virtual std::string name() const = 0;
    virtual std::string start_text() const = 0;
    virtual std::string end_text() const = 0;
    virtual void run_checkpoint(
        SingleSwitchProgramEnvironment& env,
        ProControllerContext& context,
        const AutoStoryOptions& options,
        AutoStoryStats& stats
    ) const = 0;
};


//  Registry of one segment per Phase enumerator, in phase order. Index i
//  corresponds to the i-th value of the Phase enum (B0 = 0, B1a = 1, ...).
const std::vector<std::unique_ptr<AutoStory_Segment>>& ALL_AUTO_STORY_SEGMENT_LIST();

//  StringSelectDatabase built from ALL_AUTO_STORY_SEGMENT_LIST(), used to
//  populate the START_PHASE / END_PHASE dropdowns.
const StringSelectDatabase& ALL_PHASES_SELECT_DATABASE();


class AutoStory_Descriptor : public SingleSwitchProgramDescriptor{
public:
    AutoStory_Descriptor();
    struct Stats;
    virtual std::unique_ptr<StatsTracker> make_stats() const override;
};


class AutoStory : public SingleSwitchProgramInstance, public ConfigOption::Listener{
public:
    ~AutoStory();
    AutoStory();

    virtual void program(SingleSwitchProgramEnvironment& env, ProControllerContext& context) override;

    //  Index into ALL_AUTO_STORY_SEGMENT_LIST() for the selected
    //  START_PHASE / END_PHASE dropdown entries.
    size_t get_start_phase_index() const;
    size_t get_end_phase_index() const;

    //  Instantiate and run every segment from get_start_phase_index() to
    //  get_end_phase_index(), inclusive.
    void run_autostory(SingleSwitchProgramEnvironment& env, ProControllerContext& context);

private:
    virtual void on_config_value_changed(void* object) override;

    std::string start_phase_description() const;
    std::string end_phase_description() const;

private:
    OCR::LanguageOCROption LANGUAGE;

    EnumDropdownOption<StarterChoice> STARTERCHOICE;

    StaticTextOption SETUP_NOTE;

    StringSelectOption START_PHASE;
    StringSelectOption END_PHASE;

    StaticTextOption START_DESCRIPTION;
    StaticTextOption END_DESCRIPTION;

    GoHomeWhenDoneOption GO_HOME_WHEN_DONE;

    EventNotificationOption NOTIFICATION_STATUS_UPDATE;
    EventNotificationsOption NOTIFICATIONS;
};


}
}
}
#endif
