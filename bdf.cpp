#include "bdf.h"
#include "ray.h"
#include "bvh.h"

constexpr float diffuse_pdf{1.0f/(2.0f * pi)};

float diffuse_brdf::pdf() { return diffuse_pdf; }

std::pair<color,ray> diffuse_brdf::sample(const point& at, const normed_vec3& normal)
{
  normed_vec3 scatter_dir = random_hemisphere_unit(normal);
  ray scattered{at,scatter_dir};

  return {dot(normal,scattered.direction) * eval() / pdf(),scattered};
}

color diffuse_brdf::eval()
{
  return ptr_mat->base_color / pi;
}