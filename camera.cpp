#include "camera.h"
#include "transformations.h"
#include "extern/glm/glm/gtx/norm.hpp"
#include "extern/glm/glm/mat3x3.hpp"
#include "rng.h"

camera::camera( float yfov_in_radians
              , float znear
              , float aspect_ratio
              , uint16_t image_height_in_pixels)
  : origin{0.0f, 0.0f, 0.0f}
  , aspect_ratio{aspect_ratio}
  , yfov{yfov_in_radians}
  , canvas_height{static_cast<float>(image_height_in_pixels)}
  , canvas_width{canvas_height * aspect_ratio}
  , rel_z{normed_vec3::absolute_z()}
  , rel_x{normed_vec3::absolute_x()}
  , rel_y{normed_vec3::absolute_y()}
  , to_world{mat3{}}
  , rel_upper_left_corner{ -canvas_width /2.0f
                         ,  canvas_height/2.0f
                         , -canvas_height/(2.0f * std::tan(yfov / 2.0f))}
  , znear{znear} {}

ray camera::get_ray(uint16_t pixel_x, uint16_t pixel_y) const
{
  vec3 nonunital_rel_direction{ rel_upper_left_corner
                              + vec3{pixel_x + 0.5f, -(pixel_y + 0.5f), 0.0f}};
  vec3 nonunital_direction{to_world*nonunital_rel_direction};

  return ray(origin,unit(nonunital_direction));
}

ray camera::get_offset_ray(uint16_t pixel_x, uint16_t pixel_y, std::array<float,2> rnd) const
{
  vec3 nonunital_rel_direction{rel_upper_left_corner
                              + vec3{   pixel_x + rnd[0]
                                    , -(pixel_y + rnd[1])
                                    ,   0.0f}};
  vec3 nonunital_direction{to_world*nonunital_rel_direction};

  return ray{origin,unit(nonunital_direction)};
}

float camera::get_aspect_ratio() const { return aspect_ratio; }
void  camera::set_aspect_ratio(float ratio)
{
  aspect_ratio = ratio;
  canvas_width = canvas_height * aspect_ratio;
  rel_upper_left_corner = { -canvas_width /2.0f
                          ,  canvas_height/2.0f
                          , -canvas_height/(2.0f * std::tan(yfov / 2.0f))};
}

uint16_t camera::get_image_width()   const { return static_cast<uint16_t>(canvas_width);  }
uint16_t camera::get_image_height()  const { return static_cast<uint16_t>(canvas_height); }

void     camera::set_image_height(uint16_t height)
{
  canvas_height = height;
  canvas_width = height * aspect_ratio;
  rel_upper_left_corner = { -canvas_width /2.0f
                          ,  canvas_height/2.0f
                          , -canvas_height/(2.0f * std::tan(yfov / 2.0f))};
}

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

  to_world = { epsilon_clamp(rel_x.x())
             , epsilon_clamp(rel_x.y())
             , epsilon_clamp(rel_x.z())
             , epsilon_clamp(rel_y.x())
             , epsilon_clamp(rel_y.y())
             , epsilon_clamp(rel_y.z())
             , epsilon_clamp(rel_z.x())
             , epsilon_clamp(rel_z.y())
             , epsilon_clamp(rel_z.z())};
}