#include "meshes.h"
#include "extern/glm/glm/mat3x3.hpp"
#include "extern/glm/glm/gtx/norm.hpp"

using mat3 = glm::mat3;

hit_properties triangle::get_info(const ray& r, const std::array<float,3>& uvw) const
{
  normed_vec3 gnormal{normed_vec3::absolute_y()};
  normed_vec3 snormal{normed_vec3::absolute_y()};

  const point& p0 = parent_mesh->vertices[parent_mesh->vertex_indices[3*number]];
  const point& p1 = parent_mesh->vertices[parent_mesh->vertex_indices[3*number+1]];
  const point& p2 = parent_mesh->vertices[parent_mesh->vertex_indices[3*number+2]];

  vec3 nonunital_candidate_normal{cross(p1-p0, p2-p0)};

  bool front_face{dot(r.direction, nonunital_candidate_normal) < 0};
  gnormal = front_face ? unit(nonunital_candidate_normal) : - unit(nonunital_candidate_normal);

  if (parent_mesh->normals.empty())
  {
    snormal = gnormal;
  } else {
    nonunital_candidate_normal =
      uvw[0] * parent_mesh->normals[parent_mesh->vertex_indices[3*number]] +
      uvw[1] * parent_mesh->normals[parent_mesh->vertex_indices[3*number+1]] +
      uvw[2] * parent_mesh->normals[parent_mesh->vertex_indices[3*number+2]];


    snormal = front_face ? unit(nonunital_candidate_normal) : - unit(nonunital_candidate_normal);
  }

  return hit_properties(parent_mesh->ptr_mat.get(), gnormal, snormal);
}

aabb triangle::bounding_box() const
{
  float padding = 0.001f;
  const point& p0 = parent_mesh->vertices[parent_mesh->vertex_indices[3*number]];
  const point& p1 = parent_mesh->vertices[parent_mesh->vertex_indices[3*number+1]];
  const point& p2 = parent_mesh->vertices[parent_mesh->vertex_indices[3*number+2]];

  float min_x = fminf(p0.x, fminf(p1.x, p2.x)) - padding;
  float min_y = fminf(p0.y, fminf(p1.y, p2.y)) - padding;
  float min_z = fminf(p0.z, fminf(p1.z, p2.z)) - padding;

  float max_x = fmaxf(p0.x, fmaxf(p1.x, p2.x)) + padding;
  float max_y = fmaxf(p0.y, fmaxf(p1.y, p2.y)) + padding;
  float max_z = fmaxf(p0.z, fmaxf(p1.z, p2.z)) + padding;

  return aabb(vec3(min_x, min_y, min_z), vec3(max_x, max_y, max_z));
}

void light::compute_surface_area()
{
  float surface{0.0f};
  triangles_areas.reserve(n_triangles);
  triangles_cdf.reserve(n_triangles);

  float triangle_surface{0.0f};

  for (size_t i = 0; i < n_triangles; ++i)
  {
    const point& p0 = vertices[vertex_indices[3*i]];
    const point& p1 = vertices[vertex_indices[3*i+1]];
    const point& p2 = vertices[vertex_indices[3*i+2]];

    triangle_surface = 0.5f * glm::length(cross(p1 - p0, p2 - p0));
    triangles_areas.push_back(triangle_surface);

    surface += triangle_surface;
    triangles_cdf.push_back(surface);
  }

  surface_area = surface;
}

void world_lights::compute_light_areas()
{
  for (auto& light : lights())
    light->compute_surface_area();
}

std::pair<point, const triangle*>
light::random_surface_point(uint16_t seed_x, uint16_t seed_y, uint16_t seed_z) const
{
  // select a triangle with a PDF weighted by the surface of each triangle using the inversion method
  // then return a uniformly distributed point from it

  // binary search to invert the CDF
  const float r0{random_float(0.0f,get_surface_area(), seed_x, seed_y, seed_z)};

  size_t sel{0};
  size_t len{n_triangles};
  size_t step{0};

  while (len != 0)
  {
    step = len / 2;

    if (triangles_cdf[sel + step] < r0)
    {
      sel += ++step;
      len -= step;
    } else {
      len = step;
    }
  }

  const point& p0 = vertices[vertex_indices[3*sel]];
  const point& p1 = vertices[vertex_indices[3*sel + 1]];
  const point& p2 = vertices[vertex_indices[3*sel + 2]];

  // uniform distribution on a triangle
  // u = 1 - sqrt(rand0)
  // v = sqrt(rand0) * rand1

  std::array<float,2> rnd_pair{ random_float(seed_x,seed_y,seed_z)
                              , random_float(seed_x,seed_y ,seed_z)};
  // it's better not to use random_float_pair() for this one, as the improvement is negligible and
  // definitely not worth the hassle of avoiding seed conflicts with other functions calling
  // random_float_pair() from the same thread
  auto r1{std::sqrt(rnd_pair[0])};

  // uv to world
  point res{p0};
  res += (1.0f - r1) * (p1-p0) + (r1 * rnd_pair[1]) * (p2-p0);

  return std::make_pair(res,ptr_triangles[sel]);
}

