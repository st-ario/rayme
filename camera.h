#pragma once

#include "my_vectors.h"
#include "ray.h"
#include "math.h"

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

class camera
{
  private:
    point origin;
    double aspect_ratio;
    double focal_length;
    point upper_left_corner;
    normed_vec rel_x;
    normed_vec rel_z;
    double viewport_width;
    double viewport_height;
  
  public:
    camera( point orig
          , vec nonunital_dir_of_view
          , double focal_len
          , double vertical_fov_in_deg
          , double image_ratio
          , vec absolute_z
          ) : origin{orig}, aspect_ratio{image_ratio}, focal_length{focal_len},
              rel_x{cross(nonunital_dir_of_view,absolute_z)},
              rel_z{cross(rel_x,nonunital_dir_of_view)}
          {
            double angle = degrees_to_radians(vertical_fov_in_deg);
            double h  = std::tan(angle/2.0);
            viewport_height = h * 2.0;
            viewport_width = aspect_ratio * viewport_height;

            point viewport_center = origin + focal_length * unit(nonunital_dir_of_view).to_vec();

            upper_left_corner = viewport_center
                              - (viewport_width/2.0)  * rel_x.to_vec()
                              + (viewport_height/2.0) * rel_z.to_vec();
          }

    ray get_ray(double horiz_factor, double vert_factor)
    {
      vec nonunital_direction = upper_left_corner
                              + (horiz_factor * viewport_width) * rel_x.to_vec()
                              - (vert_factor  * viewport_height) * rel_z.to_vec()
                              - origin;
      return ray(origin, unit(nonunital_direction));
    }

};