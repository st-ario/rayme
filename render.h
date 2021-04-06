#pragma once

#include <iostream>
#include <fstream>

#include "camera.h"
#include "elements.h"
#include "images.h"

void render( image& picture
           , int samples_per_pixel
           , int depth
           , const camera& cam
           , const element& world);