/*  Pokemon FRLG AutoStory
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */

#include "Common/Cpp/Color.h"
#include "CommonFramework/Exceptions/OperationFailedException.h"
#include "Common/Cpp/Exceptions.h"
#include "Pokemon/Pokemon_Strings.h"
#include "Pokemon/Inference/Pokemon_NameReader.h"
#include "PokemonFRLG_AutoStory.h"
#include "PokemonFRLG_AutoStory_Segment_B0_00.h"
#include "PokemonFRLG_AutoStory_Segment_B0_01.h"
#include "PokemonFRLG_AutoStory_Segment_B0_02.h"
#include "PokemonFRLG_AutoStory_Segment_B0_03.h"
#include "PokemonFRLG_AutoStory_Segment_B0_04.h"
#include "PokemonFRLG_AutoStory_Segment_B0_05.h"
#include "PokemonFRLG_AutoStory_Segment_B3a.h"
#include "PokemonFRLG_AutoStory_Segment_B3b.h"
#include "PokemonFRLG_AutoStory_Segment_B3c.h"

namespace PokemonAutomation{
namespace NintendoSwitch{
namespace PokemonFRLG{


std::string phase_name(Phase phase){
    switch (phase){
    case Phase::B0:  return "B0: New Game \u2192 Boulder Badge prep (Viridian Forest exit)";
    case Phase::B1a: return "B1a: Route 2/Viridian Forest \u2192 Pewter Gym entrance";
    case Phase::B1b: return "B1b: Pewter Gym \u2192 Boulder Badge won";
    case Phase::B2a: return "B2a: Route 3 \u2192 Mt. Moon entrance";
    case Phase::B2b: return "B2b: Mt. Moon traversal";
    case Phase::B2c: return "B2c: Mt. Moon exit \u2192 Cerulean Gym \u2192 Cascade Badge won";
    case Phase::B3a: return "B3a: Cerulean \u2192 S.S. Anne boarding";
    case Phase::B3b: return "B3b: S.S. Anne traversal (Cut HM)";
    case Phase::B3c: return "B3c: Vermilion Gym \u2192 Thunder Badge won";
    case Phase::B4a: return "B4a: Route 6/Rock Tunnel \u2192 Lavender Town";
    case Phase::B4b: return "B4b: Celadon Gym \u2192 Rainbow Badge won";
    case Phase::B5a: return "B5a: Pokemon Tower entrance (Mr. Fuji rescue)";
    case Phase::B5b: return "B5b: Pokemon Tower traversal";
    case Phase::B5c: return "B5c: Fuchsia Gym \u2192 Soul Badge won";
    case Phase::B6a: return "B6a: Safari Zone \u2192 HM acquisition (Surf/Strength)";
    case Phase::B6b: return "B6b: Cinnabar Gym \u2192 Marsh Badge won";
    case Phase::B7a: return "B7a: Pokemon Mansion \u2192 Cinnabar Gym entrance";
    case Phase::B7b: return "B7b: Cinnabar Gym \u2192 Volcano Badge won";
    case Phase::B8a: return "B8a: Seafoam Islands traversal";
    case Phase::B8b: return "B8b: Viridian Gym \u2192 Earth Badge won";
    case Phase::B8c: return "B8c: Route 22/23 \u2192 Victory Road entrance";
    case Phase::B8d: return "B8d: Victory Road traversal \u2192 Pokemon League gate";
    }
    return "Unknown Phase";
}


namespace{

//  Framework-only stand-in for a real segment implementation. Documents the
//  phase's start/end state for the dropdowns and resume logic, but the
//  actual navigation/battle logic has not been written yet (later
//  milestones replace these one phase at a time). Throwing here instead of
//  silently no-opping keeps "not implemented" honest if someone runs it.
class AutoStory_Segment_Stub : public AutoStory_Segment{
public:
    AutoStory_Segment_Stub(Phase phase, std::string start_text, std::string end_text)
        : m_name(phase_name(phase))
        , m_start_text(std::move(start_text))
        , m_end_text(std::move(end_text))
    {}

