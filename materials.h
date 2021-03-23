#pragma once

#include "ray.h"
#include "my_vectors.h"
#include "elements.h"

struct hit_record;

class material
{
  public:
    virtual bool scatter(
        const ray& r
      , const hit_record& rec
      , color& attenuation
      , ray& scattered
      ) const = 0;
};

class lambertian : public material
{
  public:
    color albedo;

    lambertian(const color& a) : albedo{a} {}

    virtual bool scatter(
        const ray& r
      , const hit_record& rec
      , color& attenuation
      , ray& scattered
      ) const override
      {
        vec nonunital_scatter_direction = rec.get_normal() + vec::random_unit();
        if (nonunital_scatter_direction.near_zero())
        {
          scattered = ray(rec.p, rec.get_normal());
        } else {
          scattered = ray(rec.p, nonunital_scatter_direction);
        }
        attenuation = albedo;
        return true;
      }

      // Alternatively: scatter with probability p and have attenuation be given by albedo/p
};

class metal : public material
{
  public:
    color albedo;
    double roughness;

    metal(const color& a, double r) : albedo{a}, roughness{r < 1 ? r : 1} {};

    virtual bool scatter(
        const ray& r
      , const hit_record& rec
      , color& attenuation
      , ray& scattered
      ) const override
      {
        vec reflected = reflect(r.direction(),rec.get_normal());
        scattered = ray(rec.p, reflected + roughness*vec::random_in_unit_sphere());
        // using random_unit() might be better
        //scattered = ray(rec.p, reflected + roughness*vec::random_unit());
        attenuation = albedo;
        return (dot(scattered.direction(), rec.get_normal()) > 0);
      }
};

class dielectric : public material
{
  public:
    double refraction_index;

    dielectric(double ri) : refraction_index{ri} {};

    virtual bool scatter(
        const ray& r
      , const hit_record& rec
      , color& attenuation
      , ray& scattered
      ) const override
      {
        attenuation = color(1.0, 1.0, 1.0);

        double refraction_ratio = rec.front_face ? (1.0 / refraction_index) : refraction_index;
        double cos_incindence_angle = std::fmin(1.0, dot(r.direction(),rec.get_normal()));
        double sin_incindence_angle = std::sqrt(1.0 - cos_incindence_angle * cos_incindence_angle);
        double sin_refracted_angle = refraction_ratio * sin_incindence_angle;
        double cos_refracted_angle = std::sqrt(1.0 - sin_refracted_angle * sin_refracted_angle);
        bool cannot_refract = sin_refracted_angle > 1.0;

        vec direction;

        if (cannot_refract || random_double() > fresnel_reflectance(cos_incindence_angle, cos_refracted_angle, refraction_index))
        //if (cannot_refract || random_double() > reflectance(cos_incindence_angle, refraction_index))
          direction = reflect(r.direction(), rec.get_normal());
        else
          direction = refract(r.direction(), rec.get_normal(), refraction_ratio);

        scattered = ray(rec.p, direction);
        return true;
      }
  private:
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
};