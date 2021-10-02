#include "bdf.h"
#include "ray.h"
#include "bvh.h"

float diffuse_brdf::pdf(const normed_vec3& normal, const normed_vec3& scattered)
{
  return dot(normal,scattered) / pi;
}

std::pair<color,normed_vec3> diffuse_brdf::sample(const point& at, const normed_vec3& normal)
{
  normed_vec3 scatter_dir = cos_weighted_random_hemisphere_unit(normal);

  return {dot(normal,scatter_dir) * f_r() / pdf(normal,scatter_dir), scatter_dir};
}

color diffuse_brdf::f_r()
{
  return ptr_mat->base_color / pi;
}