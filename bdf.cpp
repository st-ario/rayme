#include "bdf.h"
#include "ray.h"
#include "bvh.h"
#include "ms_tables/ms_tables.h"

color diffuse_brdf::f_r( const normed_vec3& wo
                       , const normed_vec3& wi) const
{
  #ifdef LAMBERTIAN_DIFFUSE
  return color{ptr_mat->base_color * invpi};
  #else
  float cos_wi{clamp(dot(*normal,wi),0.0f,1.0f)};
  float cos_wo{clamp(dot(wo,*normal),0.0f,1.0f)};
  float sin_wi{epsilon_clamp(std::sqrt(1.0f - cos_wi * cos_wi))};
  float sin_wo{epsilon_clamp(std::sqrt(1.0f - cos_wo * cos_wo))};
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

inline float ggx_brdf::D(const vec3& loc_h) const
{
  if (loc_h.z > 0)
  {

    float k{1.0f + loc_h.z * loc_h.z * (alpha_squared - 1.0f)};
    return alpha_squared / (pi * k * k);
  }
  return 0.0f;
}

inline float ggx_brdf::Lambda(const vec3& loc_wo) const
{
  float z_squared{loc_wo.z * loc_wo.z};
  float k{alpha_squared * (1.0f - z_squared) / z_squared};
  return 0.5f * (-1.0f + std::sqrt(1.0f + k));
}

inline float ggx_brdf::G1(const vec3& loc_wo, const vec3& loc_wm) const
{
  if (dot(loc_wo,loc_wm) > 0.0f)
    return 1.0f / (1.0f + Lambda(loc_wo));

  return 0.0f;
}

inline float ggx_brdf::G2(const vec3& loc_wo, const vec3& loc_wi, const vec3& loc_wm) const
{
  // Height-Correlated Masking and Shadowing

  if (dot(loc_wi,loc_wm) > 0.0f && dot(loc_wo,loc_wm) > 0.0f)
    return 1.0f / (1.0f + Lambda(loc_wi) + Lambda(loc_wo));

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

normed_vec3 ggx_brdf::sample_dir( const normed_vec3& wo
                                , uint16_t pixel_x
                                , uint16_t pixel_y
                                , uint16_t seed) const
{
  vec3 loc_wo{to_local * wo.to_vec3()};
  normed_vec3 loc_h{sample_halfvector(loc_wo,random_float_pair(pixel_x,pixel_y,seed))};
  vec3 loc_wi{(2.0f * dot(loc_wo,loc_h)) * loc_h - loc_wo};

  return unit(to_world * loc_wi);
}

inline float ggx_brdf::pdf( const normed_vec3& wo
                          , const normed_vec3& wi) const
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
inline color ggx_brdf::MSF(const color& F0) const
{
  return F0 * (color{0.04f} + F0 * (color{0.66f} + color{0.3f} * F0));
}

float ggx_brdf::f_ms( const normed_vec3& wo
                    , const normed_vec3& wi
                    , const std::array<std::pair<std::array<float,2>,float>,1024>& E_table
                    , const std::array<std::array<float,2>,32>& Eavg_table) const
{
  float E_i{ms_lookup(std::array<float,2>{dot(*normal,wi),alpha},E_table)};
  float E_o{ms_lookup(std::array<float,2>{dot(*normal,wo),alpha},E_table)};
  float E_avg{ms_lookup(alpha,Eavg_table)};
  return (1.0f - E_i) * (1.0f - E_o) / std::max(0.001f, (pi - E_avg));
}

float dielectric_brdf::E_spec(const normed_vec3& wo) const
{
  static constexpr float F_avg{0.089497712f};
  float E_o{ms_lookup(std::array<float,2>{dot(*normal,wo),alpha},ggx_E)};
  //return (F_avg * E_o + MSF(color{0.04f}).x * (1.0f - E_o)); // TODO try just 0.04f instead of MSF
  return (F_avg * E_o + 0.04f * (1.0f - E_o));
}
#endif

color metal_brdf::f_r( const normed_vec3& wo
                     , const normed_vec3& wi) const
{
  auto pow5 = [](float x){ return (x * x) * (x * x) * x; };
  vec3 h{glm::normalize(wi.to_vec3() + wo.to_vec3())};
  float wodoth{dot(wo,h)};
  color fresnel{
    ptr_mat->base_color + (color{1.0f} - ptr_mat->base_color) * pow5(1.0f - std::abs(wodoth))
  };

  return fresnel * ggx_brdf::f_r(wo, wi)
        #ifdef NO_MS
        ;
        #else
         + MSF(ptr_mat->base_color) * f_ms(wo, wi, ggx_E, ggx_Eavg);
         #endif
}

inline float dielectric_brdf::fresnel( const normed_vec3& wo
                                     , const normed_vec3& wi) const
{
  auto pow5 = [](float x){ return (x * x) * (x * x) * x; };
  float wodoth{0.5f * dot(wi,wo)};
  return 0.04f + 0.96f * pow5(1.0f - std::abs(wodoth));
}


color dielectric_brdf::f_r( const normed_vec3& wo
                          , const normed_vec3& wi) const
{
  #ifndef NO_MS
  static constexpr float A1{8.854467355133801f};
  static constexpr float tau{0.28430405702379613f};
  color rho{ptr_mat->base_color};
  color F_dms{A1 * tau * tau * (rho * rho) / (color{1.0f} - tau * rho)};
  #endif
  float fr{fresnel(wo,wi)};

  return fr * ggx_brdf::f_r(wo,wi)
         #ifdef NO_MS
         + (1.0f - fr) * base.f_r(wo,wi);
         #else
         //+ MSF(color{0.04f}) * f_ms(wo, wi, ggx_E, ggx_Eavg) // TODO try just 0.04f instead of MSF
         + 0.04f * f_ms(wo, wi, ggx_E, ggx_Eavg)
         + (base.f_r(wo,wi) + F_dms * f_ms(wo, wi, on_E, on_Eavg))
         * (1.0f - E_spec(wo));
         #endif
}

normed_vec3 dielectric_brdf::sample_dir( const normed_vec3& wo
                                       , uint16_t pixel_x
                                       , uint16_t pixel_y
                                       , uint16_t seed) const
{
  float rnd{random_float(pixel_x,pixel_y,seed)};
  #ifndef NO_MS
  // sample specular distribution with probability E_spec and diffuse with prob 1-E_spec
  float k{E_spec(wo)};

  if (rnd < k)
    return ggx_brdf::sample_dir(wo,pixel_x,pixel_y,seed);

  return base.sample_dir(wo,pixel_x,pixel_y,seed);
  #else
  if (rnd < 0.5f)
    return ggx_brdf::sample_dir(wo,pixel_x,pixel_y,seed);

  return base.sample_dir(wo,pixel_x,pixel_y,seed);
  #endif
}

inline float dielectric_brdf::pdf( const normed_vec3& wo
                                 , const normed_vec3& wi) const
{
  #ifdef NO_MS
  return 0.5f * ggx_brdf::pdf(wo,wi) + 0.5f * base.pdf(wo,wi);
  #else
  float k{E_spec(wi)};

  return k * ggx_brdf::pdf(wo,wi) + (1.0f - k) * base.pdf(wo,wi);
  #endif
}