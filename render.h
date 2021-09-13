#pragma once

#include "images.h"

class camera;
class bvh_tree;

void render( image& picture
           , uint16_t samples_per_pixel
           , const camera& cam
           , const bvh_tree& world);