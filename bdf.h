#pragma once

#include <array>
#include <variant>

#include "materials.h"
#include "ray.h"
#include "ms_tables/ms_tables.h"

//debug macros
//#define LAMBERTIAN_DIFFUSE 1
//#define NO_MS 1

class brdf
{
  public:
    explicit brdf(const normed_vec3* normal, uint64_t seed) : normal{normal}, seed{seed}
    {}

    virtual float pdf( const normed_vec3& wo
                     , const normed_vec3& wi) const = 0;

    virtual color f_r( const normed_vec3& wo
                     , const normed_vec3& wi) const = 0;

    virtual normed_vec3 sample_dir(const normed_vec3& wo) const = 0;

    // direct light estimator for Monte Carlo integration,
    // importance sampling formula, assuming wi has been sampled using sample_dir()
    virtual color estimator( const normed_vec3& wo
                           , const normed_vec3& wi) const = 0;
    // { return f_r(wo,wi) * dot(wi,*normal) / pdf(wo,wi); }

  protected:
    const normed_vec3* normal;
    const uint64_t seed;

    static sampler_1d& get_sampler(uint64_t seed);
};

class diffuse_brdf : public brdf
{
  public:
    diffuse_brdf(const material* ptr_mat, const normed_vec3* normal, uint64_t seed)
    : brdf{normal, seed}, ptr_mat{ptr_mat}
    #ifdef LAMBERTIAN_DIFFUSE
    {}
    #else
    // Oren--Nayar's sigma (angular standard deviation) in principle takes values in [0,pi/2],
    // but physically most of the values don't make sense; the common cap seems to be at around 0.3
    // with rare exceptions up to 0.5; heuristic multiplicative factor (0.36^2)
    // (try different values at a later stage)
    // IMPORTANT: the energy compensation tables for the diffuse multi-scatter depend on this
    // choice, if the multiplicative factor is changed they have to be recomputed;
    // for the current choice the energy loss is negligible, so the tables are omitted
    , sigma_squared{0.13f * ptr_mat->roughness_factor * ptr_mat->roughness_factor}
    , A{1.0f - sigma_squared / (2.0f * (sigma_squared + 0.33f))}
    , B{0.45f * sigma_squared / (sigma_squared + 0.09f)} {}
    #endif

    virtual color f_r( const normed_vec3& wo
                     , const normed_vec3& wi) const override;

    virtual float pdf( const normed_vec3& wo
                     , const normed_vec3& wi) const override
    {
      float cos_wi{epsilon_clamp(dot(*normal,wi))};

      if (cos_wi <= 0.0f)
        return 0.0f;

      return cos_wi / pi;
    }

    virtual normed_vec3 sample_dir(const normed_vec3& wo) const override
    {
      return cos_weighted_random_hemisphere_unit(*normal
        , get_sampler(seed).rnd_float(), get_sampler(seed).rnd_float());
    }

