#include "bdf.h"
#include "ray.h"
#include "bvh.h"

color diffuse_brdf::f_r( const normed_vec3& minus_wo
                       , const normed_vec3& wi) const
{
  #ifdef LAMBERTIAN_DIFFUSE
  return color{ptr_mat->base_color * invpi};
  #else
  float cos_wi{clamp(dot(*normal,wi),0.0f,1.0f)};
  float cos_wo{clamp(dot(-minus_wo,*normal),0.0f,1.0f)};
  float sin_wi{epsilon_clamp(std::sqrt(1.0f - cos_wi * cos_wi))};
  float sin_wo{epsilon_clamp(std::sqrt(1.0f - cos_wo * cos_wo))};
  float cos_difference{cos_wo * cos_wi + sin_wo * sin_wi};

  float sin_alpha;
  float tan_beta;
  if (cos_wo < cos_wi)
  {
    sin_alpha = sin_wo;
    tan_beta = sin_wi / (machine_two_epsilon + cos_wi);
  } else {
    sin_alpha = sin_wi;
    tan_beta = sin_wo / (machine_two_epsilon + cos_wo);
  }
  return color{ptr_mat->base_color * invpi
                * (A + B * std::max(0.0f,cos_difference) * sin_alpha * tan_beta)};
  #endif
}