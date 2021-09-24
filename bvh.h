#pragma once

#include "ray.h"

class material; // defined in materials.h

struct hit_record
{
  float t;
  bool front_face;
  std::shared_ptr<const material> ptr_mat;
  normed_vec3 normal;

  hit_record( float at
            , bool front_face
            , std::shared_ptr<const material> ptr_mat
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
        // if for any i r.direction[i] is 0, invD[i] is infinity
        // in order to avoid 0*infinity, we need to handle the following case separately
        // conservatively: true (false negatives would be bad, false positives have little
        // impact and will be handled correctly by some subsequent hit check)
        if (r.origin[a] == 0.0f && (r.origin[a] == minimum[a] || r.origin[a] == maximum[a]))
          return false;

        float t_small = (minimum[a] - r.origin[a]) * r.invD[a];
        float t_big = (maximum[a] - r.origin[a]) * r.invD[a];

        // if the direction is negative, the order of the planes is inverted
        if (r.invD[a] < 0.0f)
          std::swap(t_small, t_big);

        // float rounding compensation
        t_big *= 1.0f + 2.0f * gamma_bound(3.0);

        t_min = t_small > t_min ? t_small : t_min;
        t_max = t_big < t_max ? t_big : t_max;
        if (t_min > t_max)
          return false;
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
};

class primitive : public element
{
  public:
    point centroid;
  public:
    virtual hit_record get_record(const ray& r, float at) const = 0;
};

class bvh_node : public element
{
  public:
    std::shared_ptr<const element> left;
    std::shared_ptr<const element> right;

  public:
    bvh_node(std::vector<std::shared_ptr<const primitive>>& primitives, size_t start, size_t end);

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
};

class bvh_tree : public element
{
  private:
    std::shared_ptr<const element> root;

  public:
    bvh_tree(std::vector<std::shared_ptr<const primitive>>& primitives);

    virtual hit_check hit(const ray& r, float t_max) const override;

  private:
    std::shared_ptr<const element> recursive_build(std::vector<std::shared_ptr<const primitive>>& leaves, size_t begin, size_t end) const;
};