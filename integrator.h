#pragma once

#include "ray.h"
#include "bvh.h"

color integrate_path( const ray& r
                    , const bvh_tree& world
                    , uint16_t depth);