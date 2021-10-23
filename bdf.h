#pragma once

#include "materials.h"

//debug macros
//#define LAMBERTIAN_DIFFUSE 1

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
    #ifdef LAMBERTIAN_DIFFUSE
    explicit diffuse_brdf(const material* ptr_mat) : ptr_mat{ptr_mat} {}
    #else
    explicit diffuse_brdf(const material* ptr_mat) : ptr_mat{ptr_mat}
    {
      sigma_squared = ptr_mat->roughness_factor * ptr_mat->roughness_factor;
    }
    #endif

   #ifdef LAMBERTIAN_DIFFUSE
   brdf_sample sample( const point& at
   #else
   brdf_sample sample( const ray& r_incoming
   #endif
                     , const normed_vec3& gnormal
                     , const normed_vec3& snormal
                     , uint16_t pixel_x
                     , uint16_t pixel_y
                     , uint16_t seed) const;

  private:
    const material* ptr_mat;
    #ifndef LAMBERTIAN_DIFFUSE
    float sigma_squared;
    const float A{1.0f - sigma_squared / (2.0f * (sigma_squared + 0.33f))};
    const float B{0.45f * sigma_squared / (sigma_squared + 0.09f)};
    #endif
};