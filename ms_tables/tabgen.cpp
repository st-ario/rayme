#include "../bdf.h"
#include "../bdf.cpp"
#include "../math.cpp"

#include <iomanip>
#include <fenv.h>
#include <random>
#include <thread>

constexpr uint32_t n_samples{262144u};

void ggx_energy()
{
  material m{};
  normed_vec3 normal{normed_vec3::absolute_y()};

  for (uint i = 0; i < 64; ++i)
  {
    // roughness ranging from 0 to 1
    m.roughness_factor = static_cast<float>(i) / 63.0f;
    ggx_brdf ggx{&m,&normal};

    for (uint j = 0; j < 64; ++j)
    {
      // cosine ranging from 1/64 to 1
      float c{static_cast<float>(j+1) / 64.0f};
      normed_vec3 wo{unit(vec3(std::sqrt(1.0f - c * c), c, 0.0f))};

      double total{0.0f};
      for (uint32_t i = 0; i < n_samples; ++i)
      {
        vec3 loc_wo{ggx.to_local * wo.to_vec3()};
        normed_vec3 wi = ggx.sample_dir(wo,0,0,0);
        total += ggx.estimator(wo,wi).r;
      }
      total /= n_samples;

      std::cout.precision(9);
      std::cout << std::fixed << std::setfill('0')
        << "std::pair<std::array<float,2>,float>{std::array<float,2>{"
        << ggx.alpha << "f," << c << "f}," << static_cast<float>(total) << "f},\n";
    }
  }
}

void ggx_energy_average()
{
  material m{};
  normed_vec3 normal{normed_vec3::absolute_y()};

  for (uint i = 0; i < 64; ++i)
  {
    // roughness ranging from 0 to 1
    m.roughness_factor = static_cast<float>(i) / 63.0f;
    ggx_brdf ggx{&m,&normal};

    float total{0.0f};
    for (uint j = 0; j < 64; ++j)
    {
      // cosine ranging from 1/64 to 1
      float c{static_cast<float>(j+1)/64.0f};
      total += ms_lookup_E(std::array<float,2>{ggx.alpha,c},ggx_E) * c;
    }
    total /= 64;
    total *= 2;

    std::cout.precision(9);
    std::cout << std::fixed << std::setfill('0')
      << "std::array<float,2>{" << ggx.alpha << "f," << total << "f},\n";
  }
}

int main()
{
  feenableexcept(FE_INVALID | FE_OVERFLOW);

  ggx_energy();
  //ggx_energy_average();

  return 0;
}