#pragma once

#include "ray.h"

class transformation;

class camera
{
  private:
    point origin;
    float aspect_ratio;
    normed_vec3 rel_z;
    normed_vec3 rel_x;
    normed_vec3 rel_y;
    point rel_upper_left_corner;
    float viewport_width;
    float viewport_height;
    float znear;
    float zfar;

  public:
    camera( point origin
          , vec3 look_at
          , float vertical_fov_in_deg
          , float aspect_ratio
          , vec3 absolute_y
          , float znear = 0.1f
          , float zfar = infinity
          );

    camera( float yfov_in_radians
          , float znear);

    ray get_ray(float horiz_factor, float vert_factor) const;

    float get_zfar() const;
    float get_aspect_ratio() const;
    void set_zfar(float far);
    void set_aspect_ratio(float ratio);

    void absolute_transform_by(const transformation& transform);
    void transform_rel_origin_by(const transformation& transform);
};