/*  Xoroshiro128+ and reverse
 *
 *  From: https://github.com/PokemonAutomation/Arduino-Source
 *
 */

#include <cstddef>
#include "Pokemon_Xoroshiro128Plus.h"

namespace PokemonAutomation{
namespace Pokemon{

Xoroshiro128PlusState::Xoroshiro128PlusState(uint64_t s0, uint64_t s1)
    : s0(s0)
    , s1(s1)
{}


Xoroshiro128Plus::Xoroshiro128Plus(Xoroshiro128PlusState state)
    : state(state)
{}

Xoroshiro128Plus::Xoroshiro128Plus(uint64_t s0, uint64_t s1)
    : state(Xoroshiro128PlusState(s0, s1))
{}

uint64_t Xoroshiro128Plus::rotl(const uint64_t x, const int k){
    return (x << k) | (x >> (64 - k));
}

uint64_t Xoroshiro128Plus::next(){
    const uint64_t s0 = state.s0;
    uint64_t s1 = state.s1;
    const uint64_t result = s0 + s1;

    s1 ^= s0;
    state.s0 = rotl(s0, 24) ^ s1 ^ (s1 << 16);
    state.s1 = rotl(s1, 37);

    return result;
}

Xoroshiro128PlusState Xoroshiro128Plus::get_state(){
    return state;
}

uint64_t nextPowerOfTwo(uint64_t number){
    uint64_t x = number;
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    x++;
    return x;
}

uint64_t Xoroshiro128Plus::nextInt(uint64_t bound){
    uint64_t power = nextPowerOfTwo(bound);
    uint64_t result = next() & (power - 1);
    while (result >= bound){
        result = next() & (power - 1);
    }
    return result;
}

std::vector<bool> Xoroshiro128Plus::generate_last_bit_sequence(size_t max_advances){
    std::vector<bool> sequence(max_advances);
    Xoroshiro128Plus temp_rng(Xoroshiro128PlusState(state.s0, state.s1));

    for (size_t i = 0; i < max_advances; i++){
        sequence.at(i) = (temp_rng.next() & 1) == 1;
    }

    return sequence;
}

// The generic solution to the system of equations to calculate the initial state from the last bits of 128 consecutive Xoroshiro128+ results.
uint64_t Xoroshiro128Plus::last_bits_reverse_matrix[128][2] = {
    /*s0 bit 0*/ {0b0101001100100001111011111110111001010011111110101011100011001101, 0b0111010111110111000101010100001111101001111001011111001011010111} ,
    {0b0101010001010011100010011010100000100100111101101001100001100000, 0b1000110000001001010011000111001111111111111101110011110001000000},
    {0b0110010000111101101000100110010000011111010111011011100100011001, 0b0001001011110010110110010101101001011110101110010001100111010111},
    {0b0001010111110001100111010110010001000000110101100100001111000111, 0b0000111010011000110111101101011111110010010111001110110100000001},
    {0b0110101010001010010010010111001010100111110010110001110111010000, 0b1101010010101000010001000101100011010101101000011000010011111110},
    {0b0101111110111111110111101011001011100111110010010100001101010000, 0b1100010100111110001001011000001101010100000101010000110111000000},
    {0b1111100100110000110011011000111100000010110111001110111100110000, 0b1101011010100111000100100000001010000111011001100101001001110100},
    {0b1111001001000110100100100111100110010101101110111010111011110010, 0b1000110011100101011101110110100100100110110000001111011111010101},
    {0b1000101111010110110001111000100011001001011001111111010001011110, 0b1110010101111000110001000000001001111110111100101001111111110101},
    {0b0011011111110111010101001010100101110101110011110001010000000110, 0b1001101100000101011110000110111010000000011110101011001110011000},
    /*s0 bit 10*/{0b1100011000100101110010010000010101011101001101101101010111000011, 0b0010101110110011101000100011110010010010111000110111101010100100},
    {0b0110000010110101110001110000100100111001111001000100101101001101, 0b0010011110100101001001100101011010101111101100100111000011001000},
    {0b1110110100010111101100110100000010110110000011100110001000011110, 0b0111000000001101101000100001011000100010101110011110101011011111},
    {0b1010100011101110101010100100100101001000101000111010101001011100, 0b1001011110110001011101001000101001010001011010000100011101010000},
    {0b0010000111110000101111010001010001101111001100001011010011100001, 0b1010101000101111110011001100110001010110001001011101010111101011},
    {0b0100010001011100011101011101110110101100101110001110111111100000, 0b0111101101010000011111111011110010101101100010010010111010001110},
    {0b0111111111001100001101000111011100110100101010011100001010011010, 0b0110100110110000111011110010100000101101011100010000111111101111},
    {0b0000100111110100110110011101110111111001110101001001010010010111, 0b1110000110010111000010101110100111001010110110011110110111011100},
    {0b1001000011111110010011100000011010010001001011110010101100100010, 0b1011110101011010110010111010111110101110011111011010111111110000},
    {0b0111100010101001011100110101100110111110100111110110110111010011, 0b1110111101010000001110110011000111111100100010111011100001100110},
    /*s0 bit 20*/{0b1110100101000011101010001110100100111110011111001110000110100000, 0b0100100001111100111100101100000110011011010011110001001000110101},
    {0b0011000101010101001110010101110000010011011100101100010100001000, 0b1101010100100011000001110111100010000010100011101101101010100010},
    {0b0100011001010010001110110000011001011110011010111011111110100110, 0b1010100001010100001100101111101100101001010111101111100001111110},
    {0b0101100011010100101011000110000110001011010100100111001011010010, 0b0101101111010000001110101101010001001101101001001010111111000001},
    {0b0010111110010011001001110101001001001011110100101101001010000001, 0b0111100010111111111111100010010101001101010101110110000101101010},
    {0b0001011001011000101011100111011010111011100111000000111010100000, 0b0101011010010110101110001110010001000100000000111001110000000111},
    {0b0000111111111001100000000000111011000010010101001111100111111001, 0b1100111101001010010010100000101100011110011001101101101001010111},
    {0b0011100110110100000010011101100000110000001110000010011001110000, 0b1100010010011001000100100111111100001000000010101010010111110010},
    {0b1010011000110011100101100001110000000111011000110011001011110001, 0b0111000101010101011001100111010101100110110000000100101110010001},
    {0b1000000111000001100010011001010101000010010010001011110101101100, 0b0110000111100001101111001010110000011011111000101000011011011110},
    /*s0 bit 30*/{0b1000010011101001111010100010011101101001011010010010010111110101, 0b0111111100110100111110111110001110100000111001100010000011010011},
    {0b1111011100001000010011011100010000000101011000011001001100100111, 0b1000110100100100010101000000111000101000000101011000010011001000},
    {0b0100000111001101011011110101001011010001110101011001100001011000, 0b1110010100000101011010111100100111001111011011111110111101111010},
    {0b0101000111111010111101001011101000011110011101010000010001100011, 0b0111110110010111100110001000100100011110100011100110000010101100},
    {0b1100000011101010100010000111001001110011011101000111010010001110, 0b0001110001111001001111111010101110101011001101001000100010101111},
    {0b0000100101111111110100100010111110001111000011001010000000000111, 0b0010101110100110110001001111000110010101111010010011010110111110},
    {0b0010001110010011010010110110110110110111101110001001110101010001, 0b1001001100101001101011100101011100010110101110011000000101101101},
    {0b1011011101111100011111010101000111010011001000100111010111010111, 0b0010001101010001100110100001001010111111110011100101110000110000},
    {0b0111101101100000011000110010011011010010011111111110101111011111, 0b1000111000100111100001101000100010100111010100110111011000100000},
    {0b0010101001101100100000010100111101100110000101000111100111110100, 0b1101010110010111110010001011010110100001101010110010111110011100},
    /*s0 bit 40*/{0b0111100001010010101011110110000100100111000111101000001010001101, 0b1110001000110001011011101111111101011101011110010111001110111011},
    {0b1011100000011000010000000110111010000100110111000111000010101000, 0b1001111111000111011010100010011110111101000100000110011110100101},
    {0b0000100010001111110001011110101011111011101011001110100010101110, 0b0011011010000110001111100010001011011010000000011110010011110010},
    {0b0110110011110000010000001010000100110101000111001000010101110101, 0b0111011110100101111011111011010010101001110101000111100111100001},
    {0b0111000110101000111100011110111111110000001100100111100101001111, 0b0001111011110001111101100011110000001010000100111011111011001010},
    {0b1001111100011001000010001100001110010110101100000001100010011001, 0b0010101111000000101001000110001100011010110110000101010110011111},
    {0b1001100111000101100011001111101110110010100110111111111110000110, 0b0000011101001101011110010010010101110110100001001001001000010110},
    {0b1000111010100011101100101101110010110100000110011101101010010001, 0b0010001100010110010100010100010011000001110000110101101010100101},
    {0b0110110001011110000010110101110010111101000110101000110110011000, 0b1000110101110100110100100001000000001110100111011100111100111010},
    {0b0110111010011100111011111011000001011011110110001111111011111011, 0b1111110110011000100001100000110001000000010011000010001010110101},
    /*s0 bit 50*/{0b0100010010110110110111011011110010011111000100111001000000101100, 0b1001011111100111000011000010101110011100111000010111110001010110},
    {0b0100010110011001111011001101111111011001011001111010110111110010, 0b0101010111111000100111011111110101010110100100001011111011101001},
    {0b0111110001100101001000000101001011001001110111111000110101110110, 0b0101110111101000110000100100001011011001000000101100010011011101},
    {0b0010010111110110111001111110010111101110001010001011000110101001, 0b1111011010110111100010111101010101101110101111110011110101010111},
    {0b1011100111000100010001001111000100100001000100011001110100111111, 0b1110101110001011110110011001010010010101000110010001010001001001},
    {0b0010010011001000000011110110010011000111001001100100110100110110, 0b0000100100100111000111101110101100000111011111110010111111010100},
    {0b0101000101000011101011011110101110111010100111100101010101000101, 0b0100101110001011111010011111001010100111000101111111011110000000},
    {0b1110011101110010110010110100001001000111011110100110100101010010, 0b0100000110001111100011100001000011110101101011010100000110011100},
    {0b0111100001110000111100100011100011111001101101110011100111101001, 0b1110111101001000000101101011111000000111011001110111111011101010},
    {0b0100000000010101110011101100101011111011001000011001100100011001, 0b1100010011111111001010001110010111101001101000000101000010001001},
    /*s0 bit 60*/{0b0101101000111000001010101011111010000010011011111001100011011111, 0b1100110010111101001011101100100010001000110000001110011111000110},
    {0b1000111011001110110011100010100110110010101001100111011101101011, 0b0000000010001010101110001011000110101000100111100000010010000100},
    {0b1000110101011110011101010001001010111101101011001000100001001100, 0b1101001001010111001100111110110111100101100000111100001011001101},
    {0b1011000110100011111010110001011111001011010001110101000010000101, 0b1110000000100101110011001011110001111001110010110111111110000011},

    /*s1 bit 0*/{0b1101111011001010010100100111010111001001000000010111111000010001, 0b1111101110010001100010011011000000100010100111011110011011001111},
    {0b1001101111110011000001000000111010001011101101011011101010011011, 0b1100001011011000000110000100101110110111100001000110110000100100},
    {0b1110010101010001000000000110101110110011001011101111111100101000, 0b1011110111011110000010010100101111101100110100101000001001010011},
    {0b0111000011001010101101011011100001010011001001100010001101100110, 0b1001000100110101011100111101101001110011010001110111111001001011},
    {0b1110010000100100001010111100011101111001111000100000000000010100, 0b0000010101110100011010010000001100100010111000001000010100000001},
    {0b1001100101110010010111001100001111000011000111010100000000010001, 0b1101000100011011101101000110001111011110011011011101011001010111},
    {0b1111011001010100010100100001000000110010101011110100000001000101, 0b0111000011001011000011001001110101011101110110101000001110100110},
    {0b0010110000001001101010111011001010011100110100011010110001010000, 0b1011110001100000000011001101001011111101100011000101010010011011},
    {0b1100110000000010000010010011011111001101011000010110001101000010, 0b0000101101110000010110010010010100011111011100011111111111011111},
    {0b0110101001010100101001110010111101101011101110100110001111000111, 0b0111111101111001101100100010111100101101111101000010010011001110},
    /*s1 bit 10*/{0b0110101000101110100101100010111110011111011011101000011101110100, 0b0011101011011100101111111001011010001100010000101011010100001111},
    {0b0111101111111001001101111001100111110010001101101101000000000100, 0b0001000000000000111000110000000000010001111110101101000011010001},
    {0b0000000101110010010001101110001010110000010001101101011011110110, 0b1100101110010010011101101101000101000000101110010100010110000101},
    {0b1100100011110111111111000110001011111000100111011000111010001011, 0b1001011100000101111100010100011001011011101001011010100100011111},
    {0b1000010011110010011000101100110110101010101011110011101101101000, 0b0001101010110010110100010101000000001001111101000101111101111001},
    {0b1010011001000000111101010000011101111000001000001111110001101011, 0b1110101001110110001111110111101001111110111100110011000101100010},
    {0b0111010000100001000000001010001110100010100011000101000000110001, 0b1000111001010000110110111101010100101110111011011001001111110110},
    {0b1111101000100101001111101101100111111111011100001111010011111100, 0b1011111111010100010000000000110000111001010100101011111110011111},
    {0b1010110001110000111001001101111110011110010101111001001011110110, 0b1111111110000111111000011001001001011100011011010111001011001000},
    {0b1010111011011011110001010000010111010011110011011101000001110001, 0b1001100011101010001101111010001011111101111101011110100001011110},
    /*s1 bit 20*/{0b1111010010011011011110001111100110110100110100110100000001111101, 0b1000001010100001100110111100001101100000000111101001101000100100},
    {0b0101100110000110110111011110000100000110000010101101101010001101, 0b1100001001111000001110001111010110111001000111100000000100001011},
    {0b1011000101000111001010000111100110000110000111110100001010111110, 0b0001101001011111010111101000011000010000101101110010000001000101},
    {0b1000100010110101110010001011100000100011011101011110111010111110, 0b0000101111000001011100100101111101011100110111001110001110101101},
    {0b1110110001100100101111001001000110010100010000111111110101111010, 0b1010101011001100101101100100000010101101100000111011000111000000},
    {0b0000000111111010000000110001111000000001011110111111011100000001, 0b1000001011111111010101100001101110110111001111100011001000110001},
    {0b0000000000001010101100111101010011011101111000111011010100100001, 0b0101011110000101101110010001010011100010100000100010000000000001},
    {0b0001101111011101110010001110001111101000000101000011100010011000, 0b0100101101001000100000001111001101110100011100100000100000110001},
    {0b1001111101001011101010001001100110000011011001011111000011010111, 0b1100101000100111000100010110010001110010011001001000000011001001},
    {0b0000001011010011111101111100101110000101000001010011100101000010, 0b0000100111011100000110010011011110000110010101010001011100001001},
    /*s1 bit 30*/{0b1100101001110110010100011011100000100111111000001100000101000011, 0b0011111101011011010110100001101100000010001101110010011010010100},
    {0b0001110101010110011101101011111101100001101100011000111010101000, 0b1111010000111101111000111010001100001100000000000010001111111111},
    {0b1000110110010001101101110011011010010100010010111011001110100011, 0b0111111111001110100110101101010111110110011100111001011100101111},
    {0b0001111011001001001111110011111001100000111001110101111011101011, 0b0100110011011000001111010011111110110101011110011010001110100100},
    {0b1011110010010100110000000100001011001111001101111011000001100101, 0b0011011010001111010011100110001101010100000110110110011010011101},
    {0b1000111110101001100111010111111000001000000011010010111000111001, 0b1101110000010001001110100100111011000011000001101100000001010100},
    {0b1011101101000111111001110000110000111100111010101110111110000011, 0b1100100011111001100101001000001101011011000111010010111010101100},
    {0b0101100000011100000011011000000101011001010100110001000001001111, 0b0111010101011011100000100100000011011111110000011011111101010111},
    {0b0011011010011001111000010010000110010111101001010011011010010010, 0b0110111101001011100010101010110101111100100100010100000000110010},
    {0b1101100011000001010110001001000011010001011100101101110011110000, 0b0010000010111010000100001001101000100110100000110111111010110101},
    /*s1 bit 40*/{0b1100000000110010101011000101011101100000011111000100100110101110, 0b0000000101101001000010111001100000010101100110111101110010011110},
    {0b0100101000001111000011000110011101010110110111001010101000110010, 0b0011011010111111100000100010110001011101001000010011010100100101},
    {0b1100010000110011101100100110000101110100110100111001001000110110, 0b0111010111001001001110001001100101000100011101100001111111011001},
    {0b0001011111011010011010011010100100101100010010110010010101010111, 0b1100111111000000011010011111101000000111001110010011100000110010},
    {0b1110011110101001011111011101110011010000101010110111010111110111, 0b1110101100000011001011001101111100000101100101001000010010000111},
    {0b0111100100011101010101011011001000011110111100010111001110101000, 0b1000010110111010010101000111101111100100001000011011101001110000},
    {0b1010110011101111110111110110110000000111010001101100111001100101, 0b1011100011110001101000000011001011100000011111101000000001110001},
    {0b0011101110110001101000000010000100010101010111110100001110111011, 0b1001010110111010110100100000010111110110101000110001000000100010},
    {0b1101000110100111110010010111101000101010111100000011111100001010, 0b0010111010110110011111110001101001110111001000011011011101010010},
    {0b1110111000100000100101010010101101101101000010100100111100010000, 0b0011001110010011011000011110111010010001010100011001000001110111},
    /*s1 bit 50*/{0b1010000011001000011110100110011110001101101011001000110111111001, 0b1111011110100111001010000000001011000000011100101011100011011001},
    {0b1000011111100101100001000101001101100010110000011110101011010110, 0b1111001101100011001010011101111100100011001010111000000101010101},
    {0b0010111101000101010110101101000101110101110011111111001101000011, 0b1010100011010011110111011111111111100110011110110101110001101100},
    {0b0001111111100110011001111011010000111111011011101001100110110001, 0b0011000110011111111001100011111111111001110010011111010010101100},
    {0b0100010011010011100000100111011110110000010110000011110111010001, 0b0001111110100011001001010001100011111000111100010101101110000110},
    {0b0111001011110000011111010000101100001000000011011000010001001101, 0b0111011011100100111011100010001000101001010010011001011010110000},
    {0b0000011000101110010011110110100101010011011110001011110101111101, 0b1000011111111110111100100010001111011001111011010000001110101111},
    {0b1001110100111111011101110011111010011101000100111100100101101100, 0b1000000011011111010000111111000101000101101000100000110010001110},
    {0b1101010010110110110001010010001000010100010000111000111000111111, 0b1010010110110110110101100100111010101010101010010110111001101111},
    {0b1001100010110011000000110001110001010110110111111110001011010110, 0b0110001000010010010110110010110010010000101110101101000010101011},
    /*s1 bit 60*/{0b1001111001100111100101110000100011111101001001001011010100010000, 0b0110001010011111010010110110110101010111111011111000011000010000},
    {0b1111100011111100100100000110000101111000100001111000100111010110, 0b0101010111101100111000111001000111111110010111111101110001100100},
    {0b0000110110100110001011010111011111010011111000001010100101011100, 0b1011100011101010010001000110101001001111010111011100101111010101},
    {0b0011000110100011111010110001011111001011010001110101000010000101, 0b1110000000100101110011001011110001111001110010110111111110000011}
};


Xoroshiro128Plus Xoroshiro128Plus::xoroshiro128plus_from_last_bits(std::pair<uint64_t, uint64_t> last_bits){
    uint64_t s0 = 0;
    uint64_t s1 = 0;

    for (size_t i = 0; i < 64; i++){
        uint64_t first_half = last_bits_reverse_matrix[i][0] & last_bits.first;
        uint64_t second_half = last_bits_reverse_matrix[i][1] & last_bits.second;

        uint64_t x = first_half ^ second_half;
        x ^= x >> 32;
        x ^= x >> 16;
        x ^= x >> 8;
        x ^= x >> 4;
        x ^= x >> 2;
        x ^= x >> 1;
        x &= 1;

        s0 += x << (63 - i);
    }
    for (size_t i = 0; i < 64; i++){
        uint64_t first_half = last_bits_reverse_matrix[i + 64][0] & last_bits.first;
        uint64_t second_half = last_bits_reverse_matrix[i + 64][1] & last_bits.second;

        uint64_t x = first_half ^ second_half;
        x ^= x >> 32;
        x ^= x >> 16;
        x ^= x >> 8;
        x ^= x >> 4;
        x ^= x >> 2;
        x ^= x >> 1;
        x &= 1;

        s1 += x << (63 - i);
    }
    Xoroshiro128PlusState state(s0, s1);
    return Xoroshiro128Plus(state);
}


}
}
