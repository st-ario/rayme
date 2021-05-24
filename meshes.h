#pragma once

#include <vector>
#include "my_vectors.h"

#include "elements.h"

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

    virtual aabb bounding_box() const override
    {
      float padding = 0.001f;
      const point& p0 = parent_mesh->vertices[parent_mesh->vertex_indices[3*number]];
      const point& p1 = parent_mesh->vertices[parent_mesh->vertex_indices[3*number+1]];
      const point& p2 = parent_mesh->vertices[parent_mesh->vertex_indices[3*number+2]];

      float min_x = fminf(p0.x(), fminf(p1.x(), p2.x())) - padding;
      float min_y = fminf(p0.y(), fminf(p1.y(), p2.y())) - padding;
      float min_z = fminf(p0.z(), fminf(p1.z(), p2.z())) - padding;

      float max_x = fmaxf(p0.x(), fmaxf(p1.x(), p2.x())) + padding;
      float max_y = fmaxf(p0.y(), fmaxf(p1.y(), p2.y())) + padding;
      float max_z = fmaxf(p0.z(), fmaxf(p1.z(), p2.z())) + padding;

      return aabb(vec3(min_x, min_y, min_z), vec3(max_x, max_y, max_z));
    }

    virtual hit_check hit(const ray& r, float t_max) const override
    {
      // adapted from pbrt v3
      const point& p0 = parent_mesh->vertices[parent_mesh->vertex_indices[3*number]];
      const point& p1 = parent_mesh->vertices[parent_mesh->vertex_indices[3*number+1]];
      const point& p2 = parent_mesh->vertices[parent_mesh->vertex_indices[3*number+2]];


      // translate vertices based on ray origin
      point p0t = p0 - r.origin;
      point p1t = p1 - r.origin;
      point p2t = p2 - r.origin;

      // Permute components of triangle vertices and ray direction
      int kz = max_dimension(abs(r.direction));
      int kx = kz + 1;
      if (kx == 3) kx = 0;
      int ky = kx + 1;
      if (ky == 3) ky = 0;
      normed_vec3 d = permute(r.direction, kx, ky, kz);
      p0t = permute(p0t, kx, ky, kz);
      p1t = permute(p1t, kx, ky, kz);
      p2t = permute(p2t, kx, ky, kz);

      // shear to align ray direction with the Y axis
      // TODO memoize these values in the ray class
      float Sx = - d.x() / d.z();
      float Sy = - d.y() / d.z();
      float Sz =   1.0f / d.z();
      p0t.x() += Sx * p0t.z();
      p0t.y() += Sy * p0t.z();
      p1t.x() += Sx * p1t.z();
      p1t.y() += Sy * p1t.z();
      p2t.x() += Sx * p2t.z();
      p2t.y() += Sy * p2t.z();

      // Compute edge function coefficients
      float e0 = p1t.x() * p2t.y() - p1t.y() * p2t.x();
      float e1 = p2t.x() * p0t.y() - p2t.y() * p0t.x();
      float e2 = p0t.x() * p1t.y() - p0t.y() * p1t.x();

      // Fall back to double precision test at triangle edges
      if  (e0 == 0.0f || e1 == 0.0f || e2 == 0.0f)
      {
          double p2txp1ty = (double)p2t.x() * (double)p1t.y();
          double p2typ1tx = (double)p2t.y() * (double)p1t.x();
          e0 = (float)(p2typ1tx - p2txp1ty);
          double p0txp2ty = (double)p0t.x() * (double)p2t.y();
          double p0typ2tx = (double)p0t.y() * (double)p2t.x();
          e1 = (float)(p0typ2tx - p0txp2ty);
          double p1txp0ty = (double)p1t.x() * (double)p0t.y();
          double p1typ0tx = (double)p1t.y() * (double)p0t.x();
          e2 = (float)(p1typ0tx - p1txp0ty);
      }

      // Perform triangle edge and determinant tests
      if ((e0 < 0 || e1 < 0 || e2 < 0) && (e0 > 0 || e1 > 0 || e2 > 0))
        return std::nullopt;
      float det = e0 + e1 + e2;
      if (det == 0) return std::nullopt;

      // Compute scaled hit distance to triangle and test against ray $t$ range
      p0t.z() *= Sz;
      p1t.z() *= Sz;
      p2t.z() *= Sz;
      float tScaled = e0 * p0t.z() + e1 * p1t.z() + e2 * p2t.z();
      if (det < 0 && (tScaled >= 0 || tScaled < t_max * det))
          return std::nullopt;
      else if (det > 0 && (tScaled <= 0 || tScaled > t_max * det))
          return std::nullopt;

      // Compute barycentric coordinates and $t$ value for triangle intersection
      float invDet = 1 / det;
      float b0 = e0 * invDet;
      float b1 = e1 * invDet;
      float b2 = e2 * invDet;
      float t = tScaled * invDet;

      // Compute $\delta_z$ term for triangle $t$ error bounds
      float maxZt = max_component(abs(vec3(p0t.z(), p1t.z(), p2t.z())));
      float deltaZ = gamma_bound(3) * maxZt;

      // Compute $\delta_x$ and $\delta_y$ terms for triangle $t$ error bounds
      float maxXt = max_component(abs(vec3(p0t.x(), p1t.x(), p2t.x())));
      float maxYt = max_component(abs(vec3(p0t.y(), p1t.y(), p2t.y())));
      float deltaX = gamma_bound(5) * (maxXt + maxZt);
      float deltaY = gamma_bound(5) * (maxYt + maxZt);

      // Compute $\delta_e$ term for triangle $t$ error bounds
      float deltaE = 2 * (gamma_bound(2) * maxXt * maxYt + deltaY * maxXt + deltaX * maxYt);

      // Compute $\delta_t$ term for triangle $t$ error bounds and check _t_
      float maxE = max_component(abs(vec3(e0, e1, e2)));
      float deltaT = 3 * (gamma_bound(3) * maxE * maxZt +
                     deltaE * maxZt + deltaZ * maxE) * std::abs(invDet);
      if (t <= deltaT)
        return std::nullopt;

      /* error bounds, reintroduce later
      // Compute error bounds for triangle intersection
      float xAbsSum = (std::abs(b0 * p0.x()) + std::abs(b1 * p1.x()) + std::abs(b2 * p2.x()));
      float yAbsSum = (std::abs(b0 * p0.y()) + std::abs(b1 * p1.y()) + std::abs(b2 * p2.y()));
      float zAbsSum = (std::abs(b0 * p0.z()) + std::abs(b1 * p1.z()) + std::abs(b2 * p2.z()));
      vec3 pError = gamma_bound(7) * vec3(xAbsSum, yAbsSum, zAbsSum);
      */

      return std::make_pair(this, t);
    }

    virtual hit_record get_record(const ray& r, float at) const override
    {
      const point& p0 = parent_mesh->vertices[parent_mesh->vertex_indices[3*number]];
      const point& p1 = parent_mesh->vertices[parent_mesh->vertex_indices[3*number+1]];
      const point& p2 = parent_mesh->vertices[parent_mesh->vertex_indices[3*number+2]];

      point p = r.at(at);
      vec3 nonunital_candidate_normal = cross(p1-p0, p2-p0);

      bool front_face = dot(r.direction, nonunital_candidate_normal) < 0;

      normed_vec3 normal = front_face ? unit(nonunital_candidate_normal) : - unit(nonunital_candidate_normal);

      return hit_record(at, front_face, parent_mesh->ptr_mat, normal);
    }
}; // class triangle