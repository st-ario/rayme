#pragma once

#include "images.h"

class camera;
class bvh_tree;

void render( image& picture
           , image* albedo_map
           , image* normal_map
           , uint16_t samples_per_pixel
           , uint16_t min_depth
           , const camera& cam
           , const bvh_tree& world);