#pragma once

#include "ray.h"

class material;

// TODO verify whether tuples or raw data members are faster for hit_properties and hit_record
class hit_properties
{
  public:
    bool front_face() const { return m_front_face; }
    const material* ptr_mat() const { return m_ptr_mat; }
    const normed_vec3 gnormal() const { return m_gnormal; }
    const normed_vec3 snormal() const { return m_snormal; }

    hit_properties( bool front_face
                  , const material* ptr_mat
                  , normed_vec3 gnormal
                  , normed_vec3 snormal
                  ) : m_front_face{front_face}
                    , m_ptr_mat{ptr_mat}
                    , m_gnormal{gnormal}
                    , m_snormal{snormal} {}

  private:
    bool m_front_face;
    const material* m_ptr_mat;
    normed_vec3 m_gnormal;
    normed_vec3 m_snormal;
};

class primitive;
class hit_record
{
  public:
    const primitive* what() const { return m_what; }
    float t() const { return m_t; }
    const vec3 p_error() const { return m_p_error; }

    hit_record( const primitive* what
              , float at
              , const vec3& p_error
              , const std::array<float,3>& uvw)
      : m_what{what}, m_t{at}, m_p_error{p_error}, uvw{uvw} {}

  private:
    const primitive* m_what;
    float m_t;
    vec3  m_p_error;
  public:
    const std::array<float,3> uvw;
};

// hit_check: type to say whether a primitive was hit, and, if so, to store its hit_record
using hit_check = std::optional<hit_record>;

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

class mesh;
class primitive : public element
{
  public:
    point centroid;
    std::shared_ptr<const mesh> parent_mesh;
  public:
    virtual hit_properties get_info(const ray& r,
      const std::array<float,3>& uvw)const = 0;
};

class bvh_node : public element
{
  public:
    std::shared_ptr<const element> left;
    std::shared_ptr<const element> right;

  public:
    bvh_node(const std::vector<std::shared_ptr<const primitive>>& primitives, size_t start, size_t end);

    virtual hit_check hit(const ray& r, float t_max) const override
    {
      if (!bounds.hit(r, t_max))
        return std::nullopt;

      hit_check hit_left = left->hit(r, t_max);
      hit_check hit_right = right->hit(r, (hit_left) ? hit_left->t() : t_max);

      if (hit_left)
      {
        if (hit_right)
        {
          if (hit_left->t() < hit_right->t())
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
    explicit bvh_tree(std::vector<std::shared_ptr<const primitive>>& primitives);

    virtual hit_check hit(const ray& r, float t_max) const override;

  private:
    std::shared_ptr<const element>
    recursive_build( std::vector<std::shared_ptr<const primitive>>& leaves
                   , size_t begin
                   , size_t end) const;
};