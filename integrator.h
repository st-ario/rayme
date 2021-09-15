#pragma once

#include "ray.h"
#include "bvh.h"

color incoming_light(point r, const normed_vec3& normal, const element& world);