    virtual color estimator( const normed_vec3& wo
                           , const normed_vec3& wi) const override
    {
      float ndotl{dot(*normal,wi)};
      float ndotv{dot(*normal,wo)};
      if (ndotl > 0 && ndotv > 0)
      {
        #ifdef LAMBERTIAN_DIFFUSE
        return ptr_mat->base_color;
        #else
        return pi * f_r(wo,wi);
        #endif
      }
      return color{0.0f};
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
    ggx_brdf(const material* ptr_mat, const normed_vec3* normal, uint64_t seed)
      : brdf{normal, seed}, ptr_mat{ptr_mat},
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

    virtual float pdf( const normed_vec3& wo
                     , const normed_vec3& wi) const override;

    virtual color f_r( const normed_vec3& wo
                     , const normed_vec3& wi) const override;

    virtual normed_vec3 sample_dir(const normed_vec3& wo) const override;

    virtual color estimator( const normed_vec3& wo
                           , const normed_vec3& wi) const override
    {
      float ndotl{dot(*normal,wi)};
      float ndotv{dot(*normal,wo)};
      if (ndotl > 0 && ndotv > 0)
      {
        vec3 loc_wo = to_local * wo.to_vec3();
        vec3 loc_wi = to_local * wi.to_vec3();
        vec3 loc_wh = glm::normalize(loc_wo + loc_wi);
        return color{G2(loc_wo,loc_wi,loc_wh) / G1(loc_wo,loc_wh)};
      }
      return color{0.0f};
    }

  protected:
    const material* ptr_mat;
    const float alpha;
    const float alpha_squared;
  private:
    const glm::mat3 to_world;
    const glm::mat3 to_local;

    float D(const vec3& loc_h) const;
    float Lambda(const vec3& loc_wo) const
    {
      float z_squared{loc_wo.z * loc_wo.z};
      float k{alpha_squared * (1.0f - z_squared) / z_squared};
      return 0.5f * (-1.0f + std::sqrt(1.0f + k));
    }
    float G1(const vec3& loc_wo, const vec3& loc_wm) const
    {
      if (dot(loc_wo,loc_wm) < 0.0f)
        return 0.0f;

      return 1.0f / (1.0f + Lambda(loc_wo));
    }
    float G2(const vec3& loc_wo, const vec3& loc_wi, const vec3& loc_wm) const
    {
      // Height-Correlated Masking and Shadowing

      if (dot(loc_wi,loc_wm) < 0.0f || dot(loc_wo,loc_wm) < 0.0f)
        return 0.0;

      return 1.0f / (1.0f + Lambda(loc_wi) + Lambda(loc_wo));
    }
    float D_wo(const vec3& loc_h, const vec3& loc_wo) const;
    normed_vec3 sample_halfvector(const vec3& loc_wo, const std::array<float,2>& rnd) const;

  #ifndef NO_MS
  protected:
    // multi-scatter methods
    template <typename T>
    T MSFresnel(const T& F0) const { return F0 * (T{0.04f} + F0 * (T{0.66f} + T{0.3f} * F0)); }

    template <size_t M, size_t N>
    float f_ms( const normed_vec3& wo
              , const normed_vec3& wi
              , const std::array<std::pair<std::array<float,2>,float>,M>& E_table
              , const std::array<std::array<float,2>,N>& Eavg_table) const
    {
      float E_avg{ms_lookup_Eavg(ptr_mat->roughness_factor,Eavg_table)};
      if (E_avg == 1.0)
        return 0.0f;
      // IMPORTANT check for > 1.0 if the table changes, or make sure it doesn't contain values > 1

      float cos_i{dot(*normal,wi)};
      float cos_o{dot(*normal,wo)};
      if (cos_i > 0.0f && cos_o > 0.0f)
      {
        float E_i{ms_lookup_E(std::array<float,2>{ptr_mat->roughness_factor,dot(*normal,wi)},E_table)};
        float E_o{ms_lookup_E(std::array<float,2>{ptr_mat->roughness_factor,dot(*normal,wo)},E_table)};

        float res{std::max(1.0f - E_i, 0.0f) * std::max(1.0f - E_o, 0.0f) / (pi * (1.0f - E_avg))};

        return res;
      }

      return 0.0f;
    }
  #endif
};

class metal_brdf : public ggx_brdf
{
  public:
    using ggx_brdf::ggx_brdf;

    virtual color f_r( const normed_vec3& wo
                     , const normed_vec3& wi) const override;

    virtual color estimator( const normed_vec3& wo
                           , const normed_vec3& wi) const override
    {
      float ndotl{dot(*normal,wi)};
      float ndotv{dot(*normal,wo)};
      if (ndotl > 0 && ndotv > 0)
      {
        vec3 wh{glm::normalize(wo.to_vec3() + wi.to_vec3())};
        float pr{pdf(wo,wi)};
        float invpdf{1.0f / pr};
        #ifdef NO_MS
        return ggx_brdf::estimator(wo,wi) * fresnel(dot(wo,wh));
        #else
        return (pr == 0.0f) ? ggx_brdf::estimator(wo,wi) * fresnel(dot(wo,wh)) :
          ggx_brdf::estimator(wo,wi) * fresnel(dot(wo,wh))
          + MSFresnel(ptr_mat->base_color)
          * ((f_ms(wo,wi,ggx_E,ggx_Eavg) * dot(*normal,wi) * invpdf));
        #endif
      }
      return color{0.0f};
    }
  private:
    color fresnel(float cos_angle) const
    {
      auto pow5 = [](float x)
      {
        float x2{x*x};
        return x2 * x2 * x;
      };
      return ptr_mat->base_color + (color{1.0f} - ptr_mat->base_color) * pow5(1.0f - cos_angle);
    }
};

class dielectric_brdf: public ggx_brdf
{
  public:
    dielectric_brdf(const material* ptr_mat, const normed_vec3* normal, uint64_t seed)
    : ggx_brdf{ptr_mat, normal, seed}, base{ptr_mat,normal, seed} {}

