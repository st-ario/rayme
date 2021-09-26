#pragma once

#include "materials.h"

class ray;

class diffuse_brdf
{
  public:
    diffuse_brdf(const std::shared_ptr<const material>& ptr_mat) : ptr_mat{ptr_mat} {};

    float pdf(const normed_vec3& normal, const normed_vec3& scattered);
    color eval();
    std::pair<color,normed_vec3> sample(const point& at, const normed_vec3& normal);

  private:
    const std::shared_ptr<const material> ptr_mat;
};