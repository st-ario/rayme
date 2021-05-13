#pragma once

#include "ray.h"

class camera
{
  private:
    point origin;
    float aspect_ratio;
    point upper_left_corner;
    normed_vec3 rel_z;
    normed_vec3 rel_x;
    normed_vec3 rel_y;
    float viewport_width;
    float viewport_height;
    float z_near;
    float z_far;

  public:
    camera( point orig
          , vec3 look_at
          , float vertical_fov_in_deg
          , float image_ratio
          , vec3 absolute_y
          , float znear = 0.1f
          , float zfar = infinity
          );

    ray get_ray(float horiz_factor, float vert_factor) const;

    float get_zfar() const;
};