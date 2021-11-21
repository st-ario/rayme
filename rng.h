#pragma once

#include <array>
#include <cstdint>
#include <random>

// debug macros
// replaces all rngs by the one used by random_float()
//#define STD_RNG 1
// if not defined, random_float_pair() generates unstratified pairs
//#define PMJ02_RANDOM_PAIRS 1 // TODO currently broken, reintroduce after rng restructuration

// slower than the one given by the sampler_1d class, to be used only if there's no reasonable
// way to use that class in a thread-safe seed-based way
float random_float();
std::array<float,2> random_float_pair();

//constexpr uint16_t N_RNG_SAMPLES{401};
//constexpr uint16_t SIZE_RNG_SAMPLES{4096};

//extern std::array<const std::array<std::array<float,2>,SIZE_RNG_SAMPLES>*,N_RNG_SAMPLES> samples_2d;

class sampler_1d
{
  public:
    sampler_1d() = default;
    sampler_1d(uint64_t seed)
    : seed{seed} {}

    float rnd_float();

  private:
    uint64_t seed;
};

uint64_t next_seed(uint64_t old_seed);

// DEAD CODE
// TODO clean-up

/*
// macros determining which prng is used
// always comment all but one

//#define XORSHIFT_RANDOM 1
#ifdef XORSHIFT_RANDOM
#include <bitset>
#endif

//#define XOROSHIRO128PLUS_RANDOM 1
#ifdef XOROSHIRO128PLUS_RANDOM
#include <bitset>
#endif

#define STD_RNG 1
#ifdef STD_RNG
#include <random>
#endif

uint32_t random_uint32_t()
{
  #ifdef XORSHIFT_RANDOM
  // xorshift
  // seed depending on x, y, and z that is nontrivial even if they're all 0
  static thread_local uint32_t hash{
    ((uint32_t(x) ^ (uint32_t(z) << 8)) | ((uint32_t(y) ^ (uint32_t(z) >> 8)) << 16)) | 0xE6D5258A
  };
  uint32_t n{hash};
  n ^= n >> 13;
  n ^= n << 17;
  n ^= n >> 5;
  hash = n;

  return n;
  #elif defined(XOROSHIRO128PLUS_RANDOM)
  //xoroshiro128+

  // seeds depending on x, y, and z that are nontrivial even if they're all 0
  static thread_local std::array<uint64_t,2> s{
     uint64_t(x) |
     (uint64_t(y) << 16) |
     (uint64_t(z) << 32) |
     (uint64_t(x ^ y) << 48) |
     uint64_t(0xBD6CB273039BC9C0),

     uint64_t(y^z) |
     (uint64_t(x) << 16) |
     (uint64_t(y) << 32) |
     (uint64_t(z) << 48) |
     uint64_t(0xC7B38377DB5B4910)
  };

  const uint64_t s0{s[0]};
  uint64_t s1{s[1]};
  const uint32_t res{uint32_t((s0 + s1) >> 32)};

  // update seed
  s1 ^= s0;
  s[0] = xoroshiro_rotl(s0, 24) ^ s1 ^ (s1 << 16); // a, b
  s[1] = xoroshiro_rotl(s1, 37); // c

  return res;
  #elif defined(STD_RNG)
  static thread_local std::mt19937_64 generator(std::clock()
    + std::hash<std::thread::id>()(std::this_thread::get_id()));
  std::uniform_int_distribution<uint32_t> distribution(0,std::numeric_limits<uint32_t>::max());
  return distribution(generator);
  #endif
}

uint32_t random_uint32_t(uint32_t range)
{
  // unbiased fast ranged rng, due to D. Lemire and M.E. O'Neill
  // (see https://www.pcg-random.org/posts/bounded-rands.html)
  uint32_t n{random_uint32_t()};
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
        n = random_uint32_t();
        m = uint64_t(n) * uint64_t(range);
        l = uint32_t(m);
    }
  }
  return m >> 32;
}

float random_float()
{
  #if defined(XORSHIFT_RANDOM) || defined(XOROSHIRO128PLUS_RANDOM)
  uint32_t n{random_uint32_t(x,y,z)};
  // convert the uint32 into a float:
  // mask out the 23 bits of the mantissa, and fix the first 9 bits to make it positive and
  // concentrated in [1,2) (set the exponent to 127)
  std::bitset<32> float_bits{(n & 0x007FFFFF) | 0x3F800000};
  float res;
  std::memcpy(&res,&float_bits,32);

  // shift the result to make it concentrated in [0,1)
  return res - 1.0f;
  #elif defined(STD_RNG)
  static thread_local std::mt19937_64 generator(std::clock()
    + std::hash<std::thread::id>()(std::this_thread::get_id()));
  static std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
  return distribution(generator);
  #endif
}

std::array<float,2> random_float_pair()
{
  #ifdef PMJ02_RANDOM_PAIRS
  static thread_local std::unordered_map<uint64_t,size_t> indices;
  static thread_local const uint16_t
    shuffle_filter{uint16_t(random_uint32_t(x,y,z,SIZE_RNG_SAMPLES-1) >> 16)};
  // scramble_filter, to scramble the (mantissa bits of) the sample
  // see Fredel--Keller, Fast Generation of Randomized Low-Discrepancy Point Sets
  // and Kollig--Keller, Efficient Multidimensional Sampling
  static thread_local std::unordered_map<uint64_t,uint32_t> scramble_filters;
  uint64_t hash{uint64_t(x) | (uint64_t(y) << 16) | (uint64_t(z) << 32)};

  if (indices.find(hash) == indices.end())
  {
    indices[hash] = 0;
    scramble_filters[hash] = random_uint32_t(x,y,z) >> 9;
  }
  else
    indices[hash] += 1;

  uint16_t sample{uint16_t(hash % N_RNG_SAMPLES)};

  uint16_t offset{uint16_t(indices[hash] / SIZE_RNG_SAMPLES)};

  if (offset > 0)
  {
    sample += offset;
    sample %= N_RNG_SAMPLES;
  }

  float res0{(*samples_2d[sample])[(indices[hash] % SIZE_RNG_SAMPLES) ^ shuffle_filter][0]};
  float res1{(*samples_2d[sample])[(indices[hash] % SIZE_RNG_SAMPLES) ^ shuffle_filter][1]};

  uint32_t temp0{reinterpret_cast<uint32_t&>(res0) ^ scramble_filters[hash]};
  uint32_t temp1{reinterpret_cast<uint32_t&>(res1) ^ scramble_filters[hash]};

  std::memcpy(&res0,&temp0,32);
  std::memcpy(&res1,&temp1,32);

  return std::array<float,2>{res0,res1};
  #else
  return std::array<float,2>{random_float(), random_float()};
  #endif
}

float random_float(float min, float max)
{
  #if defined(XORSHIFT_RANDOM) || defined(XOROSHIRO128PLUS_RANDOM)
  return min + (max - min) * random_float(x,y,z);
  #elif defined(STD_RNG)
  static thread_local std::mt19937_64 generator(std::clock()
    + std::hash<std::thread::id>()(std::this_thread::get_id()));
  std::uniform_real_distribution<float> distribution(min, max);
  return distribution(generator);
  #endif
}
*/