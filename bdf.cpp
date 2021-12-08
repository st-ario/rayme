#include "bdf.h"
#include "ray.h"
#include "bvh.h"

sampler_1d& brdf::get_sampler(uint64_t seed)
{
  static thread_local sampler_1d sampler{seed};

  return sampler;
}

brdf::brdf(const normed_vec3* normal, uint64_t seed) : normal{normal}, seed{seed} {}

diffuse_brdf::diffuse_brdf(const material* ptr_mat, const normed_vec3* normal, uint64_t seed)
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

ggx_brdf::ggx_brdf(const material* ptr_mat, const normed_vec3* normal, uint64_t seed)
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

dielectric_brdf::dielectric_brdf(const material* ptr_mat, const normed_vec3* normal, uint64_t seed)
  : ggx_brdf{ptr_mat, normal, seed}, base{ptr_mat,normal, seed} {}

composite_brdf::composite_brdf(const material* ptr_mat, const normed_vec3* normal, uint64_t seed)
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
    {[&](const material* p_m, const normed_vec3* n, uint64_t s)
      -> std::variant<dielectric_brdf,metal_brdf>{
      switch(surf)
      {
        case Surface::dielectric:
          return dielectric_brdf{p_m,n,s};
        case Surface::metal:
          return metal_brdf{p_m,n,s};
      }
    }(ptr_mat,normal,seed)}
    {}

color diffuse_brdf::f_r( const normed_vec3& wo
                       , const normed_vec3& wi) const
{
  float ndotl{dot(*normal,wi)};
  float ndotv{dot(*normal,wo)};
  if (ndotl > 0 && ndotv > 0)
  {
    #ifdef LAMBERTIAN_DIFFUSE
    return color{ptr_mat->base_color * invpi};
    #else
    float cos_wi{dot(*normal,wi)};
    float cos_wo{dot(wo,*normal)};
    float sin_wi{std::sqrt(1.0f - clamp(cos_wi * cos_wi,0.0,1.0))};
    float sin_wo{std::sqrt(1.0f - clamp(cos_wo * cos_wo,0.0,1.0))};
    float cos_difference{cos_wo * cos_wi + sin_wo * sin_wi};

    float sin_alpha;
    float tan_beta;
    if (cos_wo < cos_wi)
    {
      sin_alpha = sin_wo;
      tan_beta = sin_wi / (machine_two_epsilon + cos_wi);
    } else {
      sin_alpha = sin_wi;
      tan_beta = sin_wo / (machine_two_epsilon + cos_wo);
    }
    return color{ptr_mat->base_color * invpi
                  * (A + B * std::max(0.0f,cos_difference) * sin_alpha * tan_beta)};
    #endif
  }
  return color{0.0f};
}

inline float ggx_brdf::D(const vec3& loc_h) const
{
  if (loc_h.z > 0.0f)
  {
    float k{1.0f + loc_h.z * loc_h.z * (alpha_squared - 1.0f)};
    if (k != 0.0f)
      return alpha_squared / (pi * k * k);
  }
  return 0.0f;
}

inline float ggx_brdf::D_wo(const vec3& loc_h, const vec3& loc_wo) const
{
  float vdoth{dot(loc_wo,loc_h)};
  if (vdoth > 0.0f && loc_wo.z > 0.0f)
    return G1(loc_wo, loc_h) * vdoth * D(loc_h) / loc_wo.z;

  return 0.0f;
}

normed_vec3 ggx_brdf::sample_halfvector(const vec3& loc_wo, const std::array<float,2>& rnd) const
{
  // Heitz, Sampling the GGX Distribution of Visible Normals
  // Journal of Computer Graphics Techniques Vol. 7, No. 4, 2018

  // transforming the view direction to the hemisphere configuration
  vec3 Vh{glm::normalize(vec3(alpha * loc_wo.x, alpha * loc_wo.y, loc_wo.z))};

  // orthonormal basis (with special case if cross product is zero)
  float lensq{Vh.x * Vh.x + Vh.y * Vh.y};
  vec3 T1{lensq > 0 ? vec3(-Vh.y, Vh.x, 0) * fast_inverse_sqrt(lensq) : vec3(1,0,0)};
  vec3 T2{cross(Vh, T1)};

  // parameterization of the projected area
  float r{std::sqrt(rnd[0])};
  float phi{2.0f * pi * rnd[1]};
  float t1{r * std::cos(phi)};
  float t2{r * std::sin(phi)};
  float s{0.5f * (1.0f + Vh.z)};
  t2 = (1.0f - s)*std::sqrt(1.0f - t1*t1) + s*t2;

  // reprojection onto hemisphere
  vec3 Nh{t1*T1 + t2*T2 + std::sqrt(std::max(0.0f, 1.0f - t1*t1 - t2*t2))*Vh};

  // transforming the normal back to the ellipsoid configuration
  vec3 Ne{vec3(alpha * Nh.x, alpha * Nh.y, std::max(0.0f, Nh.z))};

  return unit(Ne);
}

normed_vec3 ggx_brdf::sample_dir(const normed_vec3& wo) const
{
  vec3 loc_wo{to_local * wo.to_vec3()};
  normed_vec3 loc_h{sample_halfvector(loc_wo,{get_sampler(seed).rnd_float(),get_sampler(seed).rnd_float()})};
  vec3 loc_wi{(2.0f * dot(loc_wo,loc_h)) * loc_h - loc_wo};

  return unit(to_world * loc_wi);
}

