#include "materials.h"
#include "math.h"

std::optional<ray> lambertian::scatter(
    const ray& r
  , const point& p
  , double t
  , bool front_face
  , const std::shared_ptr<material>& ptr_mat
  , const normed_vec& normal
  , const hit_record& rec
  , color& attenuation
  ) const
{
  attenuation = albedo;
  vec nonunital_scatter_direction = normal.to_vec() + vec::random_unit();
  if (nonunital_scatter_direction.near_zero())
  {
    return ray(p, normal);
  } else {
    return ray(p, unit(nonunital_scatter_direction));
  }
}
// Alternatively: scatter with probability p and have attenuation be given by albedo/p


std::optional<ray> metal::scatter(
        const ray& r
      , const point& p
      , double t
      , bool front_face
      , const std::shared_ptr<material>& ptr_mat
      , const normed_vec& normal
      , const hit_record& rec
      , color& attenuation
 ) const
{
  attenuation = albedo;
  vec reflected = reflect(r.direction, normal).to_vec();
  normed_vec scattered_direction = unit(reflected + roughness*vec::random_in_unit_sphere());
  ray scattered = ray(p, scattered_direction);
  if (dot(scattered.direction, normal) > 0);
    return scattered;
  return std::nullopt;
}

std::optional<ray> dielectric::scatter(
    const ray& r
  , const point& p
  , double t
  , bool front_face
  , const std::shared_ptr<material>& ptr_mat
  , const normed_vec& normal
  , const hit_record& rec
  , color& attenuation
  ) const
{
  attenuation = color(1.0, 1.0, 1.0);

  double refraction_ratio = front_face ? (1.0 / refraction_index) : refraction_index;
  double cos_incindence_angle = std::fmin(1.0, dot(r.direction, normal));
  double sin_incindence_angle = std::sqrt(1.0 - cos_incindence_angle * cos_incindence_angle);
  double sin_refracted_angle = refraction_ratio * sin_incindence_angle;
  double cos_refracted_angle = std::sqrt(1.0 - sin_refracted_angle * sin_refracted_angle);
  bool cannot_refract = sin_refracted_angle > 1.0;

  bool aux = (cannot_refract || random_double() > fresnel_reflectance(cos_incindence_angle, cos_refracted_angle, refraction_index));

  normed_vec direction = get_direction(aux, r.direction, normal, refraction_ratio);

  return ray(p,direction);
}

static normed_vec get_direction(bool b, normed_vec dir, normed_vec n, double ref_ratio)
{
  if (b)
    return reflect(dir,n);
  else
    return refract(dir,n,ref_ratio);
}

static double reflectance(double cos, double refraction_index)
{
  // Use Schlick's approximation for reflectance.
  double r0 = (1.0 - refraction_index) / (1.0 + refraction_index);
  r0 = r0*r0;
  return r0 + (1.0 - r0) * std::pow((1.0 - cos), 5);
}

static double fresnel_reflectance(double cos_incidence, double cos_refraction, double refraction_index)
{
  double parallel = (refraction_index * cos_incidence - cos_refraction)/ (refraction_index * cos_incidence + cos_refraction);
  parallel = parallel * parallel;

  double perp = (cos_refraction - refraction_index * cos_incidence)/ (cos_refraction + refraction_index * cos_incidence);
  perp = perp * perp;

  double factor = (parallel + perp)/2;
  return factor;
}