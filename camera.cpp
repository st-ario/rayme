#include "camera.h"
#include "transformations.h"


// TODO deal properly with zfar

camera::camera( point origin
      , vec3 look_at
      , float vertical_fov_in_deg
      , float aspect_ratio
      , vec3 absolute_y
      , float znear
      , float zfar
      ) : origin{origin}, aspect_ratio{aspect_ratio}
        , rel_z{origin-look_at}
        , rel_x{cross(absolute_y,rel_z)}
        , rel_y{cross(rel_z,rel_x)}
        , znear{znear}
        , zfar{zfar}
{
  float angle = degrees_to_radians(vertical_fov_in_deg);
  float h  = std::tan(angle/2.0f);
  viewport_height = h * 2.0f;
  viewport_width = aspect_ratio * viewport_height;

  point viewport_center = origin - rel_z.to_vec3();

  upper_left_corner = viewport_center
                    - (viewport_width/2.0f)  * rel_x.to_vec3()
                    + (viewport_height/2.0f) * rel_y.to_vec3();
}

camera::camera( float yfov_in_radians ,float znear)
  : origin{0.0f, 0.0f, 0.0f}
  , aspect_ratio{16.0f / 9.0f}
  , upper_left_corner{}
  , rel_z{vec3{0.0f, 0.0f, 1.0f}}
  , rel_x{vec3{1.0f, 0.0f, 0.0f}}
  , rel_y{vec3{0.0f, 1.0f, 0.0f}}
  , viewport_width{}
  , viewport_height{}
  , znear{znear}
  , zfar{infinity}
{
  float angle = yfov_in_radians;
  float h  = std::tan(angle/2.0f);
  viewport_height = h * 2.0f;
  viewport_width = aspect_ratio * viewport_height;

  upper_left_corner = point{-(viewport_width/2.0f), viewport_height/2.0f, -1.0f};
}


ray camera::get_ray(float horiz_factor, float vert_factor) const
{
  vec3 nonunital_direction = upper_left_corner
                          + (horiz_factor * viewport_width)  * rel_x.to_vec3()
                          - (vert_factor  * viewport_height) * rel_y.to_vec3()
                          - origin;
  //return ray(origin, unit(nonunital_direction));
  point offset_origin = origin + znear * unit(nonunital_direction).to_vec3();
  return ray(offset_origin, unit(nonunital_direction));
}

float camera::get_zfar() const { return zfar; }
float camera::get_aspect_ratio() const { return aspect_ratio; }
void camera::set_zfar(float far) { zfar = far; }
void camera::set_aspect_ratio(float ratio) { aspect_ratio = ratio; }

void camera::transform_by(const transformation& transform)
{
  transform.apply_to(origin);
  transform.apply_to(upper_left_corner);

  vec3 new_x{transform * rel_x.to_vec3()};
  vec3 new_y{transform * rel_y.to_vec3()};
  vec3 new_z{transform * rel_z.to_vec3()};
  rel_x = unit(new_x);
  rel_y = unit(new_y);
  rel_z = unit(new_z);
}