    virtual std::string name() const override{
        return m_name;
    }
    virtual std::string start_text() const override{
        return m_start_text;
    }
    virtual std::string end_text() const override{
        return m_end_text;
    }
    virtual void run_segment(
        SingleSwitchProgramEnvironment& env,
        ProControllerContext& context,
        const AutoStoryOptions& options,
        AutoStoryStats& stats
    ) const override{
        throw OperationFailedException(
            ErrorReport::NO_ERROR_REPORT,
            "Phase \"" + m_name + "\" is not implemented yet. "
            "(M2 only builds the segment framework/orchestrator; the real "
            "navigation and battle logic for each phase ships in later milestones.)",
            env.console
        );
    }

private:
    std::string m_name;
    std::string m_start_text;
    std::string m_end_text;
};

//  Real implementation of Phase::B0, chaining the six B0_NN sub-segments
//  (new game -> Oak intro -> starter -> rival battle 1 -> Viridian PC heal
//  -> Route 1 -> Viridian Forest exit). Each sub-segment is implemented in
//  its own PokemonFRLG_AutoStory_Segment_B0_NN.h/.cpp file; see those files
//  for the per-step start/end state and known caveats (several steps here
//  are first-pass/not hardware-verified -- see each file's header comment).
class AutoStory_Segment_B0 : public AutoStory_Segment{
public:
    virtual std::string name() const override{
        return phase_name(Phase::B0);
    }
    virtual std::string start_text() const override{
        return "Standing in the player's bedroom on a brand new save.";
    }
    virtual std::string end_text() const override{
        return "Just exited Viridian Forest onto Route 2, heading for Pewter City.";
    }
    virtual void run_segment(
        SingleSwitchProgramEnvironment& env,
        ProControllerContext& context,
        const AutoStoryOptions& options,
        AutoStoryStats& stats
    ) const override{
        run_B0_00_new_game_oak_intro(env, context, options, stats);
        run_B0_01_starter_selection(env, context, options, stats);
        run_B0_02_rival_battle_1(env, context, options, stats);
        run_B0_03_viridian_pc_heal(env, context, options, stats);
        run_B0_04_route1_viridian_forest(env, context, options, stats);
        run_B0_05_viridian_forest_exit(env, context, options, stats);
    }
};

//  Real implementation of Phase::B3a: Cerulean Gym -> S.S. Anne gangplank.
//  Entirely map-driven navigate_to() calls (see B3a's .cpp for the leg-by-leg
//  breakdown).
class AutoStory_Segment_B3a : public AutoStory_Segment{
public:
    virtual std::string name() const override{
        return phase_name(Phase::B3a);
    }
    virtual std::string start_text() const override{
        return "Outside the Cerulean Gym, Cascade Badge in hand.";
    }
    virtual std::string end_text() const override{
        return "Standing at the S.S. Anne gangplank in Vermilion City.";
    }
    virtual void run_segment(
        SingleSwitchProgramEnvironment& env,
        ProControllerContext& context,
        const AutoStoryOptions& options,
        AutoStoryStats& stats
    ) const override{
        run_B3a_cerulean_to_ssanne(env, context, options, stats);
    }
};

//  Real implementation of Phase::B3b: board the S.S. Anne, fight the rival,
//  deliver the panacea, receive HM01 Cut, disembark. See B3b's header for
//  the known gaps in this first pass (interior/unmapped, not hardware-
//  verified).
class AutoStory_Segment_B3b : public AutoStory_Segment{
public:
    virtual std::string name() const override{
        return phase_name(Phase::B3b);
    }
    virtual std::string start_text() const override{
        return "At the S.S. Anne gangplank, about to board.";
    }
    virtual std::string end_text() const override{
        return "Off the S.S. Anne with Cut taught, back in Vermilion City.";
    }
    virtual void run_segment(
        SingleSwitchProgramEnvironment& env,
        ProControllerContext& context,
        const AutoStoryOptions& options,
        AutoStoryStats& stats
    ) const override{
        run_B3b_ssanne_traversal(env, context, options, stats);
    }
};

//  Real implementation of Phase::B3c: Vermilion Gym -> Lt. Surge -> Thunder
//  Badge + TM24, then a side trip to Bill's Sea Cottage for HM02 Fly before
//  returning outside the gym. See B3c's header for the known gaps (trash-
//  can puzzle order, Bill cutscene length -- not hardware-verified).
class AutoStory_Segment_B3c : public AutoStory_Segment{
public:
    virtual std::string name() const override{
        return phase_name(Phase::B3c);
    }
    virtual std::string start_text() const override{
        return "In Vermilion City with Cut, heading for the Vermilion Gym.";
    }
    virtual std::string end_text() const override{
        return "Thunder Badge won. Outside the Vermilion Gym.";
    }
    virtual void run_segment(
        SingleSwitchProgramEnvironment& env,
        ProControllerContext& context,
        const AutoStoryOptions& options,
        AutoStoryStats& stats
    ) const override{
        run_B3c_vermilion_gym_thunder_badge(env, context, options, stats);
    }
};

}  //  anonymous namespace


const std::vector<std::unique_ptr<AutoStory_Segment>>& ALL_AUTO_STORY_SEGMENT_LIST(){
    static const std::vector<std::unique_ptr<AutoStory_Segment>> list = [](){
        std::vector<std::unique_ptr<AutoStory_Segment>> ret;
        auto add = [&](Phase phase, const char* start_text, const char* end_text){
            ret.emplace_back(std::make_unique<AutoStory_Segment_Stub>(phase, start_text, end_text));
        };

        ret.emplace_back(std::make_unique<AutoStory_Segment_B0>());
        add(Phase::B1a, "On Route 2, just north of Viridian Forest's north exit.",                   "Standing in front of the Pewter Gym door.");
        add(Phase::B1b, "Standing in front of the Pewter Gym door, about to enter.",                 "Boulder Badge won. Outside the Pewter Gym.");
        add(Phase::B2a, "Outside the Pewter Gym, Boulder Badge in hand.",                            "Standing at the entrance to Mt. Moon.");
        add(Phase::B2b, "At the entrance to Mt. Moon.",                                              "At the far (Route 4) exit of Mt. Moon.");
        add(Phase::B2c, "At the Route 4 exit of Mt. Moon, heading for Cerulean City.",                "Cascade Badge won. Outside the Cerulean Gym.");
        ret.emplace_back(std::make_unique<AutoStory_Segment_B3a>());
        ret.emplace_back(std::make_unique<AutoStory_Segment_B3b>());
        ret.emplace_back(std::make_unique<AutoStory_Segment_B3c>());
        add(Phase::B4a, "Outside the Vermilion Gym, Thunder Badge in hand.",                           "In Lavender Town, having passed through Rock Tunnel.");
        add(Phase::B4b, "In Lavender Town, heading for Celadon City.",                                 "Rainbow Badge won. Outside the Celadon Gym.");
        add(Phase::B5a, "Outside the Celadon Gym, Rainbow Badge in hand.",                             "At the entrance to Pokemon Tower in Lavender Town.");
        add(Phase::B5b, "At the entrance to Pokemon Tower.",                                           "At the top of Pokemon Tower, Mr. Fuji rescued.");
        add(Phase::B5c, "Mr. Fuji rescued, heading for Fuchsia City.",                                 "Soul Badge won. Outside the Fuchsia Gym.");
        add(Phase::B6a, "Outside the Fuchsia Gym, Soul Badge in hand.",                                "Exited the Safari Zone with Surf and Strength obtained.");
        add(Phase::B6b, "Surf and Strength obtained, heading for Cinnabar Island.",                    "Marsh Badge won. Outside the Cinnabar Gym.");
        add(Phase::B7a, "Outside the Cinnabar Gym, Marsh Badge in hand.",                              "Exited Pokemon Mansion, standing outside the Cinnabar Gym.");
        add(Phase::B7b, "Standing outside the Cinnabar Gym, ready to challenge it.",                   "Volcano Badge won. Outside the Cinnabar Gym.");
        add(Phase::B8a, "Outside the Cinnabar Gym, Volcano Badge in hand.",                            "Exited Seafoam Islands onto Route 20, heading for Viridian City.");
        add(Phase::B8b, "On Route 20/Viridian City, heading for the Viridian Gym.",                    "Earth Badge won. Outside the Viridian Gym -- all 8 badges obtained.");
        add(Phase::B8c, "Outside the Viridian Gym, all 8 badges in hand.",                             "Standing at the entrance to Victory Road on Route 23.");
        add(Phase::B8d, "At the entrance to Victory Road.",                                            "At the Pokemon League gate, ready to enter the Elite Four.");

        return ret;
    }();
    return list;
}


const StringSelectDatabase& ALL_PHASES_SELECT_DATABASE(){
    static const StringSelectDatabase database = [](){
        StringSelectDatabase ret;
        const auto& segments = ALL_AUTO_STORY_SEGMENT_LIST();
        for (size_t c = 0; c < segments.size(); c++){
            ret.add_entry(StringSelectEntry(std::to_string(c), segments[c]->name()));
        }
        return ret;
    }();
    return database;
}



AutoStory_Descriptor::AutoStory_Descriptor()
    : SingleSwitchProgramDescriptor(
        "PokemonFRLG:AutoStory",
        Pokemon::STRING_POKEMON + " FRLG", "Auto Story",
        "Programs/PokemonFRLG/AutoStory.html",
        "Automatically play through the FRLG main story, badge by badge. "
        "Jump in or resume at any badge phase using the start/end phase dropdowns.",
        ProgramControllerClass::StandardController_NoRestrictions,
        FeedbackType::REQUIRED,
        AllowCommandsWhenRunning::DISABLE_COMMANDS
    )
{}
std::unique_ptr<StatsTracker> AutoStory_Descriptor::make_stats() const{
    return std::make_unique<AutoStoryStats>();
}


AutoStory::~AutoStory(){
    START_PHASE.remove_listener(*this);
    END_PHASE.remove_listener(*this);
}
AutoStory::AutoStory()
    : LANGUAGE(
        "<b>Game Language:</b><br>Required to read dialog boxes during the story.",
        Pokemon::PokemonNameReader::instance().languages(),
        LockMode::LOCK_WHILE_RUNNING, true
    )
    , STARTERCHOICE(
        "<b>Starter " + Pokemon::STRING_POKEMON + ":</b>",
        {
            {StarterChoice::Bulbasaur,  "bulbasaur",  "Bulbasaur (Grass/Poison)"},
            {StarterChoice::Charmander, "charmander", "Charmander (Fire)"},
            {StarterChoice::Squirtle,   "squirtle",   "Squirtle (Water)"},
        },
        LockMode::LOCK_WHILE_RUNNING,
        StarterChoice::Charmander
    )
    , SETUP_NOTE(
        "<b>Setup:</b> Start a new save (or resume mid-story using the dropdowns below). "
        "Text speed should be set to Fast. This program is in active development: "
        "only the segment framework exists so far, individual phases are not yet implemented."
    )
    , START_PHASE(
        "<b>Start Phase:</b><br>Jump in or resume starting from this phase.",
        ALL_PHASES_SELECT_DATABASE(),
        LockMode::LOCK_WHILE_RUNNING,
        "0"
    )
    , END_PHASE(
        "<b>End Phase:</b><br>Stop the run after completing this phase.",
        ALL_PHASES_SELECT_DATABASE(),
        LockMode::LOCK_WHILE_RUNNING,
        std::to_string(ALL_AUTO_STORY_SEGMENT_LIST().size() - 1)
    )
    , START_DESCRIPTION(
        ""
    )
    , END_DESCRIPTION(
        ""
    )
    , GO_HOME_WHEN_DONE(true)
    , NOTIFICATION_STATUS_UPDATE("Status Update", true, false, std::chrono::seconds(30))
    , NOTIFICATIONS({
        &NOTIFICATION_STATUS_UPDATE,
        &NOTIFICATION_PROGRAM_FINISH,
        &NOTIFICATION_ERROR_RECOVERABLE,
        &NOTIFICATION_ERROR_FATAL,
    })
{
    PA_ADD_OPTION(LANGUAGE);
    PA_ADD_OPTION(SETUP_NOTE);
    PA_ADD_OPTION(STARTERCHOICE);

    PA_ADD_OPTION(START_PHASE);
    PA_ADD_OPTION(START_DESCRIPTION);
    PA_ADD_OPTION(END_PHASE);
    PA_ADD_OPTION(END_DESCRIPTION);

    PA_ADD_OPTION(GO_HOME_WHEN_DONE);
    PA_ADD_OPTION(NOTIFICATIONS);

    AutoStory::on_config_value_changed(this);

    START_PHASE.add_listener(*this);
    END_PHASE.add_listener(*this);
}

void AutoStory::on_config_value_changed(void* object){
    START_DESCRIPTION.set_text(start_phase_description());
    END_DESCRIPTION.set_text(end_phase_description());
}

std::string AutoStory::start_phase_description() const{
    return "    Starts at: " + ALL_AUTO_STORY_SEGMENT_LIST()[get_start_phase_index()]->start_text();
}
std::string AutoStory::end_phase_description() const{
    return "    Ends at: " + ALL_AUTO_STORY_SEGMENT_LIST()[get_end_phase_index()]->end_text();
}

size_t AutoStory::get_start_phase_index() const{
    return START_PHASE.index();
}
size_t AutoStory::get_end_phase_index() const{
    return END_PHASE.index();
}

void AutoStory::run_autostory(SingleSwitchProgramEnvironment& env, ProControllerContext& context){
    AutoStoryOptions options{
        STARTERCHOICE,
        CheckpointMode::START_FROM_BEGINNING,
        10,
        NOTIFICATION_STATUS_UPDATE,
    };

    AutoStoryStats& stats = env.current_stats<AutoStoryStats>();
    context.wait_for_all_requests();

    const auto& segments = ALL_AUTO_STORY_SEGMENT_LIST();
    for (size_t phase_index = get_start_phase_index(); phase_index <= get_end_phase_index(); phase_index++){
        env.log("Starting phase: " + segments[phase_index]->name(), COLOR_BLUE);
        segments[phase_index]->run_segment(env, context, options, stats);
        track_phase_complete(env, options, stats, segments[phase_index]->name());
    }
}

void AutoStory::program(SingleSwitchProgramEnvironment& env, ProControllerContext& context){
    if (get_start_phase_index() > get_end_phase_index()){
        throw UserSetupError(env.logger(), "The start phase cannot be later than the end phase.");
    }

    run_autostory(env, context);
}


}
}
}
