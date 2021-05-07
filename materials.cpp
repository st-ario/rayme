#include "materials.h"
#include "math.h"

std::optional<ray> lambertian::scatter(
    const ray& r
  , const hit_record& rec
  , color& attenuation
  ) const
{
  attenuation = albedo;
  vec3 nonunital_scatter_direction = rec.normal.to_vec3() + vec3::random_unit();
  if (nonunital_scatter_direction.near_zero())
  {
    return ray(rec.p, rec.normal);
  } else {
    return ray(rec.p, unit(nonunital_scatter_direction));
  }
}
// Alternatively: scatter with probability p and have attenuation be given by albedo/p


std::optional<ray> metal::scatter(
        const ray& r
      , const hit_record& rec
      , color& attenuation
 ) const
{
  attenuation = albedo;
  vec3 reflected = reflect(r.direction, rec.normal).to_vec3();
  normed_vec3 scattered_direction = unit(reflected + roughness*vec3::random_in_unit_sphere());
  ray scattered = ray(rec.p, scattered_direction);
  if (dot(scattered.direction, rec.normal) > 0)
    return scattered;
  return std::nullopt;
}

std::optional<ray> dielectric::scatter(
    const ray& r
  , const hit_record& rec
  , color& attenuation
  ) const
{
  attenuation = color(1.0, 1.0, 1.0);

  float refraction_ratio = rec.front_face ? (1.0 / refraction_index) : refraction_index;
  float cos_incindence_angle = std::fmin(1.0, dot(r.direction, rec.normal));
  float sin_incindence_angle = std::sqrt(1.0 - cos_incindence_angle * cos_incindence_angle);
  float sin_refracted_angle = refraction_ratio * sin_incindence_angle;
  float cos_refracted_angle = std::sqrt(1.0 - sin_refracted_angle * sin_refracted_angle);
  bool cannot_refract = sin_refracted_angle > 1.0;

  bool aux = (cannot_refract || random_float() > fresnel_reflectance(cos_incindence_angle, cos_refracted_angle, refraction_index));

  normed_vec3 direction = get_direction(aux, r.direction, rec.normal, refraction_ratio);

  return ray(rec.p,direction);
}

static normed_vec3 get_direction(bool b, normed_vec3 dir, normed_vec3 n, float ref_ratio)
{
  if (b)
    return reflect(dir,n);
  else
    return refract(dir,n,ref_ratio);
}

static float reflectance(float cos, float refraction_index)
{
  // Use Schlick's approximation for reflectance.
  float r0 = (1.0 - refraction_index) / (1.0 + refraction_index);
  r0 = r0*r0;
  return r0 + (1.0 - r0) * std::pow((1.0 - cos), 5);
}

static float fresnel_reflectance(float cos_incidence, float cos_refraction, float refraction_index)
{
  float parallel = (refraction_index * cos_incidence - cos_refraction)/ (refraction_index * cos_incidence + cos_refraction);
  parallel = parallel * parallel;

  float perp = (cos_refraction - refraction_index * cos_incidence)/ (cos_refraction + refraction_index * cos_incidence);
  perp = perp * perp;

  float factor = (parallel + perp)/2;
  return factor;
}