#pragma once

#include "math.h"

struct material
{
  color base_color = {1.0,1.0,1.0};
  float alpha = 1.0f;
  bool emitter = false;
  float metallic_factor = 1.0f;
  float roughness_factor = 1.0f;
  vec3 emissive_factor;
};