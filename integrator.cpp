#include <random>

#include "integrator.h"
#include "materials.h"
#include "meshes.h"
#include "bdf.h"
#include "extern/glm/glm/gtx/norm.hpp"

static constexpr uint16_t MAX_DEPTH{1000};

color direct_light( const point& x
                  , const hit_record& record
                  , const hit_properties& info
                  , const element& world
                  , size_t L)
{
  auto target_pair{world_lights::lights()[L]->random_surface_point()};

  vec3 nonunital_shadow_dir{target_pair.first - x};
  normed_vec3 shadow_dir{unit(nonunital_shadow_dir)};
  float cos_angle{dot(info.normal(), shadow_dir)};
  if (cos_angle < machine_two_epsilon)
    return {0.0f,0.0f,0.0f};

  ray shadow{offset_ray_origin(x,record.p_error(),info.normal(),shadow_dir),shadow_dir};

  auto rec_shadow = world.hit(shadow, infinity);

  if (!rec_shadow)
    return {0.0f,0.0f,0.0f};

  auto info_shadow{rec_shadow->what()->get_info(shadow)};

  if (rec_shadow->what() != target_pair.second.get())
      return{0.0f,0.0f,0.0f};

  float cos_light_angle{dot(info_shadow.normal(), -shadow.direction)};
  float projection_factor{cos_light_angle / glm::length2(nonunital_shadow_dir)};
  if (projection_factor < machine_two_epsilon)
    return {0.0f,0.0f,0.0f};

  vec3 emit{world_lights::lights()[L]->ptr_mat->emissive_factor};
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

    // if the ray goes on, compensate for energy loss
    throughput *= 1.0f / thr;
  }

  auto rec = world.hit(r, infinity);
  if (!rec)
    return {0.0f,0.0f,0.0f};

  hit_properties info{rec->what()->get_info(r)};
  point hit_point{r.at(rec->t())};
  color res{0.0f,0.0f,0.0f};

  if (depth == 0 && info.ptr_mat()->emitter)
    return info.ptr_mat()->emissive_factor;

  // sample brdf
  diffuse_brdf brdf{info.ptr_mat()};
  auto sample{brdf.sample(hit_point,info.normal())};

  // sample direct light
  color direct{0.0f,0.0f,0.0f};
  for (size_t i = 0; i < world_lights::lights().size(); ++i)
    direct += direct_light(hit_point,rec.value(),info,world,i);

  direct /= world_lights::lights().size();
  direct *= sample.f_r / thr; // the denominator compensates for Russian Roulette

  // update throughput
  throughput *= sample.cos_angle * sample.f_r / sample.pdf;

  // sample indirect light
  ray scattered{bounce_ray(hit_point,rec->p_error(),info.normal(),sample.scatter_dir)};
  color indirect{integrator(scattered, world, depth+1, throughput)};

  res += direct + (throughput * indirect);

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