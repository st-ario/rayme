#pragma once

#include "ray.h"
#include "bvh.h"

color integrate_path( const ray& r
                    , const element& world
                    , uint16_t integration_samples_N
                    , uint16_t depth);