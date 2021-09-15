#include "integrator.h"
#include "materials.h"

color incoming_light(point p, const normed_vec3& normal, const element& world)
{
  color res{0.0f,0.0f,0.0f};

  normed_vec3 scattered = random_hemisphere_unit(normal);
  ray r{p,scattered};

  auto rec = world.hit(r,infinity);
  if (!rec)
    return {0,0,0};

  hit_record record = rec.value().first->get_record(r,rec.value().second);

  color emit{0.0f,0.0f,0.0f};

  if (record.ptr_mat->emitter)
    emit = record.ptr_mat->emissive_factor;

  res += static_cast<double>(pi * dot(normal,scattered)) * emit;

  return res;
}