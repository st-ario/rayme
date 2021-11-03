#include <random>

#include "integrator.h"
#include "materials.h"
#include "meshes.h"
#include "bdf.h"
#include "extern/glm/glm/gtx/norm.hpp"

// debug macros
//#define NO_DIRECT 1
//#define NO_INDIRECT 1
//#define NO_SDLS 1
//#define NO_BRDFDLS 1

static constexpr uint16_t MAX_DEPTH{1000};

color direct_light( const point& x
                  , const ray& r_incoming
                  , const hit_record& record
                  , const diffuse_brdf& brdf
                  , const normed_vec3& gnormal
                  , const normed_vec3& snormal
                  , const element& world
                  , size_t L
                  , uint16_t pixel_x
                  , uint16_t pixel_y
                  , uint16_t rng_offset)
{
  color res{0.0f,0.0f,0.0f};

  // Multiple importance sampling: BRDF and light source sampling
  // Power heuristcs: beta = 2
  float brdf_weight{0.0f};
  float light_weight{0.0f};
  float light_pdf{1.0f / world_lights::lights()[L]->get_surface_area()};

  auto sample{brdf.sample(r_incoming,gnormal,snormal,pixel_x,pixel_y,rng_offset)};

  // if the pdf of the BRDF evaluates to 0, skip the BRDF sampling at once
  if (sample.pdf == 0.0f)
    goto source_sampling;

  #ifdef NO_BRDFDLS
  goto source_sampling;
  #endif

  // BRDF sampling
  {
    // update sampling weight
    #ifndef NO_SDLS
    brdf_weight = sample.pdf * sample.pdf / (sample.pdf * sample.pdf + light_pdf * light_pdf);
    #else
    brdf_weight = 1.0f;
    #endif

    ray scattered{bounce_ray(x,record.p_error(),gnormal,sample.scatter_dir)};

    auto rec_shadow{world.hit(scattered,infinity)};
    if (!rec_shadow)
      goto source_sampling;

    // check whether the scattered ray interacts or not with the current light
    if (rec_shadow->what()->parent_mesh != world_lights::lights()[L].get())
      goto source_sampling;

    // if the scattered ray hit the current light, compute its contribution
    vec3 emit{world_lights::lights()[L]->ptr_mat->emissive_factor};
    res += brdf_weight * sample.f_r * emit * dot(sample.scatter_dir,snormal) / sample.pdf;

  } // BRDF sampling

source_sampling:
  // light source sampling
  #ifdef NO_SDLS
  return res;
  #endif
  {
    auto target_pair{world_lights::lights()[L]->random_surface_point(pixel_x, pixel_y, rng_offset)};

    vec3 nonunital_shadow_dir{target_pair.first - x};
    normed_vec3 shadow_dir{unit(nonunital_shadow_dir)};
    float cos_angle{dot(snormal, shadow_dir)};
    if (cos_angle < machine_two_epsilon)
      return res;

    ray shadow{offset_ray_origin(x,record.p_error(),gnormal,shadow_dir),shadow_dir};

    auto rec_shadow = world.hit(shadow, infinity);

    if (!rec_shadow)
      return res;

    auto info_shadow{rec_shadow->what()->get_info(shadow,rec_shadow->uvw)};

    if (rec_shadow->what() != target_pair.second)
        return res;

    float cos_light_angle{dot(info_shadow.gnormal(), -shadow.direction)};
    float projection_factor{cos_light_angle / glm::length2(nonunital_shadow_dir)};
    if (projection_factor < machine_two_epsilon)
      return res;

    vec3 emit{world_lights::lights()[L]->ptr_mat->emissive_factor};

    if (brdf_weight == 0.0f)
      light_weight = 1.0f;
    else {
      light_weight = light_pdf * light_pdf / (sample.pdf * sample.pdf + light_pdf * light_pdf);
    }

    res += light_weight * sample.f_r * emit * cos_angle * projection_factor / light_pdf;
  } // light source sampling

  return res;
}

color integrator( const ray& r
                , const element& world
                , uint16_t depth
                , color& throughput
                , uint16_t pixel_x
                , uint16_t pixel_y
                , uint16_t sample_id)
{
  // assuming everything is Diffuse
  // TODO change when materials are properly dealt with

  float thr{1.0f};
  #ifndef NO_INDIRECT
  // Russian roulette
  if (depth > uint16_t(3))
  {
    thr = std::max(throughput.x, std::max(throughput.y, throughput.z));
    float rand{random_float(pixel_x, pixel_y, sample_id + depth)};
    if (rand > thr || thr < machine_epsilon || depth > MAX_DEPTH)
      return {0.0f,0.0f,0.0f};

    // if the ray goes on, compensate for energy loss
    throughput *= 1.0f / thr;
  }
  #endif

  auto rec{world.hit(r, infinity)};
  if (!rec)
    return {0.0f,0.0f,0.0f};

  hit_properties info{rec->what()->get_info(r,rec->uvw)};
  point hit_point{r.at(rec->t())};
  color res{0.0f,0.0f,0.0f};

  #ifndef NO_DIRECT
  if (depth == 0 && info.ptr_mat()->emitter)
  #else
  if (info.ptr_mat()->emitter)
  #endif
    res += info.ptr_mat()->emissive_factor;

  // pick the BRDF for the surface hit
  diffuse_brdf brdf{info.ptr_mat()};

  // sample direct light

  color direct{0.0f,0.0f,0.0f};
  #ifndef NO_DIRECT
  static constexpr uint16_t seed_offset{3}; // avoids using the same sample pool for direct and
                                            // indirect light sampling
  for (size_t i = 0; i < world_lights::lights().size(); ++i)
    direct += direct_light(hit_point,r,rec.value(),brdf,info.gnormal(),info.snormal(),world,i,
      pixel_x, pixel_y, sample_id + depth + seed_offset);

  direct *= 1.0f / thr; // compensate for Russian Roulette
  #endif

  // sample indirect light

  #ifndef NO_INDIRECT
  // get scatter ray according to BRDF
  auto sample{brdf.sample(r,info.gnormal(),info.snormal(),pixel_x,pixel_y,sample_id+depth)};
  ray scattered{bounce_ray(hit_point,rec->p_error(),info.gnormal(),sample.scatter_dir)};

  // update throughput
  throughput *= dot(scattered.direction,info.snormal()) * sample.f_r / sample.pdf;

  // get indirect light contribution
  color indirect{integrator(scattered, world, depth+1, throughput, pixel_x, pixel_y, sample_id)};

  res += direct + (throughput * indirect);
  #else
  res += direct;
  #endif

  return res;
}

color integrate_path( const ray& r
                    , const element& world
                    , uint16_t depth
                    , uint16_t pixel_x
                    , uint16_t pixel_y
                    , uint16_t sample_id)
{
  color res{0.0f,0.0f,0.0f};
  color throughput{1.0f,1.0f,1.0f};

  res += integrator(r,world,depth,throughput,pixel_x,pixel_y,sample_id);

  return res;
}