#include "camera.h"
#include "transformations.h"
#include "extern/glm/glm/gtx/norm.hpp"
#include "extern/glm/glm/mat3x3.hpp"

camera::camera(float yfov_in_radians, float znear, float aspect_ratio)
  : origin{0.0f, 0.0f, 0.0f}
  , aspect_ratio{aspect_ratio}
  , yfov{yfov_in_radians}
  , sensor_width{2 * std::tan(yfov/2.0f)}
  , rel_z{normed_vec3::absolute_z()}
  , rel_x{normed_vec3::absolute_x()}
  , rel_y{normed_vec3::absolute_y()}
  , znear{znear} {}

ray camera::get_ray(float horiz_factor, float vert_factor, int canvas_height_in_pixels) const
{
  float scale = canvas_height_in_pixels / sensor_width;
  float canvas_width = canvas_height_in_pixels * aspect_ratio;
  point rel_upper_left_corner{-(canvas_width/2.0f), canvas_height_in_pixels/2.0f, -scale};

  vec3 nonunital_direction = rel_upper_left_corner - origin
                             + (horiz_factor * canvas_width)  * rel_x
                             - (vert_factor  * canvas_height_in_pixels) * rel_y;
  point offset_origin = origin + znear * unit(nonunital_direction);
  return ray(offset_origin, unit(nonunital_direction));
}

float camera::get_aspect_ratio() const { return aspect_ratio; }
void camera::set_aspect_ratio(float ratio) { aspect_ratio = ratio; }

void camera::transform_by(const transformation& transform)
{
  origin *= transform;

  // extract rotation matrix and apply it to the axes
  float scale_0 = glm::length2(vec3(transform[0]));
  float scale_1 = glm::length2(vec3(transform[1]));
  float scale_2 = glm::length2(vec3(transform[2]));

  glm::mat3 rotation = { epsilon_clamp(vec3(transform[0])/scale_0)
                       , epsilon_clamp(vec3(transform[1])/scale_1)
                       , epsilon_clamp(vec3(transform[2])/scale_2)};

  rel_z = unit(rotation * rel_z.to_vec3());
  rel_x = unit(rotation * rel_x.to_vec3());
  rel_y = unit(rotation * rel_y.to_vec3());
}