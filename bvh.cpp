#include "bvh.h"
#include <algorithm>

aabb surrounding_box(aabb box0, aabb box1)
{
  point lower(fminf(box0.lower().x, box1.lower().x),
              fminf(box0.lower().y, box1.lower().y),
              fminf(box0.lower().z, box1.lower().z));

  point upper(fmaxf(box0.upper().x, box1.upper().x),
              fmaxf(box0.upper().y, box1.upper().y),
              fmaxf(box0.upper().z, box1.upper().z));

  return aabb{lower,upper};
}

bvh_tree::bvh_tree(std::vector<std::unique_ptr<const primitive>>& primitives)
{
  root = recursive_build(primitives, 0, primitives.size());
}

inline float surface_area(const std::unique_ptr<const primitive>& leaf)
{
  auto v{glm::abs(leaf->bounds.upper() - leaf->bounds.lower())};

  return v.x * v.y * v.z;
}

float sah(const std::vector<std::unique_ptr<const primitive>>& leaves, size_t begin, size_t end, size_t at)
{
  float left_surface_area{0};
  float right_surface_area{0};
  for (size_t i = begin; i < at; ++i)
    left_surface_area += surface_area(leaves[i]);
  for (size_t i = at; i < end; ++i)
    right_surface_area += surface_area(leaves[i]);

  return left_surface_area * (at - begin) + right_surface_area * (end - at);
}

std::unique_ptr<const element>
bvh_tree::recursive_build( std::vector<std::unique_ptr<const primitive>>& leaves
                         , size_t begin
                         , size_t end) const
{
  if (begin + 1 == end)
    return std::move(leaves[begin]);

  if (begin + 2 == end)
  {
    std::unique_ptr<bvh_node> new_node = std::make_unique<bvh_node>(leaves, begin, end);
    new_node->left = recursive_build(leaves, begin, begin+1);
    new_node->right = recursive_build(leaves, begin+1, end);

    return new_node;
  }

  // pick splitting axis with the largest extension
  unsigned short int axis = 3;
  { // unnamed scope
    float span{-1.0f};

    for (unsigned short int i = 0; i < 3; ++i)
    {
      float min_coord{infinity};
      float max_coord{-infinity};

      for (size_t j = begin; j < end; ++j)
      {
        if (leaves[j]->centroid[i] < min_coord)
          min_coord = leaves[j]->centroid[i];
        if (leaves[j]->centroid[i] > max_coord)
          max_coord = leaves[j]->centroid[i];
      }

      float current_span{std::fabs(max_coord - min_coord)};
      if (current_span > span)
      {
        span = current_span;
        axis = i;
      }
    }
  } // unnamed scope

  // sort leaves based on their centroid coordinate
  std::sort(leaves.begin()+begin, leaves.begin()+end,
            [=]( const std::unique_ptr<const primitive>& leaf1
               , const std::unique_ptr<const primitive>& leaf2)
            { return leaf1->centroid[axis] < leaf2->centroid[axis]; });

  // split at the point minimizing the SAH
  size_t split_at = end;
  float sah_split = infinity;

  // binning method
  constexpr uint n_bins{16u};
  std::array<size_t,n_bins> bins_counter;
  bins_counter.fill(0u);

  float range{leaves[end-1]->centroid[axis] - leaves[begin]->centroid[axis]};

  if (range != 0)
  {
    // populate the bins according to the centroid coordinate
    for (size_t i = begin; i < end; ++i)
    {
      // M = N * (centroid - lower_bound) / (upper_bound - lower_bound)
      int M = n_bins * (leaves[i]->centroid[axis] - leaves[begin]->centroid[axis]) / range;
      if (M == n_bins) --M;
      bins_counter[M] += 1;
    }

    // pick the splitting bin
    size_t cumulative_count{0u};
    for (uint i = 0; i < n_bins - 1; ++i)
    {
      if (bins_counter[i] == 0)
        continue;
      cumulative_count += bins_counter[i];
      float current_sah = sah(leaves, begin, end, begin + cumulative_count);
      if (current_sah < sah_split)
      {
        split_at = begin + cumulative_count;
        sah_split = current_sah;
      }
    }
  } else {
    // all the primitives in the current range have the same centroid, split in the middle
    split_at = begin + ((end - begin) / 2);
  }

  // create a new node
  std::unique_ptr<bvh_node> new_node = std::make_unique<bvh_node>(leaves, begin, end);

  new_node->left = recursive_build(leaves, begin, split_at);
  new_node->right = recursive_build(leaves, split_at, end);

  return new_node;
}

bvh_node::bvh_node(const std::vector<std::unique_ptr<const primitive>>& leaves, size_t start, size_t end)
{
  for(size_t i = start; i < end; ++i)
    bounds = surrounding_box(bounds,leaves[i]->bounds);
}

inline hit_check bvh_tree::hit(const ray& r, float t_max) const { return root->hit(r, t_max); }