#include <array>
#include <cstdint>

//constexpr uint16_t N_RNG_SAMPLES{401};
//constexpr uint16_t SIZE_RNG_SAMPLES{4096};

//extern std::array<const std::array<std::array<float,2>,SIZE_RNG_SAMPLES>*,N_RNG_SAMPLES> samples_2d;

class sampler_1d
{
  public:
    sampler_1d(uint64_t seed)
    : seed{seed} {}

    float rnd_float()
    {
      //xoroshiro128+

      static thread_local std::array<uint64_t,2> s{
        seed ^ uint64_t(0xBD6CB273039BC9C0),
        ((seed >> 32) | (seed << 32)) ^ uint64_t(0xC7B38377DB5B4910)
      };

      const uint64_t s0{s[0]};
      uint64_t s1{s[1]};
      const uint32_t res{uint32_t((s0 + s1) >> 32)};

      // update seed
      s1 ^= s0;
      s[0] = ((s0 << 24) | (s0 >> 40)) ^ s1 ^ (s1 << 16);
      s[1] = (s1 << 37) | (s1 >> 27);

      return res;
    }

    std::array<float,2> rnd_pair();

  private:
    uint64_t seed;
};