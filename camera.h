#pragma once

#include "ray.h"

class transformation;

class camera
{
  public:
    camera( float yfov_in_radians
          , float znear
          , float aspect_ratio = 16.0f/9.0f
          );

    ray get_ray(float horiz_factor, float vert_factor, int canvas_height_in_pixels) const;

    float get_aspect_ratio() const;
    void set_aspect_ratio(float ratio);

    void transform_by(const transformation& transform);

  private:
    point origin;
    float aspect_ratio;
    float yfov;
    float sensor_width;
    normed_vec3 rel_z;
    normed_vec3 rel_x;
    normed_vec3 rel_y;
    float znear;
};