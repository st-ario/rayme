#include "camera.h"
#include "math.h"
#include <cmath>

camera::camera( point orig
      , vec look_at
      , double vertical_fov_in_deg
      , double image_ratio
      , vec absolute_y
      , double z_near
      ) : origin{orig}, aspect_ratio{image_ratio}
        , rel_z{orig-look_at}
        , rel_x{cross(absolute_y,rel_z)}
        , rel_y{cross(rel_z,rel_x)}
        , znear{z_near}
        , zfar{infinity}
      {
        double angle = degrees_to_radians(vertical_fov_in_deg);
        double h  = std::tan(angle/2.0);
        viewport_height = h * 2.0;
        viewport_width = aspect_ratio * viewport_height;

        point viewport_center = origin - rel_z.to_vec();

        upper_left_corner = viewport_center
                          - (viewport_width/2.0)  * rel_x.to_vec()
                          + (viewport_height/2.0) * rel_y.to_vec();
      }

camera::camera( point orig
      , vec look_at
      , double vertical_fov_in_deg
      , double image_ratio
      , vec absolute_y
      , double z_near
      , double z_far
      ) : origin{orig}, aspect_ratio{image_ratio}
        , rel_z{orig-look_at}
        , rel_x{cross(absolute_y,rel_z)}
        , rel_y{cross(rel_z,rel_x)}
        , znear{z_near}
        , zfar{z_far}
      {
        double angle = degrees_to_radians(vertical_fov_in_deg);
        double h  = std::tan(angle/2.0);
        viewport_height = h * 2.0;
        viewport_width = aspect_ratio * viewport_height;

        point viewport_center = origin - rel_z.to_vec();

        upper_left_corner = viewport_center
                          - (viewport_width/2.0)  * rel_x.to_vec()
                          + (viewport_height/2.0) * rel_y.to_vec();
      }

ray camera::get_ray(double horiz_factor, double vert_factor) const
{
  vec nonunital_direction = upper_left_corner
                          + (horiz_factor * viewport_width) * rel_x.to_vec()
                          - (vert_factor  * viewport_height) * rel_y.to_vec()
                          - origin;
  return ray(origin, unit(nonunital_direction));
}

double camera::get_znear() const { return znear; }
double camera::get_zfar()  const { return zfar; }