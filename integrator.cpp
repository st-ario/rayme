#include <random>

#include "integrator.h"
#include "materials.h"
#include "meshes.h"
#include "bdf.h"
#include "extern/glm/glm/gtx/norm.hpp"

static constexpr uint16_t MAX_DEPTH{1000};

color direct_light(const point& x, const normed_vec3& normal, const element& world, size_t L)
{
  point target = world_lights::lights()[L]->random_surface_point();

  vec3 nonunital_shadow_dir{target - x};
  ray shadow{x,unit(nonunital_shadow_dir)};

  float cos_angle{dot(normal, shadow.direction)};

  if (cos_angle < machine_epsilon)
    return {0.0f,0.0f,0.0f};

  auto rec_shadow = world.hit(shadow, infinity);

  if (!rec_shadow)
    return {0.0f,0.0f,0.0f};

  auto record_shadow = rec_shadow.value().first->get_record(shadow,rec_shadow.value().second);

  vec3 emit{0.0f,0.0f,0.0f};
  vec3 diff{glm::abs(shadow.at(record_shadow.t) - target)};
  if (diff.x < machine_two_epsilon && diff.y < machine_two_epsilon && diff.z < machine_two_epsilon)
    emit = record_shadow.ptr_mat->emissive_factor;

  float cos_light_angle{dot(record_shadow.normal, -shadow.direction)};
  if (cos_light_angle < machine_epsilon)
    return {0.0f,0.0f,0.0f};

  float projection_factor = cos_light_angle / glm::length2(nonunital_shadow_dir);

  color res{emit * cos_angle * projection_factor * world_lights::lights()[L]->get_surface_area()};

  return res;
}

color integrator(const ray& r, const element& world, uint16_t depth, color& throughput)
{
  // assuming everything is Lambertian
  // TODO change when materials are properly dealt with

  // Russian roulette
  float thr{1.0f};
  if (depth > uint16_t(3))
  {
    thr = std::max(throughput.x, std::max(throughput.y, throughput.z));
    float rand{random_float()};
    if (rand > thr || thr < machine_epsilon || depth > MAX_DEPTH)
      return {0.0f,0.0f,0.0f};

    throughput *= 1.0f / thr;
  }

  auto rec = world.hit(r, infinity);
  if (!rec)
    return {0.0f,0.0f,0.0f};
  hit_record record = rec.value().first->get_record(r,rec.value().second);

  color res{0.0f,0.0f,0.0f};

  if (depth == 0 && record.ptr_mat->emitter)
    return record.ptr_mat->emissive_factor;

  diffuse_brdf brdf{record.ptr_mat};
  auto sample{brdf.sample(r.at(record.t),record.normal)};
  throughput *= sample.first;

  size_t light_index{random_size_t(0,world_lights::lights().size()-1)};
  color direct{direct_light(r.at(record.t),record.normal,world,light_index)};

  color indirect{integrator(sample.second, world, depth+1, throughput)};

  res += throughput * (direct + indirect);

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