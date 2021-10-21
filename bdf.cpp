#include "bdf.h"
#include "ray.h"
#include "bvh.h"

brdf_sample diffuse_brdf::sample( const point& at
                                , const normed_vec3& gnormal
                                , const normed_vec3& snormal
                                , uint16_t pixel_x
                                , uint16_t pixel_y
                                , uint16_t seed) const
{
  normed_vec3 scatter_dir{cos_weighted_random_hemisphere_unit(gnormal, pixel_x, pixel_y, seed)};
  float cos_angle{dot(snormal,scatter_dir)};
  float pdf{cos_angle / pi};
  color f_r{ptr_mat->base_color / pi};

  return brdf_sample(pdf,f_r,scatter_dir,cos_angle);
}