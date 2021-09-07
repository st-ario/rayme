#pragma once

#include "bvh.h"
#include "extern/glm/glm/vec4.hpp"

using vec4 = glm::vec4;

class triangle;

class mesh : public std::enable_shared_from_this<mesh>
{
  public:
    const int n_vertices;
    const int n_triangles;
    std::vector<int> vertex_indices;
    std::vector<point> vertices;
    std::vector<normed_vec3> normals;
    std::vector<vec4> tangents;
    std::shared_ptr<material> ptr_mat;

    mesh( int n_vertices
        , int n_triangles
        , std::vector<int> vertex_indices
        , std::vector<point> vertices
        , std::shared_ptr<material> ptr_mat
        , std::vector<normed_vec3> normals = {}
        , std::vector<vec4> tangents = {}) :
        n_vertices{n_vertices}, n_triangles{n_triangles},
        vertex_indices{vertex_indices}, vertices{vertices},
        normals{normals}, tangents{tangents}, ptr_mat{ptr_mat} {}

    std::vector<std::shared_ptr<triangle>> get_triangles()
    {
      std::vector<std::shared_ptr<triangle>> triangles;
      triangles.reserve(n_triangles);

      for (int i = 0; i < n_triangles; ++i)
        triangles.push_back(std::make_shared<triangle>(shared_from_this(), i));

      return triangles;
    }
}; // class mesh

class triangle : public primitive
{
  private:
    std::shared_ptr<mesh> parent_mesh;
    const int number;

  public:
    triangle(const std::shared_ptr<mesh>& parent_mesh, int triangle_number)
    : parent_mesh{parent_mesh}, number{triangle_number}
    {
      bounds = bounding_box();
      centroid = 0.5f * bounds.max() + 0.5f * bounds.min();
    }

    virtual hit_check hit(const ray& r, float t_max) const override;
    virtual hit_record get_record(const ray& r, float at) const override;

  private:
    aabb bounding_box() const;
};

// for testing purposes
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

    hit_check hit(const ray& r, float t_max) const override;
    hit_record get_record(const ray& r, float at) const;

  private:
    aabb bounding_box() const;
};