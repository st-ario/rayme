#include "my_vectors.h"

vec3 vec3::random()
{
  return vec3(random_float(), random_float(), random_float());
}

vec3 vec3::random(float min, float max)
{
  return vec3(random_float(min,max), random_float(min,max), random_float(min,max));
}

normed_vec3 normed_vec3::random_unit()
// computed normalizing standard Gaussians for each coordinate to get the uniform distribution on the surface
{
  float rx = standard_normal_random_float();
  float ry = standard_normal_random_float();
  float rz = standard_normal_random_float();
  float inverse_norm = fast_inverse_sqrt(rx*rx + ry*ry + rz*rz);
  return normed_vec3(rx*inverse_norm,ry*inverse_norm,rz*inverse_norm);
}

vec3 vec3::random_in_unit_sphere()
{
  float random_radius = random_float();
  return random_radius * normed_vec3::random_unit().to_vec3();
}

// normed_vec3 utility functions
normed_vec3 reflect(const normed_vec3& incident, const normed_vec3& normal)
{
  vec3 i = incident.to_vec3();
  vec3 n = normal.to_vec3();

  return unit(i - 2 * dot(i,n) * n);
}

normed_vec3 refract(const normed_vec3& incident, const normed_vec3& normal, float refractive_indices_ratio)
{
  vec3 i = incident.to_vec3();
  vec3 n = normal.to_vec3();

  float cos_incidence_angle = dot(-incident, normal);
  vec3 refracted_perp = refractive_indices_ratio * (i + cos_incidence_angle * n);
  vec3 refracted_parallel = - std::sqrt(1.0f - refracted_perp.norm_squared()) * n;
  return unit(refracted_perp + refracted_parallel);
}