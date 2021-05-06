#pragma once
#include "my_vectors.h"
#include "ray.h"

#include <cmath>
#include <vector>
#include <memory>
#include <optional>

class material; // defined in materials.h

struct hit_record
{
  point p;
  float t;
  bool front_face;
  std::shared_ptr<material> ptr_mat;
  normed_vec normal;

  hit_record( point pt
            , float at
            , bool front
            , std::shared_ptr<material> p_mat
            , normed_vec n
            ) : p{pt}, t{at}, front_face{front}, ptr_mat{p_mat}, normal{n} {}
};

class element
{
  public:
    virtual std::optional<hit_record> hit(const ray& r, float t_min, float t_max) const = 0;
};

// TODO restructure this class
class scene : public element
{
  public:
    std::vector<std::shared_ptr<element>> objects;

    scene() {}
    scene(std::shared_ptr<element> obj) { add(obj); }

    void clear() { objects.clear(); }
    void add(std::shared_ptr<element> obj) { objects.push_back(obj); }

    // TODO change things in such a way that normals are computed only for the closest hit
    virtual std::optional<hit_record> hit(const ray& r, float t_min, float t_max) const override
    {
        std::optional<hit_record> record{};
        float closest_so_far = t_max;
    
        for (const auto& object : objects) {
            auto partial_record = object->hit(r, t_min, closest_so_far);
            if (partial_record) {
                closest_so_far = partial_record.value().t;
                record = partial_record;
            }
        }
    
        return record;
    }
};


class sphere : public element
{
  public:
    point center;
    float radius;
    std::shared_ptr<material> ptr_mat;
    
    sphere() : center{point(0,0,0)}, radius{0} {};
    sphere(point c, float r, std::shared_ptr<material> m) : center{c}, radius{r}, ptr_mat{m} {};

    std::optional<hit_record> hit(const ray& r, float t_min, float t_max) const override
    {
      vec center_to_origin = r.origin - center;
      float b_halved = dot(center_to_origin,r.direction);
      float c = center_to_origin.norm_squared() - radius*radius;
      float discriminant = b_halved*b_halved - c;
      if (discriminant < 0)
        return std::nullopt;

      float sqrtdel = std::sqrt(discriminant);
      float root = -b_halved - std::sqrt(discriminant);
      if (root < t_min || root > t_max)
        return std::nullopt;

      point p = r.at(root);
      vec nonunital_candidate_normal = p - center;

      bool front_face = dot(r.direction, nonunital_candidate_normal) < 0;
      normed_vec normal = front_face ? unit(nonunital_candidate_normal) : - unit(nonunital_candidate_normal);

      hit_record record = hit_record(p, root, front_face, ptr_mat, normal);

      return record;
    }
};