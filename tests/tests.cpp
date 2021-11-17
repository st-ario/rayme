#include "../bdf.h"
#include "../bdf.cpp"
#include "../math.cpp"

#include <iomanip>
#include <fenv.h>
#include <random>
#include <thread>

constexpr uint32_t n_samples{4096u};

void weak_white_furnace(uint N)
{
  material m{};
  normed_vec3 normal{normed_vec3::absolute_y()};

  uint32_t rep = 0;
  uint32_t tot = 0;
  for (uint i = 0; i < N; ++i)
  {
    m.roughness_factor = static_cast<float>(i+1) / static_cast<float>(N);
    ggx_brdf ggx{&m,&normal};

    for (uint j = 0; j < N; ++j)
    {
      float c{static_cast<float>(j+1) / static_cast<float>(N)};
      normed_vec3 wo{unit(vec3(std::sqrt(1.0f - c * c), c, 0.0f))};

      double total{0.0};
      uint32_t k = 0;
      while (k < n_samples)
      {
        ++tot;
        normed_vec3 wi = ggx.sample_dir(wo,0,0,0);
        vec3 loc_wi = ggx.to_local * wi.to_vec3();
        vec3 loc_wo = ggx.to_local * wo.to_vec3();
        vec3 loc_wh = glm::normalize(loc_wo + loc_wi);
        float pdf = ggx.pdf(wo,wi);

        if (pdf != 0.0f)
        {
		      total += ggx.G1(loc_wo,loc_wh) * ggx.D(loc_wh) / (4.0f * c * pdf);
          ++k;
        } else {
          ++rep;
        }
      }
      total /= n_samples;

      double diff{std::abs(1.0 - total)};

      std::cerr.precision(9);
      if (diff > 0.2)
      {
        std::cerr << std::fixed << std::setfill('0')
          << "alpha: " << ggx.alpha << " mu_o: " << c << " integral: " << static_cast<float>(total) << "\n";
      }
    }
  }
  std::cerr << rep << "\n";
  std::cerr << tot << "\n";
  std::cerr << static_cast<float>(rep)/ static_cast<float>(tot)<< "\n";
}

void ms_energy_conservation(uint N)
{
  material m{};
  normed_vec3 normal{normed_vec3::absolute_y()};

  for (uint i = 0; i < N; ++i)
  {
    m.roughness_factor = static_cast<float>(i) / static_cast<float>(N-1);
    ggx_brdf ggx{&m,&normal};

    double total{0.0};
    for (uint32_t j = 0; j < n_samples; ++j)
    {
      normed_vec3 wo{cos_weighted_random_upper_hemisphere_unit(0,0,0)};
      vec3 loc_wo{ggx.to_local * wo.to_vec3()};
      double subtotal{0.0};
      double s_total{0.0};
      for (uint32_t j = 0; j < n_samples; ++j)
      {
        normed_vec3 wi = ggx.sample_dir(wo,0,0,0);
        s_total += ggx.estimator(wo,wi).r;
      }
      s_total /= n_samples;
      double d_total{0.0};
      for (uint32_t j = 0; j < n_samples; ++j)
      {
        normed_vec3 wi{cos_weighted_random_upper_hemisphere_unit(0,0,0)};
        d_total += ggx.f_ms(wo,wi,ggx_E,ggx_Eavg);
      }
      d_total *= M_PI / n_samples;

      subtotal = d_total + s_total;
      total += subtotal;
    }

    total /= n_samples;

    std::cerr.precision(9);
    std::cerr << std::fixed << std::setfill('0')
      << "alpha: " << ggx.alpha << " integral: " << static_cast<float>(total) << "\n";

  }
}

int main()
{
  feenableexcept(FE_INVALID | FE_OVERFLOW);

  //weak_white_furnace(128);

  ms_energy_conservation(32);

  return 0;
}