#include <random>

#include "integrator.h"
#include "materials.h"
#include "meshes.h"
#include "bdf.h"
#include "extern/glm/glm/gtx/norm.hpp"

static constexpr uint16_t MAX_DEPTH{1000};

color integrator(const ray& r, const element& world, uint16_t depth, color& throughput)
{
  // assuming everything is Lambertian
  // TODO change when materials are properly dealt with
  color res{0.0f,0.0f,0.0f};

  auto rec = world.hit(r, infinity);

  if (!rec)
    return res;

  hit_record record = rec.value().first->get_record(r,rec.value().second);

  if (record.ptr_mat->emitter)
    res += record.ptr_mat->emissive_factor;

  diffuse_brdf brdf{record.ptr_mat};

  auto sample{brdf.sample(r.at(record.t),record.normal)};

  throughput *= sample.first;

  float thr{1.0f};
  if (depth > uint16_t(3))
  {
    thr = std::max(throughput.x, std::max(throughput.y, throughput.z));
    float rand{random_float()};
    if (rand > thr || depth > MAX_DEPTH)
      return res;

    throughput /= thr;
  }

  color incoming{0.0,0.0,0.0};
  incoming += sample.first * integrator(sample.second, world, depth+1, throughput) / thr;

  res += incoming;

  return res;
}

color sample_bdf(const ray& r, const element& world, uint16_t integration_samples_N, uint16_t depth)
{
  color res{0.0f,0.0f,0.0f};
  color throughput{1.0f,1.0f,1.0f};

  for (uint16_t i = 0; i < integration_samples_N; ++i)
  {
    throughput = color{1.0f,1.0f,1.0f};
    res += integrator(r,world,depth,throughput);
  }

  res /= integration_samples_N;
  return res;
}

color direct_light(const ray& r, const element& world, uint16_t depth, uint16_t L)
{
  color res{0.0,0.0,0.0};

  auto rec = world.hit(r, infinity);

  if (!rec)
    return res;

  hit_record record = rec.value().first->get_record(r,rec.value().second);

  if (record.ptr_mat->emitter)
  {
    res += record.ptr_mat->emissive_factor;
    return res;
  }

  point target = world_lights::lights()[L]->random_surface_point();

  vec3 nonunital_shadow_dir{target - r.at(record.t)};
  ray shadow{r.at(record.t),unit(nonunital_shadow_dir)};

  auto rec_shadow = world.hit(shadow, infinity);
  auto record_shadow = rec_shadow.value().first->get_record(shadow,rec_shadow.value().second);

  float v{0.0f};

  vec3 diff{glm::abs(shadow.at(record_shadow.t) - target)};
  if (diff.x < machine_two_epsilon && diff.y < machine_two_epsilon && diff.z < machine_two_epsilon)
    v = 1.0f;

  vec3 emit = v * record_shadow.ptr_mat->emissive_factor;

  float projection_factor = - dot(record_shadow.normal,shadow.direction) / glm::length2(nonunital_shadow_dir);

  res += record.ptr_mat->base_color/pi * emit
       * dot(record.normal, shadow.direction) * projection_factor
       * world_lights::lights()[L]->get_surface_area();

  return res;
}

color sample_lights(const ray& r, const element& world, uint16_t integration_samples_N, uint16_t depth)
{
  color res{0.0,0.0,0.0};
  uint16_t rand{0};

  for (uint16_t i = 0; i < integration_samples_N; ++i)
  {
    rand = random_uint16_t(0,world_lights::lights().size()-1);
    res += direct_light(r,world,depth,rand);
  }

  res /= integration_samples_N;

  return res;
}