    virtual float pdf( const normed_vec3& wo
                     , const normed_vec3& wi) const override;

    virtual color f_r( const normed_vec3& wo
                     , const normed_vec3& wi) const override;

    virtual normed_vec3 sample_dir(const normed_vec3& wo) const override;

    virtual color estimator( const normed_vec3& wo
                           , const normed_vec3& wi) const override
    {
      float ndotl{dot(*normal,wi)};
      float ndotv{dot(*normal,wo)};
      if (ndotl > 0 && ndotv > 0)
      {
        vec3 h{glm::normalize(wo.to_vec3()+wi.to_vec3())};
        float ldoth{dot(wi,h)};
        float invpdf{1.0f / pdf(wo,wi)};
        return fresnel(ldoth) * ggx_brdf::estimator(wo,wi)
        #ifdef NO_MS
          + ((1.0f - fresnel(ndotl)) * (1.0f - fresnel(ndotv))) * base.estimator(wo,wi);
        #else
        // IMPORTANT this is using that the current diffuse has no energy loss, change accordingly
        // if this is no longer the case
          + MSFresnel(color{0.04f}) * (f_ms(wo,wi,ggx_E,ggx_Eavg) * dot(*normal,wi) * invpdf)
          + (1.0f - E_spec(wo)) * base.estimator(wo,wi);
        #endif
      }
      return color{0.0f};
    }

  private:
    const diffuse_brdf base;

    #ifndef NO_MS
    float E_spec(const normed_vec3& w) const;
    #endif
    float fresnel(float cos_angle) const;
};

class composite_brdf : public brdf
{
  public:
    composite_brdf(const material* ptr_mat, const normed_vec3* normal, uint64_t seed)
    : brdf{normal,seed}, ptr_mat{ptr_mat}
    , surf
      {[&]{
        if (ptr_mat->metallic_factor == 0)
          return Surface::dielectric;
        else if (ptr_mat->metallic_factor == 1)
          return Surface::metal;
        else
        {
          float r{brdf::get_sampler(seed).rnd_float()};
          if (r < ptr_mat->metallic_factor)
            return Surface::metal;
          return Surface::dielectric;
        }
      }()}
    , m_brdf
      {[&]{
        switch(surf)
        {
          case Surface::dielectric:
            return std::variant<dielectric_brdf,metal_brdf>(dielectric_brdf{ptr_mat,normal,seed});
          case Surface::metal:
            return std::variant<dielectric_brdf,metal_brdf>(metal_brdf{ptr_mat,normal,seed});
        }
      }()}
      {}

    virtual float pdf( const normed_vec3& wo
                     , const normed_vec3& wi) const override
    {
      switch(surf)
      {
        case Surface::dielectric:
          return std::get<dielectric_brdf>(m_brdf).pdf(wo,wi);
        case Surface::metal:
          return std::get<metal_brdf>(m_brdf).pdf(wo,wi);
      }
    }

    virtual color f_r( const normed_vec3& wo
                     , const normed_vec3& wi) const override
    {
      switch(surf)
      {
        case Surface::dielectric: return std::get<dielectric_brdf>(m_brdf).f_r(wo,wi);
        case Surface::metal: return std::get<metal_brdf>(m_brdf).f_r(wo,wi);
      }
    }

    virtual normed_vec3 sample_dir(const normed_vec3& wo) const override
    {
      switch(surf)
      {
        case Surface::dielectric: return std::get<dielectric_brdf>(m_brdf).sample_dir(wo);
        case Surface::metal: return std::get<metal_brdf>(m_brdf).sample_dir(wo);
      }
    }

    virtual color estimator( const normed_vec3& wo
                           , const normed_vec3& wi) const override
    {
      switch(surf)
      {
        case Surface::dielectric: return std::get<dielectric_brdf>(m_brdf).estimator(wo,wi);
        case Surface::metal: return std::get<metal_brdf>(m_brdf).estimator(wo,wi);
      }
    }

  private:
    const material* ptr_mat;
    enum class Surface
    {
      dielectric,
      metal,
    };
    const Surface surf;
    std::variant<dielectric_brdf,metal_brdf> m_brdf;
};