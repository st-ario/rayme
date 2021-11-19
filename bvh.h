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
    aabb(const point& min, const point& max) : bounds{min, max} {}

    const point& lower() const { return bounds[0]; }
    const point& upper() const { return bounds[1]; }

    bool hit(const ray& r, float tmax) const
    {
      // Ize, "Robust BVH Ray Traversal", revised version

      float tmin{0.0f};
      float txmin, txmax, tymin, tymax, tzmin, tzmax;
      txmin = (bounds[ r.sign[0]].x-r.origin.x)  * r.invD.x;
      txmax = (bounds[1-r.sign[0]].x-r.origin.x) * r.invD_pad.x;
      tymin = (bounds[ r.sign[1]].y-r.origin.y)  * r.invD.y;
      tymax = (bounds[1-r.sign[1]].y-r.origin.y) * r.invD_pad.y;
      tzmin = (bounds[ r.sign[2]].z-r.origin.z)  * r.invD.z;
      tzmax = (bounds[1-r.sign[2]].z-r.origin.z) * r.invD_pad.z;
      tmin = max(tzmin, max(tymin, max(txmin, tmin)));
      tmax = min(tzmax, min(tymax, min(txmax, tmax)));
      tmax *= 1.00000024f;
      return tmin <= tmax;

    }

  private:
    std::array<point,2> bounds;
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
    const mesh* parent_mesh;
  public:
    virtual hit_properties get_info(const ray& r, const std::array<float,3>& uvw)const = 0;
};

class bvh_node : public element
{
  public:
    std::unique_ptr<const element> left;
    std::unique_ptr<const element> right;

  public:
    bvh_node(const std::vector<std::unique_ptr<const primitive>>& primitives, size_t start, size_t end);

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
    std::unique_ptr<const element> root;

  public:
    explicit bvh_tree(std::vector<std::unique_ptr<const primitive>>& primitives);

    virtual hit_check hit(const ray& r, float t_max) const override;

  private:
    std::unique_ptr<const element>
    recursive_build( std::vector<std::unique_ptr<const primitive>>& leaves
                   , size_t begin
                   , size_t end) const;
};