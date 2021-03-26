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
      ) const override;
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
      ) const override;
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
      ) const override;
};