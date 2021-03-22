#pragma once

#include "my_vectors.h"
#include "ray.h"

class camera
{
  public:
    camera() : origin{point(0,0,0)}, viewport_height{2.0}, aspect_ratio{16.0/9.0}, focal_length{1.0}
    {
      viewport_width = aspect_ratio * viewport_height;
      upper_left_corner = point(-viewport_width/2.0, focal_length, viewport_height/2.0);
    }

    ray get_ray(double u, double v)
    {
      point offset = point(u * viewport_width , 0, - v * viewport_height);
      return ray(origin, upper_left_corner + offset);
    }

    double get_aspect_ratio() const
    {
      return aspect_ratio;
    }

  private:
    point origin;
    point upper_left_corner;
    double aspect_ratio;
    double focal_length;
    double viewport_height;
    double viewport_width;
};