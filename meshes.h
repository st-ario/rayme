#pragma once

#include "bvh.h"
#include "materials.h"
#include "extern/glm/glm/vec4.hpp"

using vec4 = glm::vec4;

class triangle;

class mesh
{
  public:
    const size_t n_vertices;
    const size_t n_triangles;
    const std::vector<size_t> vertex_indices;
    std::vector<point> vertices;
    std::vector<normed_vec3> normals;
    std::vector<vec4> tangents;
    std::unique_ptr<const material> ptr_mat;

    static mesh* get_mesh( size_t n_vertices
                         , size_t n_triangles
                         , std::vector<size_t>&& vertex_indices
                         , std::vector<point>&& vertices
                         , std::unique_ptr<const material>&& ptr_mat
                         , std::vector<normed_vec3>&& normals = {}
                         , std::vector<vec4>&& tangents = {})
    {
      // meshes have static lifespan
      static std::vector<std::unique_ptr<mesh>> mesh_instances;

      std::unique_ptr<mesh> instance_ptr{
        new mesh( n_vertices, n_triangles
                , std::move(vertex_indices)
                , std::move(vertices)
                , std::move(ptr_mat)
                , std::move(normals)
                , std::move(tangents))};
      mesh_instances.emplace_back(std::move(instance_ptr));
      return mesh_instances.back().get();
    }

    virtual std::vector<std::unique_ptr<const triangle>> get_triangles() const
    {
      std::vector<std::unique_ptr<const triangle>> triangles;
      triangles.reserve(n_triangles);

      for (size_t i = 0; i < n_triangles; ++i)
        triangles.push_back(std::make_unique<const triangle>(this, i));

      return triangles;
    }

  protected:
    mesh( size_t n_vertices
        , size_t n_triangles
        , std::vector<size_t>&& vertex_indices
        , std::vector<point>&& vertices
        , std::unique_ptr<const material>&& ptr_mat
        , std::vector<normed_vec3>&& normals = {}
        , std::vector<vec4>&& tangents = {}) :
        n_vertices{n_vertices}, n_triangles{n_triangles},
        vertex_indices{vertex_indices}, vertices{vertices},
        normals{normals}, tangents{tangents}, ptr_mat{std::move(ptr_mat)} {}
};

// singleton for all the lights in the scene
class light;
class camera;
class world_lights
{
  friend class light;
  friend void parse_gltf( const std::string& filename
                        , std::vector<std::unique_ptr<const primitive>>& primitives
                        , std::unique_ptr<camera>& cam
                        , uint16_t image_height);

  public:
    world_lights(const world_lights&) = delete;
    world_lights(world_lights&&) = delete;
    world_lights& operator=(const world_lights&) = delete;
    world_lights& operator=(world_lights&&) = delete;

    static const std::vector<std::unique_ptr<light>>& lights() { return get().lights_vector; }

  private:
    world_lights() = default;

    static world_lights& get() { static world_lights static_instance; return static_instance; }
    void add(std::unique_ptr<light>&& l) { lights_vector.emplace_back(std::move(l)); }
    static void compute_light_areas();

    std::vector<std::unique_ptr<light>> lights_vector;
};

class light : public mesh
{
  friend class world_lights;

  public:
    // return a uniformly distributed random point on the surface of the mesh, and a pointer
    // to the primitive containing it; the three arguments are used for the rng
    std::pair<point, const triangle*>
      random_surface_point(uint16_t seed_x, uint16_t seed_y, uint16_t seed_z) const;

    float get_surface_area() const { return surface_area; }

    static light* get_light( size_t n_vertices
                           , size_t n_triangles
                           , std::vector<size_t>&& vertex_indices
                           , std::vector<point>&& vertices
                           , std::unique_ptr<const material>&& ptr_mat
                           , std::vector<normed_vec3>&& normals = {}
                           , std::vector<vec4>&& tangents = {})
    {
      std::unique_ptr<light> instance_ptr{
        new light( n_vertices, n_triangles
                 , std::move(vertex_indices)
                 , std::move(vertices)
                 , std::move(ptr_mat)
                 , std::move(normals)
                 , std::move(tangents))
      };
      world_lights::get().add(std::move(instance_ptr));
      return world_lights::lights().back().get();
    }

    virtual std::vector<std::unique_ptr<const triangle>> get_triangles() const override
    {
      std::vector<std::unique_ptr<const triangle>> triangles;
      triangles.reserve(n_triangles);
      ptr_triangles.reserve(n_triangles);

      for (size_t i = 0; i < n_triangles; ++i)
      {
        auto t{std::make_unique<const triangle>(this, i)};
        ptr_triangles.push_back(t.get());
        triangles.emplace_back(std::move(t));
      }

      return triangles;
    }

  private:
    using mesh::mesh;

    float surface_area{0.0f};
    std::vector<float> triangles_areas;
    std::vector<float> triangles_cdf;
    mutable std::vector<const triangle*> ptr_triangles;

    void compute_surface_area();
};

class triangle : public primitive
{
  public:
    triangle(const mesh* parent_mesh, size_t triangle_number)
    : number{triangle_number}
    {
      primitive::parent_mesh = parent_mesh;
      bounds = bounding_box();
      centroid = 0.5f * bounds.upper() + 0.5f * bounds.lower();
    }

    virtual hit_check hit(const ray& r, float t_max) const override;
    virtual hit_properties get_info(const ray& r,
      const std::array<float,3>& uvw) const override;

  private:
    const size_t number;
    aabb bounding_box() const;
};