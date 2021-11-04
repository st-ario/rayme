#pragma once

#include "materials.h"
#include "ray.h"

//debug macros
//#define LAMBERTIAN_DIFFUSE 1

struct brdf_sample
{
  const float pdf;
  const color f_r;
  const normed_vec3 scatter_dir;

  brdf_sample(float pdf, color f_r, normed_vec3 scatter_dir) :
    pdf{pdf}, f_r{f_r}, scatter_dir{scatter_dir} {}
};

class brdf
{
  public:
    explicit brdf(const normed_vec3* normal) : normal{normal} {}

    virtual float pdf( const normed_vec3& minus_wo
                     , const normed_vec3& wi) const = 0;

    virtual color f_r( const normed_vec3& minus_wo
                     , const normed_vec3& wi) const = 0;

    virtual normed_vec3 sample_dir( const ray& r_minus_wo
                                  , uint16_t pixel_x
                                  , uint16_t pixel_y
                                  , uint16_t seed) const = 0;

    brdf_sample sample( const ray& r_minus_wo
                      , uint16_t pixel_x
                      , uint16_t pixel_y
                      , uint16_t seed) const
    {
      normed_vec3 scatter_dir{sample_dir(r_minus_wo,pixel_x,pixel_y,seed)};
      float prob{pdf(r_minus_wo.direction,scatter_dir)};
      color f{f_r(r_minus_wo.direction,scatter_dir)};

      return brdf_sample(prob,f,scatter_dir);
    }

  protected:
    const normed_vec3* normal;
};

class diffuse_brdf : public brdf
{
  public:
    diffuse_brdf(const material* ptr_mat, const normed_vec3* normal)
    : brdf{normal}, ptr_mat{ptr_mat}
    #ifdef LAMBERTIAN_DIFFUSE
    {}
    #else
    , sigma_squared{ptr_mat->roughness_factor * ptr_mat->roughness_factor}
    , A{1.0f - sigma_squared / (2.0f * (sigma_squared + 0.33f))}
    , B{0.45f * sigma_squared / (sigma_squared + 0.09f)} {}
    #endif

    virtual color f_r( const normed_vec3& minus_wo
                     , const normed_vec3& wi) const override;

    virtual float pdf( const normed_vec3& minus_wo
                     , const normed_vec3& wi) const override
    {
      float cos_wi{epsilon_clamp(dot(*normal,wi))};

      if (cos_wi <= 0.0f)
        return 0.0f;

      return cos_wi / pi;
    }

    virtual normed_vec3 sample_dir( const ray& r_minus_wo
                                  , uint16_t pixel_x
                                  , uint16_t pixel_y
                                  , uint16_t seed) const override
    {
      return cos_weighted_random_hemisphere_unit(*normal,pixel_x,pixel_y,seed);
    }

  private:
    const material* ptr_mat;
    #ifndef LAMBERTIAN_DIFFUSE
    const float sigma_squared;
    const float A;
    const float B;
    #endif
};