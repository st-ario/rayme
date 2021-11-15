#include "../bdf.h"
#include "../bdf.cpp"
#include "../math.cpp"

#include <iomanip>
#include <fenv.h>
#include <random>
#include <thread>

void weak_white_furnace(uint N)
{
  constexpr uint32_t n_samples{4096u};
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

      float total{0.0f};
      uint32_t i = 0;
      while (i < n_samples)
      {
        ++tot;
        normed_vec3 wi = ggx.sample_dir(wo,0,0,0);
        vec3 loc_wi = ggx.to_local * wi.to_vec3();
        vec3 loc_wo = ggx.to_local * wo.to_vec3();
        vec3 loc_wh = glm::normalize(loc_wo + loc_wi);
        float pdf = ggx.pdf(wo,wi);

        if (pdf != 0.0f)
        {
		      total += ggx.G1(loc_wo,loc_wh) * ggx.D(loc_wh) / (4.0f * c * ggx.pdf(wo,wi));
          ++i;
        } else {
          ++rep;
        }
      }
      total /= n_samples;

      float diff = std::abs(1.0f - total);

      std::cerr.precision(9);
      if (diff > 0.2f)
      {
        std::cerr << std::fixed << std::setfill('0')
          << "alpha: " << ggx.alpha << " mu_o: " << c << " integral: " << total << "\n";
      }
    }
  }
  std::cerr << rep << "\n";
  std::cerr << tot << "\n";
  std::cerr << static_cast<float>(rep)/ static_cast<float>(tot)<< "\n";
}

int main()
{
  feenableexcept(FE_INVALID | FE_OVERFLOW);

  weak_white_furnace(128);

  return 0;
}