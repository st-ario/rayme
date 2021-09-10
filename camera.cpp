#include "camera.h"
#include "transformations.h"

camera::camera(float yfov_in_radians, float znear, float aspect_ratio)
  : origin{0.0f, 0.0f, 0.0f}
  , aspect_ratio{aspect_ratio}
  , rel_z{unit(vec3{0.0f, 0.0f, 1.0f})}
  , rel_x{unit(vec3{1.0f, 0.0f, 0.0f})}
  , rel_y{unit(vec3{0.0f, 1.0f, 0.0f})}
  , viewport_width{-1}
  , viewport_height{-1}
  , znear{znear}
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
                             + (horiz_factor * viewport_width)  * rel_x
                             - (vert_factor  * viewport_height) * rel_y;
  point offset_origin = origin + znear * unit(nonunital_direction);
  return ray(offset_origin, unit(nonunital_direction));
}

float camera::get_aspect_ratio() const { return aspect_ratio; }
void camera::set_aspect_ratio(float ratio) { aspect_ratio = ratio; }

void camera::transform_by(const transformation& transform)
{
  origin *= transform;
  rel_z = unit(transform * rel_z.to_vec3());
  normed_vec3 absolute_y{unit(vec3{0.0f,1.0f,0.0f})};
  if (rel_z == absolute_y) { /* TODO */ };
  rel_x = normed_vec3{cross(absolute_y,rel_z)};
  rel_y = normed_vec3{cross(rel_z,rel_x)};
}