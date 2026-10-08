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
#include "PokemonFRLG/Programs/Farming/PokemonFRLG_MoveLearnDecider.h"
#include "PokemonFRLG/Programs/Farming/PokemonFRLG_MoveLearnStateMachine.h"
#include "PokemonFRLG/Programs/Farming/PokemonFRLG_MovePlan.h"
#include "PokemonFRLG/Programs/PokemonFRLG_WalkingDriftGuard.h"
#include "PokemonFRLG/Programs/PokemonFRLG_TravelDecisionTable.h"
#include "PokemonFRLG/Programs/PokemonFRLG_MenuConfirmGuard.h"
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


//  ---- FRO-225 Phase 2: walking drift, collision, travel decision, menu confirm ----
//
//  Registered as "PokemonFRLG_WalkingAndTravelLogic". Pure logic, no image/file
//  data needed, so the test always runs regardless of what file (if any) is
//  dropped in its Tests/ folder.

namespace{

int test_drift_guard(){
    //  Exact match: no drift, no re-localize.
    {
        DriftCheckResult r = check_walk_drift(10, 10, 10, 10, false);
        TEST_RESULT_EQUAL(r.drift_tiles, 0);
        TEST_RESULT_EQUAL(r.should_relocalize, false);
    }
    //  1-tile drift in either axis must re-localize immediately -- this is
    //  the acceptance criteria's "within 1 tile of actual drift", not after
    //  waiting for the strand guard to trip on repeated non-movement.
    {
        DriftCheckResult r = check_walk_drift(10, 10, 11, 10, false);
        TEST_RESULT_EQUAL(r.drift_tiles, 1);
        TEST_RESULT_EQUAL(r.should_relocalize, true);
    }
    {
        DriftCheckResult r = check_walk_drift(10, 10, 10, 9, false);
        TEST_RESULT_EQUAL(r.drift_tiles, 1);
        TEST_RESULT_EQUAL(r.should_relocalize, true);
    }
    //  Diagonal drift reports Chebyshev distance, not Manhattan.
    {
        DriftCheckResult r = check_walk_drift(10, 10, 13, 14, false);
        TEST_RESULT_EQUAL(r.drift_tiles, 4);
        TEST_RESULT_EQUAL(r.should_relocalize, true);
    }
    //  An interrupted step (encounter/fade) never targeted the predicted
    //  tile in the first place: must not be reported as drift.
    {
        DriftCheckResult r = check_walk_drift(10, 10, 7, 10, true);
        TEST_RESULT_EQUAL(r.drift_tiles, 0);
        TEST_RESULT_EQUAL(r.should_relocalize, false);
    }
    //  drift_check_due: interval <= 1 means "always due".
    TEST_RESULT_EQUAL(drift_check_due(0, 1), true);
    TEST_RESULT_EQUAL(drift_check_due(0, 0), true);
    TEST_RESULT_EQUAL(drift_check_due(3, 4), false);
    TEST_RESULT_EQUAL(drift_check_due(4, 4), true);
    return 0;
}

int test_collision_guard(){
    //  Reroutes within 2 unexplained stalls (the acceptance criteria), and
    //  resets cleanly once movement resumes.
    CollisionGuard guard(2);
    TEST_RESULT_EQUAL((int)guard.report(true, false), (int)CollisionDecision::NotACollision);
    TEST_RESULT_EQUAL(guard.consecutive_unexplained_stalls(), 0);
    TEST_RESULT_EQUAL((int)guard.report(false, true), (int)CollisionDecision::NotACollision);   //  expected non-move (turn)
    TEST_RESULT_EQUAL(guard.consecutive_unexplained_stalls(), 0);
    TEST_RESULT_EQUAL((int)guard.report(false, false), (int)CollisionDecision::KeepTrying);     //  1st unexplained stall
    TEST_RESULT_EQUAL((int)guard.report(false, false), (int)CollisionDecision::Reroute);        //  2nd: threshold reached
    guard.reset();
    TEST_RESULT_EQUAL(guard.consecutive_unexplained_stalls(), 0);
    //  A legitimate movement in between resets the streak, so the next
    //  stall only counts as the 1st again.
    TEST_RESULT_EQUAL((int)guard.report(false, false), (int)CollisionDecision::KeepTrying);
    TEST_RESULT_EQUAL((int)guard.report(true, false), (int)CollisionDecision::NotACollision);
    TEST_RESULT_EQUAL((int)guard.report(false, false), (int)CollisionDecision::KeepTrying);
    return 0;
}

int test_travel_decision_table(){
    //  Branch 1: Fly available and sufficiently charged wins outright, even
    //  when Teleport would also apply.
    {
        TravelOptions o;
        o.fly_spot_unlocked = true;
        o.fly_user_pp = 3;
        o.fly_pp_required = 1;
        o.teleport_available = true;
        o.destination_is_last_visited_pokecenter = true;
        TEST_RESULT_EQUAL((int)choose_travel_method(o), (int)TravelMethod::Fly);
    }
    //  Fly locked out (not enough PP) falls through to Teleport, but only
    //  when the destination actually IS the last-visited Pokemon Center.
    {
        TravelOptions o;
        o.fly_spot_unlocked = true;
        o.fly_user_pp = 0;
        o.fly_pp_required = 1;
        o.teleport_available = true;
        o.destination_is_last_visited_pokecenter = true;
        TEST_RESULT_EQUAL((int)choose_travel_method(o), (int)TravelMethod::Teleport);
    }
    //  Teleport "available" but destination is NOT the last-visited PC: it
    //  would land in the wrong place, so this must fall through to walking,
    //  not be treated as usable for this trip.
    {
        TravelOptions o;
        o.teleport_available = true;
        o.destination_is_last_visited_pokecenter = false;
        TEST_RESULT_EQUAL((int)choose_travel_method(o), (int)TravelMethod::Walk);
    }
    //  Walk-only: neither Fly nor Teleport apply.
    {
        TravelOptions o;
        TEST_RESULT_EQUAL((int)choose_travel_method(o), (int)TravelMethod::Walk);
    }
    //  Explicit Dig escape pre-empts the whole chain, even when Fly is also
    //  fully available -- Dig must never be chosen implicitly otherwise.
    {
        TravelOptions o;
        o.fly_spot_unlocked = true;
        o.fly_user_pp = 5;
        o.fly_pp_required = 1;
        o.dig_explicit_escape_requested = true;
        TEST_RESULT_EQUAL((int)choose_travel_method(o), (int)TravelMethod::Dig);
    }
    {
        TravelOptions o;
        TEST_RESULT_EQUAL(std::string(travel_method_name(TravelMethod::Fly)), std::string("Fly"));
        TEST_RESULT_EQUAL(std::string(travel_method_name(TravelMethod::Teleport)), std::string("Teleport"));
        TEST_RESULT_EQUAL(std::string(travel_method_name(TravelMethod::Walk)), std::string("Walk"));
        TEST_RESULT_EQUAL(std::string(travel_method_name(TravelMethod::Dig)), std::string("Dig"));
    }
    return 0;
}

int test_menu_confirm_guard(){
    using Outcome = MenuConfirmGuard::Outcome;
    using Step = MenuConfirmGuard::Step;

    //  A clean read-back confirms immediately.
    {
        MenuConfirmGuard guard(3);
        TEST_RESULT_EQUAL((int)guard.report(Outcome::Verified), (int)Step::Confirm);
    }
    //  Mismatch retries, not confirms -- an unverified selection must never
    //  silently proceed.
    {
        MenuConfirmGuard guard(3);
        TEST_RESULT_EQUAL((int)guard.report(Outcome::Mismatch), (int)Step::Retry);
        TEST_RESULT_EQUAL(guard.attempts(), 1);
        TEST_RESULT_EQUAL((int)guard.report(Outcome::Unreadable), (int)Step::Retry);
        TEST_RESULT_EQUAL(guard.attempts(), 2);
        //  Budget exhausted on the 3rd bad attempt.
        TEST_RESULT_EQUAL((int)guard.report(Outcome::Mismatch), (int)Step::GiveUp);
    }
    //  A late success still confirms (no sticky failure state).
    {
        MenuConfirmGuard guard(3);
        guard.report(Outcome::Mismatch);
        TEST_RESULT_EQUAL((int)guard.report(Outcome::Verified), (int)Step::Confirm);
    }
    //  reset() clears the attempt count.
    {
        MenuConfirmGuard guard(2);
        guard.report(Outcome::Mismatch);
        guard.reset();
        TEST_RESULT_EQUAL(guard.attempts(), 0);
        TEST_RESULT_EQUAL((int)guard.report(Outcome::Mismatch), (int)Step::Retry);
    }
    return 0;
}

}  //  namespace

int test_pokemonFRLG_WalkingAndTravelLogic(const std::string& test_file_path){
    (void)test_file_path;
    for (int (*scenario)() : {
        test_drift_guard,
        test_collision_guard,
        test_travel_decision_table,
        test_menu_confirm_guard,
    }){
        int result = scenario();
        if (result != 0){
            return result;
        }
    }
    return 0;
}

}
