#pragma once

#include "math.h"

struct hit_record;
class ray;
class normed_vec3;

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
    float roughness;

    metal(const color& a, float r) : albedo{a}, roughness{r < 1 ? r : 1} {};

    virtual std::optional<ray> scatter(
        const ray& r
      , const hit_record& rec
      , color& attenuation
      ) const override;
};

class dielectric : public material
{
  public:
    float refraction_index;

    dielectric(float ri) : refraction_index{ri} {};

    virtual std::optional<ray> scatter(
        const ray& r
      , const hit_record& rec
      , color& attenuation
      ) const override;
};

static normed_vec3 get_direction(bool b, normed_vec3 dir, normed_vec3 n, float ref_ratio);
static float reflectance(float cos, float refraction_index);
static float fresnel_reflectance(float cos_incidence, float cos_refraction, float refraction_index);