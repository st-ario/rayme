#pragma once

#include <array>

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

class ggx_brdf : public brdf
{
  public:
    ggx_brdf(const material* ptr_mat, const normed_vec3* normal)
      : brdf{normal}, ptr_mat{ptr_mat},
        alpha{ptr_mat->roughness_factor * ptr_mat->roughness_factor},
        alpha_squared{alpha * alpha},
        to_world{[&]
          {
            glm::mat3 res{rotate_given_north_pole(*normal)};
            std::swap(res[1],res[2]);
            res[1] *= -1;
            return res;
          }()},
        to_local{glm::transpose(to_world)} {}

    virtual float pdf( const normed_vec3& minus_wo
                     , const normed_vec3& wi) const override;

    virtual color f_r( const normed_vec3& minus_wo
                     , const normed_vec3& wi) const override;

    virtual normed_vec3 sample_dir( const ray& r_minus_wo
                                  , uint16_t pixel_x
                                  , uint16_t pixel_y
                                  , uint16_t seed) const override;

  protected:
    const material* ptr_mat;
    const float alpha;
    const float alpha_squared;
  private:
    const glm::mat3 to_world;
    const glm::mat3 to_local;

    float D(const vec3& loc_h) const;
    float Lambda(const vec3& loc_wo) const;
    float G1(const vec3& loc_wo) const;
    float G2(const vec3& loc_wo, const vec3& loc_wi) const;
    float D_wo(const vec3& loc_h, const vec3& loc_wo) const;
    vec3  sample_halfvector(const vec3& loc_wo, const std::array<float,2>& rnd) const;

  protected:
    // multi-scatter methods
    color MSF(const color& F0) const;
    float f_ms( const normed_vec3& minus_wo
              , const normed_vec3& wi
              , const std::array<std::pair<std::array<float,2>,float>,1024>& E_table
              , const std::array<std::array<float,2>,32>& Eavg_table) const;
};

class metal_brdf: public ggx_brdf
{
  public:
    using ggx_brdf::ggx_brdf;

    virtual color f_r( const normed_vec3& minus_wo
                     , const normed_vec3& wi) const override;
};

class dielectric_brdf: public ggx_brdf
{
  public:
    dielectric_brdf(const material* ptr_mat, const normed_vec3* normal)
    : ggx_brdf{ptr_mat, normal}, base{ptr_mat,normal} {}

    virtual float pdf( const normed_vec3& minus_wo
                     , const normed_vec3& wi) const override;

    virtual color f_r( const normed_vec3& minus_wo
                     , const normed_vec3& wi) const override;

    virtual normed_vec3 sample_dir( const ray& r_minus_wo
                                  , uint16_t pixel_x
                                  , uint16_t pixel_y
                                  , uint16_t seed) const override;

  private:
    const diffuse_brdf base;

    float E_spec(const normed_vec3& wi) const;
    float fresnel( const normed_vec3& minus_wo
                 , const normed_vec3& wi) const;
};