float ggx_brdf::pdf(const normed_vec3& wo, const normed_vec3& wi) const
{
  if (dot(wo,*normal) > 0 && dot(wi,*normal) > 0)
  {
    vec3 h{glm::normalize(wi.to_vec3() + wo.to_vec3())};
    float wodoth{dot(wo,h)};
    h = to_local * h;
    vec3 loc_wo{to_local * wo.to_vec3()};

    return D_wo(h,loc_wo) / (4.0f * wodoth);
  }

  return 0.0f;
}

inline color ggx_brdf::f_r( const normed_vec3& wo
                          , const normed_vec3& wi) const
{
  if (dot(wo,*normal) > 0 && dot(wi,*normal) > 0)
  {
    vec3 loc_wi{to_local * wi.to_vec3()};
    vec3 loc_wo{to_local * wo.to_vec3()};
    vec3 loc_h{glm::normalize(loc_wi + loc_wo)};

    float res{G2(loc_wo,loc_wi,loc_h) * D(loc_h) / (4.0f * loc_wo.z * loc_wi.z)};

    return color{res};
  }

  return color{0.0f};
}

#ifndef NO_MS
float dielectric_brdf::E_spec(const normed_vec3& w) const
{
  static constexpr float F_avg{0.089497712f};
  float E_o{ms_lookup_E(std::array<float,2>{ptr_mat->roughness_factor, dot(*normal,w)},ggx_E)};
  return (F_avg * E_o + MSFresnel(color{0.04f}).x * (1.0f - E_o));
}
#endif

color metal_brdf::f_r( const normed_vec3& wo
                     , const normed_vec3& wi) const
{
  float ndotl{dot(*normal,wi)};
  float ndotv{dot(*normal,wo)};
  if (ndotl > 0 && ndotv > 0)
  {
    vec3 wh{glm::normalize(wo.to_vec3()+wi.to_vec3())};
    return ggx_brdf::f_r(wo,wi) * fresnel(dot(wo,wh))
    #ifdef NO_MS
    ;
    #else
    + MSFresnel(ptr_mat->base_color) * ggx_brdf::f_ms(wo,wi,ggx_E,ggx_Eavg);
    #endif
  }
  return color{0.0f};
}

float dielectric_brdf::fresnel(float cos_angle) const
{
  // TODO measure performance difference between exact and approximated formula
  float g{std::sqrt(1.25f + cos_angle * cos_angle)};
  float gmc{g-cos_angle};
  float gpc{g+cos_angle};
  float auxup{cos_angle*(gpc) - 1.0f};
  float auxdn{cos_angle*(gmc) + 1.0f};
  float x{1.0f + (auxup * auxup) / (auxdn * auxdn)};
  return 0.5f * (gmc * gmc) * x / (gpc * gpc);
  /*
  auto pow5 = [](float x)
  {
    float x2{x*x};
    return x2 * x2 * x;
  };
  //return 0.04f + 0.96f * pow5(1.0f - clamp(cos_angle,0.0f,1.0f));
  return 0.04f + 0.96f * pow5(1.0f - cos_angle);
  */
}

color dielectric_brdf::f_r( const normed_vec3& wo
                          , const normed_vec3& wi) const
{
  float ndotl{dot(*normal,wi)};
  float ndotv{dot(*normal,wo)};
  if (ndotl > 0 && ndotv > 0)
  {
    #ifndef NO_MS
    // no energy loss for current diffuse
    /*
    static constexpr float A1{8.854467355133801f};
    static constexpr float tau{0.28430405702379613f};
    color rho{ptr_mat->base_color};
    color F_dms{A1 * tau * tau * (rho * rho) / (color{1.0f} - tau * rho)};
    */
    #endif
    vec3 h{glm::normalize(wo.to_vec3()+wi.to_vec3())};
    float ldoth{dot(wi,h)};

    return fresnel(ldoth) * ggx_brdf::f_r(wo,wi)
    #ifdef NO_MS
      + (1.0f - fresnel(ndotl)) * (1.0f - fresnel(ndotv)) * base.f_r(wo,wi) ;
    #else
      + MSFresnel(color{0.04f}) * ggx_brdf::f_ms(wo,wi,ggx_E,ggx_Eavg)
      + base.f_r(wo,wi) * (1.0f - E_spec(wo));
      // no energy loss for current diffuse, otherwise
      //+ (base.f_r(wo,wi) + MSFresnel(ptr_mat->base_color) * f_ms(wo,wi,on_E,on_Eavg)) * (1.0f - E_spec(wo));
    #endif
  }

  return color{0.0f};
}

normed_vec3 dielectric_brdf::sample_dir(const normed_vec3& wo) const
{
  float rnd{get_sampler(seed).rnd_float()};
  #ifndef NO_MS
  // sample specular distribution with probability E_spec and diffuse with prob 1-E_spec
  float k{E_spec(wo)};

  if (rnd < k)
    return ggx_brdf::sample_dir(wo);

  return base.sample_dir(wo);
  #else
  if (rnd < 0.5f)
    return ggx_brdf::sample_dir(wo);

  return base.sample_dir(wo);
  #endif
}

float dielectric_brdf::pdf(const normed_vec3& wo, const normed_vec3& wi) const
{
  #ifdef NO_MS
  return 0.5f * ggx_brdf::pdf(wo,wi) + 0.5f * base.pdf(wo,wi);
  #else
  float k{E_spec(wo)};

  return k * ggx_brdf::pdf(wo,wi) + (1.0f - k) * base.pdf(wo,wi);
  #endif
}