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

}
