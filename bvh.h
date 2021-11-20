#pragma once

#include "ray.h"
#include "meshes.h"

class bvh_node : public bounded
{
  public:
    const bounded* left;
    const bounded* right;

  public:
    bvh_node(const std::vector<std::unique_ptr<const primitive>>& primitives, size_t start, size_t end);
};

class bvh_tree
{
  public:
    explicit bvh_tree(std::vector<std::unique_ptr<const primitive>>&& primitives);
    hit_check hit(const ray& r, float t_max) const;

  private:
    std::vector<std::unique_ptr<const primitive>> leaves;
    std::vector<bvh_node> m_nodes;
};