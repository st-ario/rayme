#pragma once

#include "bvh.h"
#include "materials.h"
#include "extern/glm/glm/vec4.hpp"

using vec4 = glm::vec4;

class triangle;

class mesh : public std::enable_shared_from_this<mesh>
{
  public:
    const size_t n_vertices;
    const size_t n_triangles;
    const std::vector<size_t> vertex_indices;
    std::vector<point> vertices;
    std::vector<normed_vec3> normals;
    std::vector<vec4> tangents;
    std::unique_ptr<const material> ptr_mat;

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

    virtual std::vector<std::unique_ptr<const triangle>> get_triangles() const
    {
      std::vector<std::unique_ptr<const triangle>> triangles;
      triangles.reserve(n_triangles);

      for (size_t i = 0; i < n_triangles; ++i)
        triangles.push_back(std::make_unique<const triangle>(shared_from_this(), i));

      return triangles;
    }
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

    static const std::vector<light*>& lights() { return get().lights_vector; }

  private:
    world_lights() = default;

    static world_lights& get() { static world_lights static_instance; return static_instance; }
    void add(light* l) { lights_vector.push_back(l); }
    static void compute_light_areas();

    std::vector<light*> lights_vector;
};

class light : public mesh
{
  friend class world_lights;
  public:
    light( size_t n_vertices
         , size_t n_triangles
         , std::vector<size_t>&& vertex_indices
         , std::vector<point>&& vertices
         , std::unique_ptr<const material>&& ptr_mat
         , std::vector<normed_vec3>&& normals = {}
         , std::vector<vec4>&& tangents = {})
         : mesh{n_vertices,n_triangles,std::move(vertex_indices),std::move(vertices),std::move(ptr_mat),std::move(normals),std::move(tangents)}
         {
           world_lights::get().add(this);
         }

    // return a uniformly distributed random point on the surface of the mesh, and a pointer
    // to the primitive containing it; the three arguments are used for the rng
    std::pair<point, const triangle*>
      random_surface_point(uint16_t seed_x, uint16_t seed_y, uint16_t seed_z) const;

    float get_surface_area() const { return surface_area; }

    virtual std::vector<std::unique_ptr<const triangle>> get_triangles() const override
    {
      std::vector<std::unique_ptr<const triangle>> triangles;
      triangles.reserve(n_triangles);
      ptr_triangles.reserve(n_triangles);

      for (size_t i = 0; i < n_triangles; ++i)
      {
        auto t{std::make_unique<const triangle>(shared_from_this(), i)};
        ptr_triangles.push_back(t.get());
        triangles.emplace_back(std::move(t));
      }

      return triangles;
    }

  private:
    float surface_area{0.0f};
    std::vector<float> triangles_areas;
    std::vector<float> triangles_cdf;
    mutable std::vector<const triangle*> ptr_triangles;

    void compute_surface_area();
};

class triangle : public primitive
{
  private:
    const size_t number;

  public:
    triangle(const std::shared_ptr<const mesh>& parent_mesh, size_t triangle_number)
    : number{triangle_number}
    {
      primitive::parent_mesh = parent_mesh;
      bounds = bounding_box();
      centroid = 0.5f * bounds.max() + 0.5f * bounds.min();
    }

    virtual hit_check hit(const ray& r, float t_max) const override;
    virtual hit_properties get_info(const ray& r,
      const std::array<float,3>& uvw) const override;

  private:
    aabb bounding_box() const;
};