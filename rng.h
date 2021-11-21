#pragma once

#include <array>
#include <cstdint>
#include <random>

// debug macros

// if not defined, rnd_float_pair() generates unstratified pairs
#define PMJ02_RANDOM_PAIRS 1

// replaces all rngs by the one used by random_float()
// in particular, rnd_float_pair() generates unstratified pairs
//#define STD_RNG 1

// uses the STL rng; slower than sampler_1d::rnd_float(), to be used only if there's
// no reasonable way to use that class in a thread-safe seed-based way
float random_float();

// unstratified, to be used for debugging and profiling purposes
// or if there's no reasonable way to use a sampler correctly
inline std::array<float,2> random_float_pair()
{ return std::array<float,2>{random_float(), random_float()}; }

constexpr uint16_t N_RNG_SAMPLES{401};
constexpr uint16_t SIZE_RNG_SAMPLES{4096};

extern std::array<const std::array<std::array<float,2>,SIZE_RNG_SAMPLES>*,N_RNG_SAMPLES> samples_2d;

class sampler_1d
{
  friend class sampler_2d;
  public:
    sampler_1d() = default;
    sampler_1d(uint64_t seed)
    : seed{seed} {}

    float rnd_float();
    uint32_t rnd_uint32();

  private:
    uint64_t seed;
    #ifndef STD_RNG
    uint32_t rnd_uint32(uint32_t range);
    #endif
};

class sampler_2d
{
  public:
    std::array<float,2> rnd_float_pair();
    sampler_2d(uint32_t seed)
    : seed{seed}
    , rng_1d{uint64_t(seed ^ uint32_t(0x44117E89)) << 32 | uint64_t(seed ^ uint32_t(0xBCB44618))}
    {}

  private:
    uint32_t seed;
    sampler_1d rng_1d;
    uint16_t offset{0u};
    uint32_t index{0u};
};

uint64_t next_seed(uint64_t old_seed);