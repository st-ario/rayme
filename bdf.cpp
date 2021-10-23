#include "bdf.h"
#include "ray.h"
#include "bvh.h"

#ifdef LAMBERTIAN_DIFFUSE
brdf_sample diffuse_brdf::sample( const point& at
#else
brdf_sample diffuse_brdf::sample( const ray& r_incoming
#endif
                                , const normed_vec3& gnormal
                                , const normed_vec3& snormal
                                , uint16_t pixel_x
                                , uint16_t pixel_y
                                , uint16_t seed) const
{
  normed_vec3 scatter_dir{cos_weighted_random_hemisphere_unit(gnormal, pixel_x, pixel_y, seed)};
  #ifdef LAMBERTIAN_DIFFUSE
  float cos_angle{dot(snormal,scatter_dir)};
  float pdf{cos_angle / pi};
  color f_r{ptr_mat->base_color / pi};

  return brdf_sample(pdf,f_r,scatter_dir,cos_angle);
  #else
  float cos_outgoing{epsilon_clamp(dot(snormal,scatter_dir))};
  float cos_incoming{epsilon_clamp(dot(-r_incoming.direction,snormal))};
  float sin_outgoing{epsilon_clamp(std::sqrt(1.0f - cos_outgoing * cos_outgoing))};
  float sin_incoming{epsilon_clamp(std::sqrt(1.0f - cos_incoming * cos_incoming))};
  float cos_difference{cos_incoming * cos_outgoing + sin_incoming * sin_outgoing};
  float pdf{cos_outgoing / pi};

  float sin_alpha;
  float tan_beta;
  if (cos_incoming < cos_outgoing)
  {
    sin_alpha = sin_incoming;
    tan_beta = sin_outgoing / cos_outgoing;
  } else {
    sin_alpha = sin_outgoing;
    tan_beta = sin_incoming / cos_incoming;
  }
  color f_r{ptr_mat->base_color / pi * (A + B * std::max(0.0f,cos_difference)
    * sin_alpha * tan_beta)};

  return brdf_sample(pdf,f_r,scatter_dir,cos_outgoing);
  #endif
}