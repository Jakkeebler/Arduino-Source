/*  PokemonFRLG Tests
 *
 *  From: https://github.com/PokemonAutomation/
 *
 *
 */


#ifndef PokemonAutomation_Tests_PokemonFRLG_Tests_H
#define PokemonAutomation_Tests_PokemonFRLG_Tests_H

#include <vector>
#include <string>

namespace PokemonAutomation{

class ImageViewRGB32;


int test_pokemonFRLG_AdvanceWhiteDialogDetector(const ImageViewRGB32& image, bool target);

int test_pokemonFRLG_ShinySymbolDetector(const ImageViewRGB32& image, bool target);

int test_pokemonFRLG_SelectionDialogDetector(const ImageViewRGB32& image, bool target);

int test_pokemonFRLG_AdvanceBattleDialogDetector(const ImageViewRGB32& image, bool target);

int test_pokemonFRLG_BattleMenuDetector(const ImageViewRGB32& image, bool target);

int test_pokemonFRLG_PrizeSelectDetector(const ImageViewRGB32& image, bool target);

int test_pokemonFRLG_PartySummaryReader_dex(const ImageViewRGB32& image, int target);
int test_pokemonFRLG_PartySummaryReader_moves(const ImageViewRGB32& image, const std::vector<std::string>& expected);
int test_pokemonFRLG_LearnMoveDialogReader(const ImageViewRGB32& image, const std::vector<std::string>& expected);
int test_pokemonFRLG_ForgetMoveScreenDetector(const ImageViewRGB32& image, bool target);
int test_pokemonFRLG_ForgetMoveScreenReader(const ImageViewRGB32& image, const std::vector<std::string>& expected);

}

#endif
