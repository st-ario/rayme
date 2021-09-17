#include "bdf.h"
#include "ray.h"
#include "bvh.h"

static constexpr float inverse_hemisphere_area{1.0f/two_pi};

float diffuse_brdf::pdf(const normed_vec3& normal, const normed_vec3& scattered)
{
  return dot(normal,scattered) * inverse_hemisphere_area;
}

std::pair<color,ray> diffuse_brdf::sample(const point& at, const normed_vec3& normal)
{
  normed_vec3 scatter_dir = cos_weighted_random_hemisphere_unit(normal);
  ray scattered{at,scatter_dir};

  return {dot(normal,scattered.direction) * eval() / pdf(normal,scattered.direction), scattered};
}

color diffuse_brdf::eval()
{
  return ptr_mat->base_color / pi;
}