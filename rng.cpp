#include "rng.h"

#include <random>
#include <thread>
#include <bitset>
#include <cstring>

float sampler_1d::rnd_float()
{
  #ifdef STD_RNG
  return random_float()
  #else
  //xoroshiro128+

  static thread_local std::array<uint64_t,2> s{
    seed ^ uint64_t(0xBD6CB273039BC9C0),
    ((seed >> 32) | (seed << 32)) ^ uint64_t(0xC7B38377DB5B4910)
  };

  const uint64_t s0{s[0]};
  uint64_t s1{s[1]};
  const uint32_t n{uint32_t((s0 + s1) >> 32)};

  // update seed
  s1 ^= s0;
  s[0] = ((s0 << 24) | (s0 >> 40)) ^ s1 ^ (s1 << 16);
  s[1] = (s1 << 37) | (s1 >> 27);

  // convert the uint32 into a float:
  // mask out the 23 bits of the mantissa, and fix the first 9 bits to make it positive and
  // concentrated in [1,2) (set the exponent to 127)
  std::bitset<32> float_bits{(n & 0x007FFFFF) | 0x3F800000};
  float res;
  std::memcpy(&res,&float_bits,32);

  // shift the result to make it concentrated in [0,1)
  return res - 1.0f;
  #endif
}

float random_float()
{
  static thread_local std::mt19937_64 generator(std::clock()
    + std::hash<std::thread::id>()(std::this_thread::get_id()));
  static std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
  return distribution(generator);
}

std::array<float,2> random_float_pair()
{
  return std::array<float,2>{random_float(), random_float()};
}

uint64_t next_seed(uint64_t old_seed)
{
  // inspired by xoroshiro128+, probably not as effective but doesn't matter for seed generation

  uint64_t s1{((old_seed >> 32) ^ uint64_t(0x93BBC663)) | ((old_seed ^ uint64_t(0x110E560E)) << 32)};

  s1 ^= old_seed;
  old_seed = ((old_seed << 24) | (old_seed >> 40)) ^ s1 ^ (s1 << 16);
  s1 = (s1 << 37) | (s1 >> 27);

  return old_seed + s1;
}