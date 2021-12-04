#pragma once

#include "ray.h"
#include "bvh.h"
#include "rng.h"

class brdf;
class integrator
{
  public:
    explicit integrator(uint64_t seed)
    : sampler{seed} {}

    color integrate_path( ray& r
                        , const bvh_tree& world
                        , uint16_t min_depth) const;
  private:
    color sample_light( const point& x
                      , const normed_vec3& gnormal
                      , const normed_vec3& snormal
                      , const normed_vec3& incoming_dir
                      , const hit_record& record
                      , const bvh_tree& world
                      , const brdf& b) const;

    sampler_1d sampler;
};