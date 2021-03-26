#pragma once
#include "my_vectors.h"
#include "ray.h"

#include <cmath>
#include <vector>
#include <memory>

class material; // defined in materials.h

struct hit_record
{
  point p;
  double t;
  bool front_face;
  std::shared_ptr<material> ptr_mat;
  normed_vec normal;

  inline void set_face_normal(const ray& r, const vec& nonunital_outward_normal)
  {
    front_face = dot(r.direction, nonunital_outward_normal) < 0;
    normal = front_face ? unit(nonunital_outward_normal) : - unit(nonunital_outward_normal);
  }
};

class element
{
  public:
    virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const = 0;
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

    virtual bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const override;
};

// TODO change things in such a way that normals are computed only for the closest hit
bool scene::hit(const ray& r, double t_min, double t_max, hit_record& rec) const
{
    //hit_record temp_rec{rec};
    bool hit_anything = false;
    auto closest_so_far = t_max;

    for (const auto& object : objects) {
        if (object->hit(r, t_min, closest_so_far, temp_rec)) {
            hit_anything = true;
            closest_so_far = temp_rec.t;
            rec = temp_rec;
        }
    }

    return hit_anything;
}


class sphere : public element
{
  public:
    point center;
    double radius;
    std::shared_ptr<material> ptr_mat;
    
    sphere() : center{point(0,0,0)}, radius{0} {};
    sphere(point c, double r, std::shared_ptr<material> m) : center{c}, radius{r}, ptr_mat{m} {};

    bool hit(const ray& r, double t_min, double t_max, hit_record& rec) const override
    {
      vec center_to_origin = r.origin - center;
      double b_halved = dot(center_to_origin,r.direction);
      double c = center_to_origin.norm_squared() - radius*radius;
      double discriminant = b_halved*b_halved - c;
      if (discriminant < 0)
        return false;

      double sqrtdel = std::sqrt(discriminant);
      double root = -b_halved - std::sqrt(discriminant);
      if (root < t_min || root > t_max)
        return false;

      rec.t = root;
      rec.p = r.at(rec.t);
      //not allowing negative radius trick:
      //vec nonunital_outward_normal = rec.p - center; 
      //rec.set_face_normal(r, nonunital_outward_normal);
      vec outward_normal = (rec.p - center)/radius;
      rec.set_face_normal(r, outward_normal);
      rec.ptr_mat = ptr_mat;

      return true;
    }
};