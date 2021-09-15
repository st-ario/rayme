#pragma once

#include "ray.h"
#include "extern/glm/glm/mat3x3.hpp"

using mat3 = glm::mat3;

class transformation;

class camera
{
  public:
    camera( float yfov_in_radians
          , float znear
          , float aspect_ratio = 16.0f/9.0f
          , uint16_t image_height_in_pixels = 1080
          );

    ray get_ray(uint16_t pixel_x, uint16_t pixel_y) const;
    ray get_stochastic_ray(uint16_t pixel_x, uint16_t pixel_y) const;

    float get_aspect_ratio() const;
    void  set_aspect_ratio(float ratio);

    uint16_t get_image_width()  const;
    uint16_t get_image_height() const;
    void set_image_height(uint16_t height);

    void transform_by(const transformation& transform);

  private:
    point origin;
    float aspect_ratio;
    float yfov;
    float canvas_height;
    float canvas_width;
    normed_vec3 rel_z;
    normed_vec3 rel_x;
    normed_vec3 rel_y;
    mat3 to_world;
    vec3 rel_upper_left_corner;
    float znear;
};