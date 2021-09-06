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

  rel_upper_left_corner = point{-(viewport_width/2.0f), viewport_height/2.0f, -1.0f};
}

camera::camera( float yfov_in_radians ,float znear)
  : origin{0.0f, 0.0f, 0.0f}
  , aspect_ratio{16.0f / 9.0f}
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

  rel_upper_left_corner = point{-(viewport_width/2.0f), viewport_height/2.0f, -1.0f};
}


ray camera::get_ray(float horiz_factor, float vert_factor) const
{
  vec3 nonunital_direction = rel_upper_left_corner - origin
                             + (horiz_factor * viewport_width)  * rel_x.to_vec3()
                             - (vert_factor  * viewport_height) * rel_y.to_vec3();
  point offset_origin = origin + znear * unit(nonunital_direction).to_vec3();
  return ray(offset_origin, unit(nonunital_direction));
}

float camera::get_zfar() const { return zfar; }
float camera::get_aspect_ratio() const { return aspect_ratio; }
void camera::set_zfar(float far) { zfar = far; }
void camera::set_aspect_ratio(float ratio) { aspect_ratio = ratio; }

void camera::absolute_transform_by(const transformation& transform)
{
  transform.apply_to(origin);
  transform_rel_origin_by(transform);
}

void camera::transform_rel_origin_by(const transformation& transform)
{
  vec3 new_z{transform * rel_z.to_vec3()};
  rel_z = unit(new_z);
  normed_vec3 absolute_y{point{0.0f,1.0f,0.0f}};
  if (rel_z.to_vec3() == absolute_y.to_vec3()) { /* TODO */ };
  rel_x = normed_vec3{cross(absolute_y,rel_z)};
  rel_y = normed_vec3{cross(rel_z,rel_x)};
}