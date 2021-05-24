#pragma once
#include "ray.h"

#include <cmath>
#include <vector>
#include <memory>
#include <optional>

class material; // defined in materials.h

struct hit_record
{
  float t;
  bool front_face;
  std::shared_ptr<material> ptr_mat;
  normed_vec3 normal;

  hit_record( float at
            , bool front_face
            , std::shared_ptr<material> ptr_mat
            , normed_vec3 n
            ) : t{at}, front_face{front_face}, ptr_mat{ptr_mat}, normal{n} {}
};


class aabb
{
  public:
    aabb(const point& min, const point& max) : minimum{min}, maximum{max} {}

    const point& min() const { return minimum; }
    const point& max() const { return maximum; }

    bool hit(const ray& r, float t_max) const
    {
      float t_min = 0.0f;

      for (int a = 0; a < 3; a++)
      {
        if (r.direction[a] != 0.0f)
        {
          float invD = 1.0f / r.direction[a];

          // smaller coordinate
          float t0 = (minimum[a] - r.origin[a]) * invD;
          // bigger coordinate
          float t1 = (maximum[a] - r.origin[a]) * invD;

          // if the direction is negative, the order of the planes is inverted
          if (invD < 0.0f)
            std::swap(t0, t1);

          t_min = t0 > t_min ? t0 : t_min;
          t_max = t1 < t_max ? t1 : t_max;
          if (t_max <= t_min)
            return false;
        } else {
          if (r.origin[a] == minimum[a] || r.origin[a] == maximum[a])
          {
            // the ray lies on one of the planes bounding the slab
            return false;
          }
          // otherwise, use signed infinities to deal with the comparisons

          float invD = r.direction[a] * infinity;
          float t0 = (minimum[a] - r.origin[a]) * invD;
          float t1 = (maximum[a] - r.origin[a]) * invD;

          if (invD < 0.0f)
            std::swap(t0, t1);

          t_min = t0 > t_min ? t0 : t_min;
          t_max = t1 < t_max ? t1 : t_max;
          if (t_max <= t_min)
            return false;
        }
      }
      return true;
    }

  private:
    point minimum;
    point maximum;
};

class primitive;
// hit_check: type to say whether a primitive was hit, and if so, which one and at what ray coordinate
using hit_check = std::optional<std::pair<const primitive*, float>>;

// elements: primitives and nodes of the BVH
class element
{
  public:
    aabb bounds{point{infinity, infinity, infinity}, point{-infinity, -infinity, -infinity}};
  public:
    // check whether a primitive contained in the element (or the element itself, in case _this_
    // points to a primitive) is hit
    virtual hit_check hit(const ray& r, float t_max) const = 0;

    virtual aabb bounding_box() const = 0;
};

class primitive : public element
{
  public:
    point centroid;
  public:
    virtual hit_record get_record(const ray& r, float at) const = 0;
};

class sphere : public primitive
{
  public:
    point center;
    float radius;
    std::shared_ptr<material> ptr_mat;

    sphere() : center{point(0,0,0)}, radius{0}
    {
      bounds = bounding_box();
      centroid = center;
    };

    sphere(point c, float r, std::shared_ptr<material> m) : center{c}, radius{r}, ptr_mat{m}
    {
      bounds = bounding_box();
      centroid = center;
    };

    hit_check hit(const ray& r, float t_max) const override
    {
      vec3 center_to_origin = r.origin - center;
      float b_halved = dot(center_to_origin,r.direction);
      float c = center_to_origin.norm_squared() - radius*radius;
      float discriminant = b_halved*b_halved - c;
      if (discriminant < 0)
        return std::nullopt;

      float sqrtdel = std::sqrt(discriminant);
      float root = -b_halved - std::sqrt(discriminant);
      if (root < 0.0f || root > t_max)
        return std::nullopt;

      return std::make_pair(this, root);
    }

    hit_record get_record(const ray& r, float at) const
    {
      point p = r.at(at);
      vec3 nonunital_candidate_normal = p - center;
      bool front_face = dot(r.direction, nonunital_candidate_normal) < 0;
      normed_vec3 normal = front_face ? unit(nonunital_candidate_normal) : - unit(nonunital_candidate_normal);
      return hit_record(at, front_face, ptr_mat, normal);
    }

    aabb bounding_box() const
    {
      float padding = 0.001f;
      float r = radius + padding;
      return aabb{ center - vec3(r, r, r)
                 , center + vec3(r, r, r)};
    }
};

class bvh_node : public element
{
  public:
    std::shared_ptr<element> left;
    std::shared_ptr<element> right;

  public:
    bvh_node(const std::vector<std::shared_ptr<primitive>>& primitives, size_t start, size_t end);

    virtual hit_check hit(const ray& r, float t_max) const override
    {
      if (!bounds.hit(r, t_max))
        return std::nullopt;

      hit_check hit_left = left->hit(r, t_max);
      hit_check hit_right = right->hit(r, (hit_left) ? hit_left->second : t_max);

      if (hit_left)
      {
        if (hit_right)
        {
          if (hit_left->second < hit_right->second)
          {
            return *hit_left;
          } else {
            return *hit_right;
          }
        }
        return *hit_left;
      } else if (hit_right) {
        return *hit_right;
      }

      return std::nullopt;
    }

    virtual aabb bounding_box() const override { return bounds; }
};

class bvh_tree : public element
{
  private:
    std::shared_ptr<element> root;
    std::vector<std::shared_ptr<primitive>> primitives;

  public:
    bvh_tree(std::vector<std::shared_ptr<primitive>>&& primitives);

    virtual hit_check hit(const ray& r, float t_max) const override;
    virtual aabb bounding_box() const override;

  private:
    std::shared_ptr<element> recursive_build(std::vector<std::shared_ptr<primitive>>& leaves, size_t begin, size_t end, unsigned short int axis) const;
};