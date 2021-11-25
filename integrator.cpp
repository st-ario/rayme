#include <random>

#include "integrator.h"
#include "materials.h"
#include "meshes.h"
#include "bdf.h"
#include "extern/glm/glm/gtx/norm.hpp"

// debug macros
//#define NO_NEE 1
//#define NO_BRDFDLS 1
//#define NO_RR 1

#ifndef NO_RR
static constexpr uint16_t MIN_DEPTH{5};
static constexpr uint16_t MAX_DEPTH{1000};
#else
static constexpr uint16_t MAX_DEPTH{10};
#endif

color
integrator::sample_light( const point& x
                        , const normed_vec3& gnormal
                        , const normed_vec3& snormal
                        , const normed_vec3& incoming_dir
                        , const hit_record& record
                        , const bvh_tree& world
                        , const brdf& b) const
{
  // naive method for source sampling: select a random light uniformly
  // TODO improve

  uint32_t L{sampler.rnd_uint32(uint32_t(world_lights::lights().size()))};
  auto target_pair{world_lights::lights()[L]->random_surface_point()};

  vec3 nonunital_shadow_dir{target_pair.first - x};
  normed_vec3 shadow_dir{unit(nonunital_shadow_dir)};

  // important: to evaluate whether or not the point is illuminated use the geometric normal
  // to prevent light leaks
  float cos_angle{dot(gnormal, shadow_dir)};
  if (cos_angle < machine_two_epsilon)
    return color{0.0f};

  // use the shading normal for the shading computations
  cos_angle = max(0.0f, dot(snormal,shadow_dir));

  ray shadow{offset_ray_origin(x,record.p_error(),gnormal,shadow_dir),shadow_dir};

  auto rec_shadow = world.hit(shadow, infinity);

  // check whether the ray is occluded
  if (!rec_shadow || rec_shadow->what() != target_pair.second)
    return color{0.0f};

  auto info_shadow{rec_shadow->what()->get_info(shadow,rec_shadow->uvw)};

  float cos_light_angle{max(0.0f,dot(info_shadow.snormal(), -shadow_dir))};
  if (cos_light_angle == 0.0f)
    return color{0.0f};

  color emit{world_lights::lights()[L]->ptr_mat->emissive_factor};

  float light_area{world_lights::lights()[L]->get_surface_area()};
  float dist_squared{glm::length2(nonunital_shadow_dir)};

  if (dist_squared == 0)
    return color{0.0f};

  color brdf_estimator{b.estimator(-incoming_dir,shadow_dir)};
  float brdf_pdf{b.pdf(-incoming_dir,shadow_dir)};
  float nee_pdf{dist_squared / (world_lights::lights().size() * light_area * cos_light_angle)};
  color nee_contribution{ (emit * brdf_estimator)
    * (brdf_pdf * cos_light_angle * light_area * world_lights::lights().size()
    / dist_squared)};

  color brdf_contribution{brdf_pdf == 0 ? color{0.0f} : emit * brdf_estimator};

  // MIS, power heuristic
  float bpdf2{brdf_pdf * brdf_pdf};
  float npdf2{nee_pdf * nee_pdf};
  float normalize{1.0f / (bpdf2 + npdf2)};

  return color{normalize * (bpdf2 * brdf_contribution + npdf2 * nee_contribution)};
}

color integrator::integrate_path( ray& r
                                , const bvh_tree& world) const
{
  color res{0.0f,0.0f,0.0f};
  color throughput{1.0f,1.0f,1.0f};
  uint16_t depth{0u};

  uint64_t seed{uint64_t(sampler.rnd_uint32()) | uint64_t(sampler.rnd_uint32()) << 32};

  // data that needs to be stored between one cycle and the next
  // direct light contribution accumulated pre-bounce by light sampling
  color past_direct{0.0f};
  // integral estimator using brdf importance sampling
  color brdf_estimator{0.0f};
  // pdf for the brdf sampler
  float brdf_pdf{0.0f};
  // russian roulette probability
  float rr_p{1.0f};

  while (depth < MAX_DEPTH)
  {
    auto rec{world.hit(r, infinity)};
    if (!rec)
    {
      // eventual light at infinity info goes here: res += throughput * [skycolor]
      //res += throughput * color{0.5,0.5,0.7f};
      break;
    }

    hit_properties info{rec->what()->get_info(r,rec->uvw)};

    if (info.ptr_mat()->emitter)
    {
      if (depth == 0)
        res += info.ptr_mat()->emissive_factor;
      else if (brdf_pdf != 0.0f) // MIS only non-deterministic bounces
      {
        // MIS this light
        color brdf_contribution{info.ptr_mat()->emissive_factor * brdf_estimator};
        float dist_squared{rec->t() * rec->t()};
        auto light_hit{static_cast<const light*>(rec->what()->parent_mesh)};
        float light_area{light_hit->get_surface_area()};
        float cos_thetay{dot(-r.get_direction(),info.snormal())};
        float nee_pdf{dist_squared / (world_lights::lights().size() * light_area * cos_thetay)};

        float bpdf2{brdf_pdf * brdf_pdf};
        float npdf2{nee_pdf * nee_pdf};
        float normalize{1.0f / (bpdf2 + npdf2)};

        color nee_contribution{ (info.ptr_mat()->emissive_factor * brdf_estimator)
          * (brdf_pdf * cos_thetay * light_area * world_lights::lights().size() / dist_squared)};

        color future_direct{normalize * (bpdf2 * brdf_contribution + npdf2 * nee_contribution)};
        res += 0.5f * (throughput * (past_direct + future_direct));
      } else { // deterministic bounce
        // add contribution from this light
        color brdf_contribution{info.ptr_mat()->emissive_factor * brdf_estimator};
        res += throughput * brdf_contribution;
      }
    } else {
      res += throughput * past_direct;
    }

    // past and future are now synchronized
    // update throughput and compensate for russian roulette
    if (depth > 0)
        throughput *= brdf_estimator / rr_p;

    normed_vec3 snormal{info.snormal()};

    // prevent black spots: flip the shading normal if the brdf is undefined
    if (dot(snormal,-r.get_direction()) < 0)
      snormal = unit((2.0f * dot(info.gnormal(),info.snormal())) * info.gnormal().to_vec3() - info.snormal().to_vec3());

    // get hit BRDF
    composite_brdf b{info.ptr_mat(),&snormal,seed};

    // sample bounce direction using BRDF
    normed_vec3 scatter_dir{b.sample_dir(-r.get_direction())};
    brdf_pdf = b.pdf(-r.get_direction(),scatter_dir);

    point hit_point{r.at(rec->t())};

    // direct light contribution for non-deterministic bounces
      past_direct = (brdf_pdf == 0.0f) ? color{0.0}
        : sample_light(hit_point,info.gnormal(),info.snormal(),r.get_direction(),*rec,world,b);

    // sample integral estimator
    brdf_estimator = b.estimator(-r.get_direction(),scatter_dir);

    ++depth;
    if (depth == MAX_DEPTH)
    {
      res += throughput * past_direct;
      break;
    }

    #ifndef NO_RR
    // russian roulette
    if (depth > MIN_DEPTH)
    {
      rr_p = min(0.99f,max(throughput.x,max(throughput.y,throughput.z)));
      if (sampler.rnd_float() > rr_p)
      {
        res += throughput * past_direct;
        break;
      }
    }
    #endif

    // update seed for next BRDF
    seed = next_seed(seed);

    // bounce ray
    r = bounce_ray(hit_point,rec->p_error(),info.gnormal(),scatter_dir);
  }

  return res;
}
