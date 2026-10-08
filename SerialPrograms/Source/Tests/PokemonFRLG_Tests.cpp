/*  PokemonFRLG Tests
 *
 *  From: https://github.com/PokemonAutomation/
 *
 */


#include "CommonFramework/Logging/Logger.h"
#include "CommonFramework/Language.h"
#include "CommonFramework/ImageTools/ImageBoxes.h"
//#include "CommonFramework/Recording/StreamHistorySession.h"
//#include "NintendoSwitch/Controllers/SerialPABotBase/NintendoSwitch_SerialPABotBase_WiredController.h"
//#include "NintendoSwitch/NintendoSwitch_ConsoleHandle.h"
#include "PokemonFRLG/Inference/Dialogs/PokemonFRLG_DialogDetector.h"
#include "PokemonFRLG/Inference/Dialogs/PokemonFRLG_BattleDialogs.h"
#include "PokemonFRLG/Inference/Dialogs/PokemonFRLG_PrizeSelectDetector.h"
#include "PokemonFRLG/Inference/Dialogs/PokemonFRLG_LearnMoveDialogReader.h"
#include "PokemonFRLG/Inference/Dialogs/PokemonFRLG_ForgetMoveScreen.h"
#include "PokemonFRLG/Inference/PokemonFRLG_ShinySymbolDetector.h"
#include "PokemonFRLG/Inference/PokemonFRLG_PartySummaryReader.h"
#include "PokemonFRLG/Programs/Farming/PokemonFRLG_EvolutionPolicy.h"
#include "PokemonFRLG/Programs/Farming/PokemonFRLG_MoveLearnDecider.h"
#include "PokemonFRLG/Programs/Farming/PokemonFRLG_MoveLearnStateMachine.h"
#include "PokemonFRLG/Programs/Farming/PokemonFRLG_MovePlan.h"
#include "PokemonFRLG/Resources/PokemonFRLG_Evolutions.h"
#include "PokemonFRLG/Resources/PokemonFRLG_Learnsets.h"
#include "PokemonFRLG/Resources/PokemonFRLG_MoveData.h"
#include "PokemonFRLG_Tests.h"
#include "TestUtils.h"

#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

