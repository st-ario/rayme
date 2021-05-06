#pragma once

#include "ray.h"

class camera
{
  private:
    point origin;
    float aspect_ratio;
    point upper_left_corner;
    normed_vec rel_z;
    normed_vec rel_x;
    normed_vec rel_y;
    float viewport_width;
    float viewport_height;
    float znear;
    float zfar;
  
  public:
    camera( point orig
          , vec look_at
          , float vertical_fov_in_deg
          , float image_ratio
          , vec absolute_y
          , float znear
          );

    camera( point orig
          , vec look_at
          , float vertical_fov_in_deg
          , float image_ratio
          , vec absolute_y
          , float znear
          , float zfar
          );

    ray get_ray(float horiz_factor, float vert_factor) const;
    float get_znear() const;
    float get_zfar() const;
};