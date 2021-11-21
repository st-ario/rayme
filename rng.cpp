#include "rng.h"

#include <random>
#include <thread>
#include <bitset>
#include <cstring>


float random_float()
{
  static thread_local std::mt19937_64 generator(std::clock()
    + std::hash<std::thread::id>()(std::this_thread::get_id()));
  static std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
  return distribution(generator);
}

float sampler_1d::rnd_float()
{
  #ifdef STD_RNG
  return random_float();
  #else
  uint32_t n{rnd_uint32()};

  // convert the uint32 into a float:
  // mask out the 23 bits of the mantissa, and fix the first 9 bits to make it positive and
  // concentrated in [1,2) (set the exponent to 127)
  std::bitset<32> float_bits{(n & 0x007FFFFF) | 0x3F800000};
  float res;
  std::memcpy(&res,&float_bits,4);

  // shift the result to make it concentrated in [0,1)
  return res - 1.0f;
  #endif
}


uint32_t sampler_1d::rnd_uint32()
{
  #ifndef STD_RNG
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

  return n;
  #else
  static thread_local std::mt19937_64 generator(std::clock()
    + std::hash<std::thread::id>()(std::this_thread::get_id()));
  static std::uniform_int_distribution<uint32_t> distribution(0);
  return distribution(generator);
  #endif
}

#ifndef STD_RNG
inline uint32_t sampler_1d::rnd_uint32(uint32_t range)
{
  // unbiased fast ranged rng, due to D. Lemire and M.E. O'Neill
  // (see https://www.pcg-random.org/posts/bounded-rands.html)
  uint32_t n{rnd_uint32()};
  uint64_t m{uint64_t(n) * uint64_t(range)};
  uint32_t l{uint32_t(m)};
  if (l < range)
  {
    uint32_t t{-range};
    if (t >= range)
    {
        t -= range;
        if (t >= range)
            t %= range;
    }
    while (l < t)
    {
        n = rnd_uint32();
        m = uint64_t(n) * uint64_t(range);
        l = uint32_t(m);
    }
  }
  return m >> 32;
}
#endif


std::array<float,2> sampler_2d::rnd_float_pair()
{
  #if defined PMJ02_RANDOM_PAIRS && !defined STD_RNG

  // shuffling indices keeping the stratification
  static uint16_t shuffle_filter{uint16_t(rng_1d.rnd_uint32(SIZE_RNG_SAMPLES-1) >> 16)};

  // scramble_filter, to scramble the (mantissa bits of) the sample
  // see Fredel--Keller, Fast Generation of Randomized Low-Discrepancy Point Sets
  // and Kollig--Keller, Efficient Multidimensional Sampling
  static uint32_t scramble_filter{rng_1d.rnd_uint32() >> 9};

  // pick samples pool based on seed
  static uint16_t sample{uint16_t(seed % N_RNG_SAMPLES)};

  float res0{(*samples_2d[sample])[index ^ shuffle_filter][0]};
  float res1{(*samples_2d[sample])[index ^ shuffle_filter][1]};

  uint32_t temp0{reinterpret_cast<uint32_t&>(res0) ^ scramble_filter};
  uint32_t temp1{reinterpret_cast<uint32_t&>(res1) ^ scramble_filter};

  std::memcpy(&res0,&temp0,4);
  std::memcpy(&res1,&temp1,4);

  ++index;

  // if the index overflows, move to the next pool and start back from top
  if (index > SIZE_RNG_SAMPLES - 1u)
  {
    ++offset;
    sample += offset;
    sample %= N_RNG_SAMPLES;
    index = 0;
  }

  return std::array<float,2>{res0,res1};
  #else
  return std::array<float,2>{random_float(), random_float()};
  #endif
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