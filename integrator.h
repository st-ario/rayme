#pragma once

#include "ray.h"
#include "bvh.h"

color integrate_path( const ray& r
                    , const element& world
                    , uint16_t depth
                    , uint16_t pixel_x
                    , uint16_t pixel_y
                    , uint16_t sample_id);