namespace PokemonAutomation{

using namespace NintendoSwitch;
using namespace NintendoSwitch::PokemonFRLG;

int test_pokemonFRLG_AdvanceWhiteDialogDetector(const ImageViewRGB32& image, bool target){
    auto overlay = DummyVideoOverlay();
    AdvanceWhiteDialogDetector detector(COLOR_RED);
    bool result = detector.detect(image);
    TEST_RESULT_EQUAL(result, target);
    return 0;
}

int test_pokemonFRLG_ShinySymbolDetector(const ImageViewRGB32& image, bool target){
    auto& logger = global_logger_command_line();
    auto overlay = DummyVideoOverlay();
    ShinySymbolDetector detector(COLOR_RED);
    bool result = detector.read(logger, image);
    TEST_RESULT_EQUAL(result, target);
    return 0;
}

int test_pokemonFRLG_SelectionDialogDetector(const ImageViewRGB32& image, bool target){
    auto overlay = DummyVideoOverlay();
    SelectionDialogDetector detector(COLOR_RED);
    bool result = detector.detect(image);
    TEST_RESULT_EQUAL(result, target);
    return 0;
}

int test_pokemonFRLG_AdvanceBattleDialogDetector(const ImageViewRGB32& image, bool target){
    auto overlay = DummyVideoOverlay();
    AdvanceBattleDialogDetector detector(COLOR_RED);
    bool result = detector.detect(image);
    TEST_RESULT_EQUAL(result, target);
    return 0;
}

int test_pokemonFRLG_BattleMenuDetector(const ImageViewRGB32& image, bool target){
    auto overlay = DummyVideoOverlay();
    BattleMenuDetector detector(COLOR_RED);
    bool result = detector.detect(image);
    TEST_RESULT_EQUAL(result, target);
    return 0;
}

int test_pokemonFRLG_PrizeSelectDetector(const ImageViewRGB32& image, bool target){
    auto overlay = DummyVideoOverlay();
    PrizeSelectDetector detector(COLOR_RED);
    bool result = detector.detect(image);
    TEST_RESULT_EQUAL(result, target);
    return 0;
}

int test_pokemonFRLG_PartySummaryReader_dex(const ImageViewRGB32& image, int target){
    auto& logger = global_logger_command_line();
    PartySummaryReader reader(COLOR_RED);
    PartySummaryRead out;
    reader.read_page1(logger, Language::English, image, out);
    TEST_RESULT_EQUAL(out.dex_no, target);
    return 0;
}

int test_pokemonFRLG_PartySummaryReader_moves(const ImageViewRGB32& image, const std::vector<std::string>& expected){
    auto& logger = global_logger_command_line();
    if (expected.size() < 4){
        cerr << "PartySummaryReader_moves test needs 4 expected slugs in filename." << endl;
        return 1;
    }
    PartySummaryReader reader(COLOR_RED);
    PartySummaryRead out;
    reader.read_page3_moves(logger, Language::English, image, out);
    //  Last 4 words of the filename are the expected slugs in order.
    const size_t base = expected.size() - 4;
    for (int i = 0; i < 4; i++){
        TEST_RESULT_EQUAL(out.move_slugs[i], expected[base + i]);
    }
    return 0;
}

int test_pokemonFRLG_LearnMoveDialogReader(const ImageViewRGB32& image, const std::vector<std::string>& expected){
    auto& logger = global_logger_command_line();
    if (expected.empty()){
        cerr << "LearnMoveDialogReader test needs an expected move slug in filename." << endl;
        return 1;
    }
    LearnMoveDialogReader reader(COLOR_RED);
    std::string got = reader.read_new_move(logger, Language::English, image);
    TEST_RESULT_EQUAL(got, expected[expected.size() - 1]);
    return 0;
}

int test_pokemonFRLG_ForgetMoveScreenDetector(const ImageViewRGB32& image, bool target){
    auto overlay = DummyVideoOverlay();
    ForgetMoveScreenDetector detector(COLOR_RED);
    bool result = detector.detect(image);
    TEST_RESULT_EQUAL(result, target);
    return 0;
}

int test_pokemonFRLG_ForgetMoveScreenReader(const ImageViewRGB32& image, const std::vector<std::string>& expected){
    auto& logger = global_logger_command_line();
    if (expected.size() < 4){
        cerr << "ForgetMoveScreenReader test needs 4 expected slugs in filename." << endl;
        return 1;
    }
    ForgetMoveScreenReader reader(COLOR_RED);
    std::array<std::string, 4> got = reader.read_moves(logger, Language::English, image);
    const size_t base = expected.size() - 4;
    for (int i = 0; i < 4; i++){
        TEST_RESULT_EQUAL(got[i], expected[base + i]);
    }
    return 0;
}

//  ---- Move learn decider ----
//
//  Registered as "PokemonFRLG_MoveLearnDecider". It reads no image, but the test
//  runner calls a test once per file in its folder, so drop any placeholder file
//  in Tests/PokemonFRLG/MoveLearnDecider/.
//
//  The pure-logic checks (voting, state machine, evolution gate, coverage) need
//  no data and always run. The five decider scenarios need the FRLG move /
//  learnset data from the Packages repo; without it they are reported as skipped
//  on stderr rather than failed.

namespace{

using FirstAction = MoveLearnDecider::FirstAction;
using Moves = std::array<std::string, 4>;

int as_int(FirstAction a){ return (int)a; }

//  std::array has no operator<<, which TEST_RESULT_EQUAL needs to report a mismatch.
std::string join(const Moves& moves){
    return moves[0] + "|" + moves[1] + "|" + moves[2] + "|" + moves[3];
}

bool have_move_data(){
    const MoveData* m = get_move_nothrow("slash");
    return m != nullptr && m->power != 0 && get_move_nothrow("ice-beam") != nullptr;
}
bool have_learnset_data(){
    return !learnset_for("charmander").empty()
        && !learnset_for("magikarp").empty()
        && !learnset_for("gyarados").empty();
}

int test_pure_logic(){
    //  Voting: strict majority, blanks never count.
    TEST_RESULT_EQUAL(vote_string({"ember", "ember"}), std::string("ember"));
    TEST_RESULT_EQUAL(vote_string({"ember", "tackle"}), std::string());
    TEST_RESULT_EQUAL(vote_string({"ember", "tackle", "ember"}), std::string("ember"));
    TEST_RESULT_EQUAL(vote_string({"", ""}), std::string());
    TEST_RESULT_EQUAL(vote_string({"", "ember"}), std::string());
    TEST_RESULT_EQUAL(vote_string({"ember"}), std::string("ember"));
    {
        Moves out{"keep", "keep", "keep", "keep"};
        std::vector<Moves> agree{{"a", "b", "c", "d"}, {"a", "b", "c", "d"}};
        TEST_RESULT_EQUAL(vote_move_list(agree, out), true);
        TEST_RESULT_EQUAL(out[2], std::string("c"));
        std::vector<Moves> split{{"a", "b", "c", "d"}, {"a", "b", "x", "d"}};
        TEST_RESULT_EQUAL(vote_move_list(split, out), false);
        TEST_RESULT_EQUAL(out[2], std::string("c"));    //  untouched on failure
    }

    //  State machine: success walks the chain, 3 straight failures recover.
    {
        using SM = MoveLearnStateMachine;
        SM sm(3);
        TEST_RESULT_EQUAL((int)sm.state(), (int)MoveLearnState::AwaitingPrompt);
        TEST_RESULT_EQUAL((int)sm.report(SM::Outcome::Success), (int)SM::Step::Advance);
        TEST_RESULT_EQUAL((int)sm.state(), (int)MoveLearnState::ReadingNewMove);
        TEST_RESULT_EQUAL((int)sm.report(SM::Outcome::Disagreement), (int)SM::Step::Retry);
        TEST_RESULT_EQUAL((int)sm.report(SM::Outcome::Timeout), (int)SM::Step::Retry);
        TEST_RESULT_EQUAL((int)sm.report(SM::Outcome::Disagreement), (int)SM::Step::Recover);
        TEST_RESULT_EQUAL((int)sm.state(), (int)MoveLearnState::ReadingNewMove);    //  stays put
        TEST_RESULT_EQUAL((int)sm.consecutive_failures(), 0);                       //  fresh budget
        //  A success in between resets the streak.
        TEST_RESULT_EQUAL((int)sm.report(SM::Outcome::Disagreement), (int)SM::Step::Retry);
        TEST_RESULT_EQUAL((int)sm.report(SM::Outcome::Success), (int)SM::Step::Advance);
        TEST_RESULT_EQUAL((int)sm.state(), (int)MoveLearnState::ReadingMoveList);
        TEST_RESULT_EQUAL((int)sm.report(SM::Outcome::Disagreement), (int)SM::Step::Retry);
        TEST_RESULT_EQUAL((int)sm.report(SM::Outcome::Disagreement), (int)SM::Step::Retry);
        sm.report(SM::Outcome::Success);
        sm.report(SM::Outcome::Success);
        TEST_RESULT_EQUAL((int)sm.state(), (int)MoveLearnState::Confirmed);
    }

    //  Evolution gate: exactly one level short, and only if the pool changes.
    TEST_RESULT_EQUAL(should_defer_for_evolution(15, 16, true), true);
    TEST_RESULT_EQUAL(should_defer_for_evolution(14, 16, true), false);
    TEST_RESULT_EQUAL(should_defer_for_evolution(16, 16, true), false);
    TEST_RESULT_EQUAL(should_defer_for_evolution(15, 16, false), false);
    TEST_RESULT_EQUAL(should_defer_for_evolution(15, 0, true), false);

    //  Coverage: fire hits bug / steel / grass / ice; ice already covers grass.
    TEST_RESULT_EQUAL(coverage_gain("fire", {}), 4);
    TEST_RESULT_EQUAL(coverage_gain("fire", {"ice"}), 3);
    TEST_RESULT_EQUAL(coverage_gain("fire", {"fire"}), 0);
    TEST_RESULT_EQUAL(coverage_gain("fire", {}, {"grass", "water"}), 1);
    TEST_RESULT_EQUAL(coverage_gain("normal", {}), 0);
    TEST_RESULT_EQUAL(coverage_gain("ghost", {}, {"steel"}), 0);    //  Gen 3: steel resists ghost
    return 0;
}

//  1. STAB: an own-type move outranks a stronger off-type one.
int test_scenario_stab(){
    const Moves current{"slash", "strength", "body-slam", "hyper-fang"};   //  all Normal
    MoveLearnDecider decider(
        {}, OnUnknownOffered::Decline, /*auto_rank=*/true, {"fire"}, current
    );
    //  Ember (40, Fire, STAB) displaces the weakest Normal move even though
    //  every one of them has more power.
    TEST_RESULT_EQUAL(as_int(decider.decide_accept_or_decline("ember")), as_int(FirstAction::Replace));
    TEST_RESULT_EQUAL(decider.pick_forget_slot("ember", current), 0);   //  Slash, the weakest
    //  Off-type Tackle (35) has no STAB, no coverage and less power: declined.
    TEST_RESULT_EQUAL(as_int(decider.decide_accept_or_decline("tackle")), as_int(FirstAction::Decline));
    return 0;
}

//  2. Weakness coverage: forget the move whose coverage the rest already has.
int test_scenario_coverage(){
    //  No Normal moves, so a Normal-type user gets no STAB from any of them.
    const Moves current{"water-gun", "thunder-shock", "ember", "flamethrower"};
    MoveLearnDecider decider(
        {}, OnUnknownOffered::Decline, true, {"normal"}, current
    );
    //  Ember and Flamethrower overlap completely; Ember is the weaker. Raw
    //  power alone would have forgotten Water Gun (slot 0, lowest index among
    //  the 40-power moves).
    TEST_RESULT_EQUAL(decider.pick_forget_slot("ice-beam", current), 2);
    //  Ice Beam adds Dragon on top of what is left, so it earns the slot.
    TEST_RESULT_EQUAL(as_int(decider.decide_accept_or_decline("ice-beam")), as_int(FirstAction::Replace));
    return 0;
}

//  3. Pinning: a pinned move is never forgotten, however weak it scores.
int test_scenario_pinning(){
    const Moves current{"tackle", "ember", "flamethrower", "water-gun"};
    {
        MoveLearnDecider decider(
            {"tackle", "", "", ""}, OnUnknownOffered::Decline, true, {"fire"}, current
        );
        const int slot = decider.pick_forget_slot("slash", current);
        TEST_RESULT_EQUAL(slot >= 0 && slot != 0, true);    //  Tackle is the weakest, but pinned
    }
    {
        //  All four pinned: nothing may be forgotten, and a worse unpinned
        //  offer is declined.
        MoveLearnDecider decider(
            current, OnUnknownOffered::Decline, true, {"fire"}, current
        );
        TEST_RESULT_EQUAL(decider.pick_forget_slot("slash", current), -1);
        TEST_RESULT_EQUAL(as_int(decider.decide_accept_or_decline("slash")), as_int(FirstAction::Decline));
        //  Same with auto-rank off.
        MoveLearnDecider plain(current, OnUnknownOffered::Decline);
        TEST_RESULT_EQUAL(plain.pick_forget_slot("slash", current), -1);
    }
    {
        //  A pinned move that is offered is always accepted, even if worse than
        //  everything known. Forgetting the duplicate of itself is the one
        //  exception to "never forget a pinned move".
        MoveLearnDecider decider(
            {"growl", "", "", ""}, OnUnknownOffered::Decline, true, {"fire"}, current
        );
        TEST_RESULT_EQUAL(as_int(decider.decide_accept_or_decline("growl")), as_int(FirstAction::Replace));
        const Moves with_dup{"growl", "ember", "flamethrower", "water-gun"};
        TEST_RESULT_EQUAL(decider.pick_forget_slot("growl", with_dup), 0);
    }
    return 0;
}

//  4. Recommended moveset: used only when the user configured nothing.
int test_scenario_recommended(){
    const Moves blank{};
    const Moves recommended = recommended_desired_moves("charmander", blank);
    TEST_RESULT_EQUAL(join(recommended), join(suggest_desired_moves("charmander")));
    TEST_RESULT_EQUAL(recommended[0].empty(), false);

    const Moves user{"", "scratch", "", ""};
    TEST_RESULT_EQUAL(join(recommended_desired_moves("charmander", user)), join(user));

    //  And it feeds the decider as the pinned set: its top pick is accepted.
    MoveLearnDecider decider(recommended, OnUnknownOffered::Decline);
    TEST_RESULT_EQUAL(as_int(decider.decide_accept_or_decline(recommended[0])), as_int(FirstAction::Replace));
    return 0;
}

//  5. Evolution protection: hold off one level before a pool-changing evolution.
//  Magikarp -> Gyarados at 20 changes the remaining pool completely (Flail vs
//  Thrash / Bite / ...). The level is overridden so the check does not depend on
//  where in Magikarp's learnset the offered move happens to sit.
int test_scenario_evolution(){
    const Moves current{"splash", "growl", "leer", "tackle"};
    auto make = [&](int level, bool protect, const Moves& desired){
        MoveLearnContext context;
        context.species_slug = "magikarp";
        context.level = level;
        context.evolution_protection = protect;
        return MoveLearnDecider(
            desired, OnUnknownOffered::Decline, true, {"water"}, current, context
        );
    };
    const Moves none{};

    //  Two levels short: business as usual (Slash beats the status moves).
    MoveLearnDecider early = make(18, true, none);
    TEST_RESULT_EQUAL(as_int(early.decide_accept_or_decline("slash")), as_int(FirstAction::Replace));
    TEST_RESULT_EQUAL(early.evolution_deferral_reason("slash").empty(), true);

    //  One level short: declined, with a reason to log.
    MoveLearnDecider close = make(19, true, none);
    TEST_RESULT_EQUAL(as_int(close.decide_accept_or_decline("slash")), as_int(FirstAction::Decline));
    TEST_RESULT_EQUAL(close.evolution_deferral_reason("slash").empty(), false);

    //  Opt-out restores the normal decision.
    MoveLearnDecider off = make(19, false, none);
    TEST_RESULT_EQUAL(as_int(off.decide_accept_or_decline("slash")), as_int(FirstAction::Replace));

    //  A pinned move is the user's call and is still taken.
    MoveLearnDecider pinned = make(19, true, {"slash", "", "", ""});
    TEST_RESULT_EQUAL(as_int(pinned.decide_accept_or_decline("slash")), as_int(FirstAction::Replace));
    return 0;
}

}  //  namespace

int test_pokemonFRLG_MoveLearnDecider(const std::string& test_file_path){
    int result = test_pure_logic();
    if (result != 0){
        return result;
    }
    if (!have_move_data() || !have_learnset_data()){
        cerr << "PokemonFRLG_MoveLearnDecider: FRLG move/learnset data not found under the resource path; "
                "skipped the 5 decider scenarios (only the pure-logic checks ran)." << endl;
        return 0;
    }
    for (int (*scenario)() : {
        test_scenario_stab,
        test_scenario_coverage,
        test_scenario_pinning,
        test_scenario_recommended,
        test_scenario_evolution,
    }){
        result = scenario();
        if (result != 0){
            return result;
        }
    }
    return 0;
}

//  ---- Evolution policy (FRO-226) ----
//
//  Registered as "PokemonFRLG_EvolutionPolicy". Pure logic, no image/file
//  data needed -- same "ignores test_file_path" pattern as the move-learn
//  decider test above.

int test_pokemonFRLG_EvolutionPolicy(const std::string& test_file_path){
    //  All 4 policy types: whether an automatic (level-up dialog) evolution
    //  must be blocked.
    TEST_RESULT_EQUAL(policy_blocks_automatic_evolution(EvolutionPolicyType::LevelUp), false);
    TEST_RESULT_EQUAL(policy_blocks_automatic_evolution(EvolutionPolicyType::Stone), true);
    TEST_RESULT_EQUAL(policy_blocks_automatic_evolution(EvolutionPolicyType::Trade), true);
    TEST_RESULT_EQUAL(policy_blocks_automatic_evolution(EvolutionPolicyType::Block), true);

    //  Only Stone is the explicit-item-step policy.
    TEST_RESULT_EQUAL(policy_uses_explicit_stone_step(EvolutionPolicyType::LevelUp), false);
    TEST_RESULT_EQUAL(policy_uses_explicit_stone_step(EvolutionPolicyType::Stone), true);
    TEST_RESULT_EQUAL(policy_uses_explicit_stone_step(EvolutionPolicyType::Trade), false);
    TEST_RESULT_EQUAL(policy_uses_explicit_stone_step(EvolutionPolicyType::Block), false);

    //  Pre-level-up guard: Block / Trade stop one level short (current_level+1
    //  >= evolution_level); LevelUp and Stone never do; no level-up evolution
    //  (<=0) never triggers it.
    TEST_RESULT_EQUAL(should_stop_grind_before_blocked_evolution(EvolutionPolicyType::Block, 15, 16), true);   //  one level short
    TEST_RESULT_EQUAL(should_stop_grind_before_blocked_evolution(EvolutionPolicyType::Block, 14, 16), false);  //  two levels short: not yet
    TEST_RESULT_EQUAL(should_stop_grind_before_blocked_evolution(EvolutionPolicyType::Block, 16, 16), true);   //  already at it: still must not let it through
    TEST_RESULT_EQUAL(should_stop_grind_before_blocked_evolution(EvolutionPolicyType::Trade, 15, 16), true);
    TEST_RESULT_EQUAL(should_stop_grind_before_blocked_evolution(EvolutionPolicyType::LevelUp, 15, 16), false);
    TEST_RESULT_EQUAL(should_stop_grind_before_blocked_evolution(EvolutionPolicyType::Stone, 15, 16), false);
    TEST_RESULT_EQUAL(should_stop_grind_before_blocked_evolution(EvolutionPolicyType::Block, 15, 0), false);   //  no level-up evolution at all

    //  Evolution stone item names round-trip through the dropdown database.
    TEST_RESULT_EQUAL(evolution_stone_item_name(EvolutionStoneItem::None), std::string());
    TEST_RESULT_EQUAL(evolution_stone_item_name(EvolutionStoneItem::FireStone), std::string("Fire Stone"));
    TEST_RESULT_EQUAL(EvolutionPolicyType_Database().find(EvolutionPolicyType::Block) != nullptr, true);
    TEST_RESULT_EQUAL(EvolutionStoneItem_Database().find(EvolutionStoneItem::Thunderstone) != nullptr, true);

    return 0;
}

}
