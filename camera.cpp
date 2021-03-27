#include "camera.h"
#include "math.h"
#include <cmath>

camera::camera( point orig
      , vec nonunital_dir_of_view
      , double vertical_fov_in_deg
      , double image_ratio
      , vec absolute_y
      ) : origin{orig}, aspect_ratio{image_ratio}
        , rel_x{cross(nonunital_dir_of_view,absolute_y)}
        , rel_y{cross(rel_x,nonunital_dir_of_view)}
      {
        double angle = degrees_to_radians(vertical_fov_in_deg);
        double h  = std::tan(angle/2.0);
        viewport_height = h * 2.0;
        viewport_width = aspect_ratio * viewport_height;

        point viewport_center = origin + unit(nonunital_dir_of_view).to_vec();

        upper_left_corner = viewport_center
                          - (viewport_width/2.0)  * rel_x.to_vec()
                          + (viewport_height/2.0) * rel_y.to_vec();
      }

ray camera::get_ray(double horiz_factor, double vert_factor)
{
  vec nonunital_direction = upper_left_corner
                          + (horiz_factor * viewport_width) * rel_x.to_vec()
                          - (vert_factor  * viewport_height) * rel_y.to_vec()
                          - origin;
  return ray(origin, unit(nonunital_direction));
}