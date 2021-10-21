#include <array>
#include <cstdint>

constexpr uint16_t N_RNG_SAMPLES{401};
constexpr uint16_t SIZE_RNG_SAMPLES{4096};

extern std::array<const std::array<std::array<float,2>,4096>*,401> samples_2d;
