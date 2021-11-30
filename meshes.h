#pragma once

#include "materials.h"
#include "ray.h"
#include "extern/glm/glm/vec4.hpp"

using vec4 = glm::vec4;

class hit_properties
{
  public:
    const material* ptr_mat() const { return m_ptr_mat; }
    const point where() const {return x; }
    const normed_vec3 gnormal() const { return m_gnormal; }
    const normed_vec3 snormal() const { return m_snormal; }

    hit_properties( const material* ptr_mat
                  , point where
                  , normed_vec3 gnormal
                  , normed_vec3 snormal
                  ) : m_ptr_mat{ptr_mat}
                    , x{where}
                    , m_gnormal{gnormal}
                    , m_snormal{snormal} {}

  private:
    const material* m_ptr_mat;
    point x;
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
    std::array<float,3> uvw;
};

// hit_check: type to say whether a primitive was hit, and, if so, to store its hit_record
using hit_check = std::optional<hit_record>;

class aabb
{
  public:
    aabb(const point& min, const point& max) : bounds{min, max} {}

    const point& lower() const { return bounds[0]; }
    const point& upper() const { return bounds[1]; }

    std::optional<float> hit(const ray& r, float tmax) const
    {
      // Ize, "Robust BVH Ray Traversal", revised version

      float tmin{0.0f};
      float txmin, txmax, tymin, tymax, tzmin, tzmax;
      txmin = (bounds[  r.sign[0]].x -r.origin.x) * r.invD.x;
      txmax = (bounds[!(r.sign[0])].x-r.origin.x) * r.invD.x;
      tymin = (bounds[  r.sign[1]].y -r.origin.y) * r.invD.y;
      tymax = (bounds[!(r.sign[1])].y-r.origin.y) * r.invD.y;
      tzmin = (bounds[  r.sign[2]].z -r.origin.z) * r.invD.z;
      tzmax = (bounds[!(r.sign[2])].z-r.origin.z) * r.invD.z;
      tmin = max(tzmin, max(tymin, max(txmin, tmin)));
      tmax = min(tzmax, min(tymax, min(txmax, tmax)));
      tmax *= 1.00000024f;
      // no hit
      if (tmin > tmax)
        return std::nullopt;
      // ray inside box
      if (std::signbit(tmin) != std::signbit(tmax))
        return tmax;
      // ray outside
      return tmin;
    }

  private:
    std::array<point,2> bounds;
};

class bounded
{
  public:
    aabb bounds{point{infinity, infinity, infinity}, point{-infinity, -infinity, -infinity}};
    bool is_primitive{false};
};

class mesh;
class primitive : public bounded
{
  public:
    point centroid;
    const mesh* parent_mesh;
  public:
    primitive() { is_primitive = true; }
    virtual hit_check hit(const ray& r, float t_max) const = 0;
    virtual hit_properties get_info(const ray& r, const std::array<float,3>& uvw)const = 0;
};

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

class triangle : public primitive
{
  public:
    triangle(const mesh* parent_mesh, size_t triangle_number)
    : number{triangle_number}
    {
      primitive::parent_mesh = parent_mesh;
      bounds = bounding_box();
      centroid = 0.5f * bounds.upper() + 0.5f * bounds.lower();
      const point& p0{parent_mesh->vertices[parent_mesh->vertex_indices[3*number]]};
      const point& p1{parent_mesh->vertices[parent_mesh->vertex_indices[3*number+1]]};
      const point& p2{parent_mesh->vertices[parent_mesh->vertex_indices[3*number+2]]};
      e1 = p1-p0;
      e2 = p2-p0;
      nu_gnormal = cross(e1,e2);
    }

    virtual hit_check hit(const ray& r, float t_max) const override;
    virtual hit_properties get_info(const ray& r,
      const std::array<float,3>& uvw) const override;

  private:
    const size_t number;
    // sides adjacent to p0
    vec3 e1;
    vec3 e2;
    // nonunital geometric normal
    vec3 nu_gnormal;
    aabb bounding_box() const;
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
      random_surface_point() const;

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