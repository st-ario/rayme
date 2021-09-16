#include "integrator.h"
#include "materials.h"
#include "bdf.h"

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

  diffuse_brdf brdf{record.ptr_mat};

  auto sample{brdf.sample(r.at(record.t),record.normal)};

  color incoming{0.0,0.0,0.0};
  incoming += sample.first * integrator(sample.second,world,depth+1);

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