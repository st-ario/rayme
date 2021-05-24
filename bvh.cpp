#include "elements.h"
#include <algorithm>

aabb surrounding_box(aabb box0, aabb box1)
{
  point small(fminf(box0.min().x(), box1.min().x()),
              fminf(box0.min().y(), box1.min().y()),
              fminf(box0.min().z(), box1.min().z()));

  point big(fmaxf(box0.max().x(), box1.max().x()),
            fmaxf(box0.max().y(), box1.max().y()),
            fmaxf(box0.max().z(), box1.max().z()));

  return aabb(small,big);
}

bvh_tree::bvh_tree(std::vector<std::shared_ptr<primitive>>&& primitives)
: primitives{primitives}
{
  root = recursive_build(bvh_tree::primitives, 0, bvh_tree::primitives.size(), 0);
}

inline float surface_area(std::shared_ptr<primitive>& leaf)
{
  return fabs(leaf->bounds.max().x() - leaf->bounds.min().x())
       * fabs(leaf->bounds.max().y() - leaf->bounds.min().y())
       * fabs(leaf->bounds.max().z() - leaf->bounds.min().z());
}

float sah(std::vector<std::shared_ptr<primitive>>& leaves, size_t begin, size_t end, size_t at)
{
  float left_surface_area{0};
  float right_surface_area{0};
  for (size_t i = begin; i < at; ++i)
    left_surface_area += surface_area(leaves[i]);
  for (size_t i = at; i < end; ++i)
    right_surface_area += surface_area(leaves[i]);

  return left_surface_area * (at - begin) + right_surface_area * (end - at);
}

std::shared_ptr<element> bvh_tree::recursive_build(std::vector<std::shared_ptr<primitive>>& leaves, size_t begin, size_t end, unsigned short int axis) const
{
  if (begin + 1 == end)
  {
    return leaves[begin];
  }

  // sort leaves based on their centroid coordinate
  std::sort(leaves.begin()+begin, leaves.begin()+end,
            [=](const std::shared_ptr<primitive>& leaf1, const std::shared_ptr<primitive>& leaf2)
            {
              return leaf1->centroid[axis] < leaf2->centroid[axis];
            });

  // split at the point minimizing the SAH
  size_t split_at = end;
  float sah_split = infinity;
  for (size_t at = begin+1; at < end; ++at)
  {
    float current_sah = sah(leaves, begin, end, at);
    if (current_sah < sah_split)
    {
      split_at = at;
      sah_split = current_sah;
    }
  }

  // create a new node
  std::shared_ptr<bvh_node> new_node = std::make_shared<bvh_node>(leaves, begin, end);
  new_node->left = recursive_build(leaves, begin, split_at, (axis + 1) % 3);
  new_node->right = recursive_build(leaves, split_at, end, (axis + 1) % 3);

  return new_node;
}

bvh_node::bvh_node(const std::vector<std::shared_ptr<primitive>>& leaves, size_t start, size_t end)
{
  for(size_t i = start; i < end; ++i)
    bounds = surrounding_box(bounds,leaves[i]->bounds);
}

inline hit_check bvh_tree::hit(const ray& r, float t_max) const { return root->hit(r, t_max); }
inline aabb bvh_tree::bounding_box() const { return root->bounding_box(); }