#pragma once

#include "camera.h"
#include "materials.h"
#include "meshes.h"
#include "images.h"

class camera;
class bvh_tree;

void render( image& picture
           , int samples_per_pixel
           , int depth
           , const camera& cam
           , const bvh_tree& world);