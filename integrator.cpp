#include "integrator.h"
#include "materials.h"

color integrator(const ray& r, const element& world, uint16_t depth)
{
  // assuming everything is Lambertian
  // TODO change when materials are properly dealt with
  color res{0.0,0.0,0.0};

  if (depth > uint16_t(5))
    return res;

  auto rec = world.hit(r, infinity);

  if (!rec)
    return res;

  hit_record record = rec.value().first->get_record(r,rec.value().second);

  if (record.ptr_mat->emitter)
    res += record.ptr_mat->emissive_factor;

  normed_vec3 scatter_dir = random_hemisphere_unit(record.normal);
  ray scattered{r.at(record.t),scatter_dir};

  color incoming{0.0,0.0,0.0};
  incoming += record.ptr_mat->base_color
    * integrator(scattered,world,depth+1)
    * static_cast<double>(dot(record.normal,scatter_dir * pi));

  res += incoming;

  return res;
}

color incoming_light(const ray& r, const element& world, uint16_t integration_samples_N, uint16_t depth)
{
  color res{0.0,0.0,0.0};

  for (uint16_t i = 0; i < integration_samples_N; ++i)
    res += integrator(r,world,depth);

  res /= integration_samples_N;
  return res;
}