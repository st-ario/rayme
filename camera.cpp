#include "camera.h"

camera::camera( point origin
      , vec3 look_at
      , float vertical_fov_in_deg
      , float aspect_ratio
      , vec3 absolute_y
      , float z_near
      , float z_far
      ) : origin{origin}, aspect_ratio{aspect_ratio}
        , rel_z{origin-look_at}
        , rel_x{cross(absolute_y,rel_z)}
        , rel_y{cross(rel_z,rel_x)}
        , z_near{z_near}
        , z_far{z_far}
      {
        float angle = degrees_to_radians(vertical_fov_in_deg);
        float h  = std::tan(angle/2.0f);
        viewport_height = h * 2.0f;
        viewport_width = aspect_ratio * viewport_height;

        point viewport_center = origin - static_cast<vec3>(rel_z);

        upper_left_corner = viewport_center
                          - (viewport_width/2.0f)  * static_cast<vec3>(rel_x)
                          + (viewport_height/2.0f) * static_cast<vec3>(rel_y);
      }

ray camera::get_ray(float horiz_factor, float vert_factor) const
{
  vec3 nonunital_direction = upper_left_corner
                          + (horiz_factor * viewport_width)  * static_cast<vec3>(rel_x)
                          - (vert_factor  * viewport_height) * static_cast<vec3>(rel_y)
                          - origin;
  //return ray(origin, unit(nonunital_direction));
  point offset_origin = origin + z_near * static_cast<vec3>(unit(nonunital_direction));
  normed_vec3 dir{unit(nonunital_direction)};
  return ray(offset_origin, dir);
}

float camera::get_zfar() const { return z_far; }