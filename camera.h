#pragma once

#include "ray.h"

class camera
{
  private:
    point origin;
    double aspect_ratio;
    point upper_left_corner;
    normed_vec rel_x;
    normed_vec rel_y;
    double viewport_width;
    double viewport_height;
  
  public:
    camera( point orig
          , vec nonunital_dir_of_view
          , double vertical_fov_in_deg
          , double image_ratio
          , vec absolute_y
          );

    ray get_ray(double horiz_factor, double vert_factor);
};