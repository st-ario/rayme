#pragma once

#include "ray.h"
#include "my_vectors.h"
#include "elements.h"

#include <optional>

struct hit_record;

class material
{
  public:
    virtual std::optional<ray> scatter(
        const ray& r
      , const hit_record& rec
      , color& attenuation
      ) const = 0;
};

class lambertian : public material
{
  public:
    color albedo;

    lambertian(const color& a) : albedo{a} {}

    virtual std::optional<ray> scatter(
        const ray& r
      , const hit_record& rec
      , color& attenuation
      ) const override;
};

class metal : public material
{
  public:
    color albedo;
    double roughness;

    metal(const color& a, double r) : albedo{a}, roughness{r < 1 ? r : 1} {};

    virtual std::optional<ray> scatter(
        const ray& r
      , const hit_record& rec
      , color& attenuation
      ) const override;
};

class dielectric : public material
{
  public:
    double refraction_index;

    dielectric(double ri) : refraction_index{ri} {};

    virtual std::optional<ray> scatter(
        const ray& r
      , const hit_record& rec
      , color& attenuation
      ) const override;
};

static normed_vec get_direction(bool b, normed_vec dir, normed_vec n, double ref_ratio);
static double reflectance(double cos, double refraction_index);
static double fresnel_reflectance(double cos_incidence, double cos_refraction, double refraction_index);