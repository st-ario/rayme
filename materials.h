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