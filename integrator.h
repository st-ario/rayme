#pragma once

#include "ray.h"
#include "bvh.h"

color sample_bdf(const ray& r, const element& world, uint16_t integration_samples_N, uint16_t depth);
color sample_lights(const ray& r, const element& world, uint16_t integration_samples_N, uint16_t depth);