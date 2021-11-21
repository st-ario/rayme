#pragma once

#include "ray.h"
#include "bvh.h"
#include "rng.h"

class integrator
{
  public:
    explicit integrator(uint64_t seed)
    : rng_1d{seed} {}

    color integrate_path( const ray& r
                        , const bvh_tree& world);
  private:
    sampler_1d rng_1d;
};