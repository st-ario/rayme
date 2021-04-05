#pragma once

#include "ray.h"

class camera
{
  private:
    point origin;
    double aspect_ratio;
    point upper_left_corner;
    normed_vec rel_z;
    normed_vec rel_x;
    normed_vec rel_y;
    double viewport_width;
    double viewport_height;
    double znear;
    double zfar;
  
  public:
    camera( point orig
          , vec look_at
          , double vertical_fov_in_deg
          , double image_ratio
          , vec absolute_y
          , double znear
          );

    camera( point orig
          , vec look_at
          , double vertical_fov_in_deg
          , double image_ratio
          , vec absolute_y
          , double znear
          , double zfar
          );

    ray get_ray(double horiz_factor, double vert_factor);
    double get_znear();
    double get_zfar();
};