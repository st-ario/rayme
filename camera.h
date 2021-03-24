#pragma once

#include "my_vectors.h"
#include "ray.h"
#include "math.h"

class camera
{
  private:
    point origin;
    double aspect_ratio;
    double focal_length;
    point upper_left_corner;
    vec rel_x;
    vec rel_z;
    double viewport_width;
    double viewport_height;
  
  public:
    camera( point orig
          , vec nonunital_dir_of_view
          , double focal_len
          , double vertical_fov_in_deg
          , double image_ratio
          , vec absolute_z
          ) : origin{orig}, aspect_ratio{image_ratio}, focal_length{focal_len}
          {
            rel_x = unit(cross(nonunital_dir_of_view,absolute_z));
            rel_z = unit(cross(rel_x,nonunital_dir_of_view));

            double angle = degrees_to_radians(vertical_fov_in_deg);
            double h  = std::tan(angle/2.0);
            viewport_height = h * 2.0;
            viewport_width = aspect_ratio * viewport_height;

            point viewport_center = origin + focal_length * unit(nonunital_dir_of_view);

            upper_left_corner = viewport_center
                              - (viewport_width/2.0)  * rel_x
                              + (viewport_height/2.0) * rel_z;
          }

    ray get_ray(double horiz_factor, double vert_factor)
    {
      vec nonunital_direction = upper_left_corner
                              + (horiz_factor * viewport_width) * rel_x
                              - (vert_factor  * viewport_height) * rel_z
                              - origin;
      return ray(origin, nonunital_direction);
    }

};

static vec random_in_unit_disk()
{
  double rx = standard_normal_random_double();
  double ry = standard_normal_random_double();
  double norm = std::sqrt(rx*rx + ry*ry);
  if (norm == 0)
    return random_in_unit_disk();
  vec rp = (vec(rx,ry,0) / norm);
  double random_radius = random_double();
  return random_radius * vec::random_unit();
}