hit_check triangle::hit(const ray& r, float t_max) const
{
  // Adapted from Woop--Benthin--Wald "Watertight Ray/Triangle Intersection"
  // Journal of Computer Graphics Techniques, 2013

  // Numerical error analysis adapted from Pharr--Jakob--Humphreys,
  // "Physically Based Rendering: From Theory to Implementation"
  // online edition, 2018

  const point& p0{parent_mesh->vertices[parent_mesh->vertex_indices[3*number]]};
  const point& p1{parent_mesh->vertices[parent_mesh->vertex_indices[3*number+1]]};
  const point& p2{parent_mesh->vertices[parent_mesh->vertex_indices[3*number+2]]};

  // vertices relative to ray origin
  point tp0{p0-r.origin};
  point tp1{p1-r.origin};
  point tp2{p2-r.origin};

  // shear and scale vertices
  const float tp0x{tp0[r.perm.x] - r.shear_coefficients.x * tp0[r.perm.z]};
  const float tp0y{tp0[r.perm.y] - r.shear_coefficients.y * tp0[r.perm.z]};
  const float tp1x{tp1[r.perm.x] - r.shear_coefficients.x * tp1[r.perm.z]};
  const float tp1y{tp1[r.perm.y] - r.shear_coefficients.y * tp1[r.perm.z]};
  const float tp2x{tp2[r.perm.x] - r.shear_coefficients.x * tp2[r.perm.z]};
  const float tp2y{tp2[r.perm.y] - r.shear_coefficients.y * tp2[r.perm.z]};

  // scaled baricentric coordinates
  float u{tp2x * tp1y - tp2y * tp1x};
  float v{tp0x * tp2y - tp0y * tp2x};
  float w{tp1x * tp0y - tp1y * tp0x};

  // double precision fallback
  if (u == 0.0f || v == 0.0f || w == 0.0f)
  {
    double tp2x1y{double(tp2x)*double(tp1y)};
    double tp2y1x{double(tp2y)*double(tp1x)};
    u = float(tp2x1y - tp2y1x);

    double tp0x2y{double(tp0x)*double(tp2y)};
    double tp0y2x{double(tp0y)*double(tp2x)};
    v = float(tp0x2y - tp0y2x);

    double tp1x0y{double(tp1x)*double(tp0y)};
    double tp1y0x{double(tp1y)*double(tp0x)};
    w = float(tp1x0y - tp1y0x);
  }

  if ((u < 0.0f || v < 0.0f || w < 0.0f) && (u > 0.0f || v > 0.0f || w > 0.0f))
    return std::nullopt;

  float det{u+v+w};
  if (det == 0.0f)
    return std::nullopt;

  // scaled z coordinates of vertices
  const float tp0z{r.shear_coefficients.z * tp0[r.perm.z]};
  const float tp1z{r.shear_coefficients.z * tp1[r.perm.z]};
  const float tp2z{r.shear_coefficients.z * tp2[r.perm.z]};
  const float t_scaled{u * tp0z + v * tp1z + w * tp2z};

  if (std::signbit(t_scaled) || t_scaled > t_max * det)
    return std::nullopt;

  // unscaled values
  float inv_det{1.0f / det};
  float t{t_scaled  * inv_det};
  float uu{u * inv_det};
  float uv{v * inv_det};
  float uw{w * inv_det};

  // numerical error analysis

  // avoid intersection behind ray origin (see Pharr--Jakob--Humphreys)
  float max_zt{max_component(glm::abs(vec3{tp0z, tp1z, tp2z}))};
  float deltaZ{gamma_bound(3) * max_zt};
  float max_xt{max_component(glm::abs(vec3{tp0x, tp1x, tp2x}))};
  float max_yt{max_component(glm::abs(vec3{tp0y, tp1y, tp2y}))};
  float deltaX{gamma_bound(5) * (max_xt + max_zt)};
  float deltaY{gamma_bound(5) * (max_yt + max_zt)};
  float delta_bar = 2 * (gamma_bound(2) * max_xt * max_yt + deltaY * max_xt + deltaX * max_yt);
  float max_bar = max_component(glm::abs(vec3{u, v, w}));
  float deltaT = 3 * (gamma_bound(3) * max_bar * max_zt
               + delta_bar * max_zt + deltaZ * max_bar) * std::abs(inv_det);

  if (t <= deltaT)
    return std::nullopt;

  // error bounds
  float x_abs{(std::abs(uu * p0.x) + std::abs(uv * p1.x) + std::abs(uw * p2.x))};
  float y_abs{(std::abs(uu * p0.y) + std::abs(uv * p1.y) + std::abs(uw * p2.y))};
  float z_abs{(std::abs(uu * p0.z) + std::abs(uv * p1.z) + std::abs(uw * p2.z))};
  vec3 p_error{gamma_bound(7) * vec3{x_abs, y_abs, z_abs}};

  return hit_record{this,t,p_error,{uu,uv,uw}};
}