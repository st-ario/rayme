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
    explicit brdf(const normed_vec3* normal, uint64_t seed);

    virtual float pdf( const normed_vec3& wo
                     , const normed_vec3& wi) const = 0;

    virtual normed_vec3 sample_dir(const normed_vec3& wo) const = 0;

    // direct light estimator for Monte Carlo integration,
    // importance sampling formula, assuming wi has been sampled using sample_dir()
    virtual color estimator( const normed_vec3& wo
                           , const normed_vec3& wi) const = 0;
    // estimator = f_r(wo,wi) * dot(wi,*normal) / pdf(wo,wi)
    // -> use f_r = estimator * pdf / dot(wi,*normal) when needed

  protected:
    const normed_vec3* normal;
    const uint64_t seed;

    static sampler_1d& get_sampler(uint64_t seed);
};

class diffuse_brdf : public brdf
{
  public:
    diffuse_brdf(const material* ptr_mat, const normed_vec3* normal, uint64_t seed);


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
    color f_r(const normed_vec3& wo, const normed_vec3& wi) const;
};

class ggx_brdf : public brdf
{
  public:
    ggx_brdf(const material* ptr_mat, const normed_vec3* normal, uint64_t seed);

    virtual float pdf( const normed_vec3& wo
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
  public:
    // multi-scatter methods
    template <typename T>
    static
    T MSFresnel(const T& F0) { return F0 * (T{0.04f} + F0 * (T{0.66f} + T{0.3f} * F0)); }

    template <size_t M, size_t N>
    static
    float f_ms( const normed_vec3& wo
              , const normed_vec3& wi
              , float roughness_factor
              , const normed_vec3& normal
              , const std::array<std::pair<std::array<float,2>,float>,M>& E_table
              , const std::array<std::array<float,2>,N>& Eavg_table)
    {
      float E_avg{ms_lookup_Eavg(roughness_factor,Eavg_table)};
      if (E_avg == 1.0)
        return 0.0f;
      // IMPORTANT check for > 1.0 if the table changes, or make sure it doesn't contain values > 1

      float cos_i{dot(normal,wi)};
      float cos_o{dot(normal,wo)};
      if (cos_i > 0.0f && cos_o > 0.0f)
      {
        float E_i{ms_lookup_E(std::array<float,2>{roughness_factor,dot(normal,wi)},E_table)};
        float E_o{ms_lookup_E(std::array<float,2>{roughness_factor,dot(normal,wo)},E_table)};

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

    virtual color estimator( const normed_vec3& wo
                           , const normed_vec3& wi) const override
    {
      float ndotl{dot(*normal,wi)};
      float ndotv{dot(*normal,wo)};
      if (ndotl > 0 && ndotv > 0)
      {
        vec3 wh{glm::normalize(wo.to_vec3() + wi.to_vec3())};
        float pr{pdf(wo,wi)};
        #ifdef NO_MS
        return ggx_brdf::estimator(wo,wi) * fresnel(dot(wo,wh));
        #else
        return (pr == 0.0f) ? ggx_brdf::estimator(wo,wi) * fresnel(dot(wo,wh)) :
          ggx_brdf::estimator(wo,wi) * fresnel(dot(wo,wh))
          + ggx_brdf::MSFresnel(ptr_mat->base_color)
          * ((ggx_brdf::f_ms(wo,wi,ptr_mat->roughness_factor,*normal,ggx_E,ggx_Eavg) * dot(*normal,wi) / pr));
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

class dielectric_brdf: public brdf
{
  public:
    dielectric_brdf(const material* ptr_mat, const normed_vec3* normal, uint64_t seed);

    virtual float pdf( const normed_vec3& wo
                     , const normed_vec3& wi) const override
    {
      switch (lobe)
      {
        case Lobe::diffuse:
          return std::get<diffuse_brdf>(m_brdf).pdf(wo,wi);
        case Lobe::specular:
          return std::get<ggx_brdf>(m_brdf).pdf(wo,wi);
      }
    }

    virtual normed_vec3 sample_dir(const normed_vec3& wo) const override
    {
      switch (lobe)
      {
        case Lobe::diffuse:
          return std::get<diffuse_brdf>(m_brdf).sample_dir(wo);
        case Lobe::specular:
          return std::get<ggx_brdf>(m_brdf).sample_dir(wo);
      }
    }

    virtual color estimator( const normed_vec3& wo
                           , const normed_vec3& wi) const override
    {
      float ndotl{dot(*normal,wi)};
      float ndotv{dot(*normal,wo)};
      switch (lobe)
      {
        // the factor of 2 in front of each return is to compensate for the choice of the lobe
        case Lobe::diffuse:
        // IMPORTANT this is using that the current diffuse has no energy loss, change accordingly
        // if this is no longer the case
          #ifdef NO_MS
          return 2.0f * ((1.0f-fresnel(ndotl)) * (1.0f-fresnel(ndotv)))
                * std::get<diffuse_brdf>(m_brdf).estimator(wo,wi);
          #else
          return 2.0f * (1.0f-E_spec(wo))
                * std::get<diffuse_brdf>(m_brdf).estimator(wo,wi);
          #endif
        case Lobe::specular:
          if (ndotl > 0 && ndotv > 0)
          {
            vec3 h{glm::normalize(wo.to_vec3()+wi.to_vec3())};
            float ldoth{dot(wi,h)};
            #ifdef NO_MS
            return 2.0f * fresnel(ldoth) * std::get<ggx_brdf>(m_brdf).estimator(wo,wi);
            #else
            float pr{pdf(wo,wi)};
            // deterministic bounce -> all energy is reflected
            return (pr == 0) ? 2.0f*fresnel(ldoth) * std::get<ggx_brdf>(m_brdf).estimator(wo,wi) :
              2.0f * fresnel(ldoth) * std::get<ggx_brdf>(m_brdf).estimator(wo,wi)
                + ggx_brdf::MSFresnel(0.04f)
                * (ggx_brdf::f_ms(wo,wi,ptr_mat->roughness_factor,*normal,ggx_E,ggx_Eavg) * dot(*normal,wi) / pr);
            #endif
          }
          return color{0.0f};
      }
    }

  private:
    const material* ptr_mat;
    enum class Lobe
    {
      diffuse,
      specular,
    };
    const Lobe lobe;
    std::variant<diffuse_brdf,ggx_brdf> m_brdf;

    #ifndef NO_MS
    float E_spec(const normed_vec3& w) const;
    #endif
    float fresnel(float cos_angle) const;
};

class composite_brdf : public brdf
{
  public:
    composite_brdf(const material* ptr_mat, const normed_vec3* normal, uint64_t seed);

    virtual float pdf( const normed_vec3& wo
                     , const normed_vec3& wi) const override
    {
      switch(lobe)
      {
        case Lobe::dielectric:
          return std::get<dielectric_brdf>(m_brdf).pdf(wo,wi);
        case Lobe::metal:
          return std::get<metal_brdf>(m_brdf).pdf(wo,wi);
      }
    }

    virtual normed_vec3 sample_dir(const normed_vec3& wo) const override
    {
      switch(lobe)
      {
        case Lobe::dielectric: return std::get<dielectric_brdf>(m_brdf).sample_dir(wo);
        case Lobe::metal: return std::get<metal_brdf>(m_brdf).sample_dir(wo);
      }
    }

    virtual color estimator( const normed_vec3& wo
                           , const normed_vec3& wi) const override
    {
      switch(lobe)
      {
        case Lobe::dielectric: return std::get<dielectric_brdf>(m_brdf).estimator(wo,wi);
        case Lobe::metal: return std::get<metal_brdf>(m_brdf).estimator(wo,wi);
      }
    }

  private:
    const material* ptr_mat;
    enum class Lobe
    {
      dielectric,
      metal,
    };
    const Lobe lobe;
    std::variant<dielectric_brdf,metal_brdf> m_brdf;
};