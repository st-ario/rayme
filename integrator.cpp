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
#define NO_RR 1 // TODO fix Russian Roulette, currently buggy

#ifndef NO_RR
static constexpr uint16_t MAX_DEPTH{1000};
#else
static constexpr uint16_t MAX_DEPTH{10};
#endif

color direct_light( const point& x
                  , const ray& r_incoming
                  , const hit_record& record
                  , const composite_brdf& b
                  , const normed_vec3& gnormal
                  , const normed_vec3& snormal
                  , const bvh_tree& world
                  , size_t L)
{
  color res{0.0f,0.0f,0.0f};

  // Multiple importance sampling: BRDF and light source sampling
  // Power heuristcs: beta = 2
  float brdf_weight{0.0f};
  float light_weight{0.0f};
  float light_pdf{1.0f / world_lights::lights()[L]->get_surface_area()};

  // get scatter direction for BRDF sampliing
  normed_vec3 scatter_dir{b.sample_dir(-r_incoming.direction)};
  float brdf_pdf{b.pdf(-r_incoming.direction,scatter_dir)};

  // if the pdf of the BRDF evaluates to 0, skip the BRDF sampling at once
  if (brdf_pdf == 0.0f)
    goto source_sampling;

  #ifdef NO_BRDFDLS
  goto source_sampling;
  #endif

  // BRDF sampling
  {
    // update sampling weight
    #ifndef NO_SDLS
    brdf_weight = brdf_pdf * brdf_pdf / (brdf_pdf * brdf_pdf + light_pdf * light_pdf);
    #else
    brdf_weight = 1.0f;
    #endif

    ray scattered{bounce_ray(x,record.p_error(),gnormal,scatter_dir)};

    auto rec_shadow{world.hit(scattered,infinity)};
    if (!rec_shadow)
      goto source_sampling;

    // check whether the scattered ray interacts or not with the current light
    if (rec_shadow->what()->parent_mesh != world_lights::lights()[L].get())
      goto source_sampling;

    // if the scattered ray hit the current light, compute its contribution
    vec3 emit{world_lights::lights()[L]->ptr_mat->emissive_factor};

    res += brdf_weight * emit * b.estimator(-r_incoming.direction,scatter_dir);
  } // BRDF sampling

source_sampling:
  // light source sampling
  #ifdef NO_SDLS
  return res;
  #endif
  {
    auto target_pair{world_lights::lights()[L]->random_surface_point()};

    vec3 nonunital_shadow_dir{target_pair.first - x};
    normed_vec3 shadow_dir{unit(nonunital_shadow_dir)};

    // important: to evaluate whether or not the point is illuminated use the geometric normal
    float cos_angle{dot(gnormal, shadow_dir)};
    if (cos_angle < machine_two_epsilon)
      return res;

    // use the shading normal for the shading computations
    cos_angle = dot(snormal,shadow_dir);

    ray shadow{offset_ray_origin(x,record.p_error(),gnormal,shadow_dir),shadow_dir};

    auto rec_shadow = world.hit(shadow, infinity);

    if (!rec_shadow)
      return res;

    auto info_shadow{rec_shadow->what()->get_info(shadow,rec_shadow->uvw)};

    if (rec_shadow->what() != target_pair.second)
        return res;

    // TODO check for geometric/shadowing normal artifacts
    float cos_light_angle{dot(info_shadow.snormal(), -shadow.direction)};
    float projection_factor{cos_light_angle / glm::length2(nonunital_shadow_dir)};
    if (projection_factor < machine_two_epsilon)
      return res;

    vec3 emit{world_lights::lights()[L]->ptr_mat->emissive_factor};

    if (brdf_weight == 0.0f)
      light_weight = 1.0f;
    else {
      light_weight = light_pdf * light_pdf / (brdf_pdf * brdf_pdf + light_pdf * light_pdf);
    }

    // the role of wo and wi is inverted since we are following the shadow ray
    // (relevant for those brdfs that break symmetry)
    color f_r{b.f_r(shadow_dir,-r_incoming.direction)};

    res += light_weight * emit * projection_factor * f_r * cos_angle / light_pdf;
  } // light source sampling

  return res;
}

color integr( const ray& r
            , const bvh_tree& world
            , uint16_t depth
            , color& throughput
            , uint64_t seed)
{
  #ifndef NO_INDIRECT
  #ifndef NO_RR
  float thr{1.0f};
  // Russian roulette
  if (depth > uint16_t(4))
  {
    thr = std::max(throughput.x, std::max(throughput.y, throughput.z));
    float rand{random_float()};
    if (rand > thr || thr < machine_epsilon || depth > MAX_DEPTH)
      return {0.0f,0.0f,0.0f};

    // if the ray goes on, compensate for energy loss
    throughput *= 1.0f / thr;
  }
  #else
  if (depth > MAX_DEPTH)
    return color{0.0f};
  #endif
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
  normed_vec3 snormal{info.snormal()};
  composite_brdf b{info.ptr_mat(),&snormal,seed};
  seed = next_seed(seed);

  // sample direct light

  color direct{0.0f,0.0f,0.0f};
  #ifndef NO_DIRECT
  for (size_t i = 0; i < world_lights::lights().size(); ++i)
    direct += direct_light(hit_point,r,rec.value(),b,info.gnormal(),info.snormal(),world,i);

  #ifndef NO_RR
  direct *= 1.0f / thr; // compensate for Russian Roulette
  #endif
  #endif

  // sample indirect light

  #ifndef NO_INDIRECT

  // get scatter ray according to BRDF
  normed_vec3 scatter_dir{b.sample_dir(-r.direction)};
  ray scattered{bounce_ray(hit_point,rec->p_error(),info.gnormal(),scatter_dir)};

  // update throughput
  throughput *= b.estimator(-r.direction,scatter_dir);

  // get indirect light contribution
  color indirect{integr(scattered, world, depth+1, throughput,seed)};

  res += direct + (throughput * indirect);
  #else
  res += direct;
  #endif

  return res;
}

color integrator::integrate_path( const ray& r
                                , const bvh_tree& world)
{
  color res{0.0f,0.0f,0.0f};
  color throughput{1.0f,1.0f,1.0f};

  float seedf0{sampler.rnd_float()};
  float seedf1{sampler.rnd_float()};
  uint64_t seed;
  std::memcpy(&seed,&seedf0,4);
  std::memcpy(&seed+32,&seedf1,4);

  res += integr(r,world,0,throughput,seed);

  return res;
}