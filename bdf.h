#pragma once

#include "materials.h"

class ray;

struct brdf_sample
{
  const float pdf;
  const color f_r;
  const normed_vec3 scatter_dir;
  const float cos_angle;

  brdf_sample(float pdf, color f_r, normed_vec3 scatter_dir, float cos_angle) :
    pdf{pdf}, f_r{f_r}, scatter_dir{scatter_dir}, cos_angle{cos_angle} {}
};

class diffuse_brdf
{
  public:
    explicit diffuse_brdf(const material* ptr_mat) : ptr_mat{ptr_mat} {}

   brdf_sample sample( const point& at
                     , const normed_vec3& gnormal
                     , const normed_vec3& snormal
                     , uint16_t pixel_x
                     , uint16_t pixel_y
                     , uint16_t seed) const;

  private:
    const material* ptr_mat;
};