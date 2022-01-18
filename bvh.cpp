#include "bvh.h"
#include <algorithm>
#include <stack>
#include <queue>

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

bvh_tree::bvh_tree(std::vector<std::unique_ptr<const primitive>>&& primitives)
 : leaves{std::move(primitives)}
{
  struct tracker
  {
    bvh_node* ptr;
    size_t begin;
    size_t end;
  };

  std::queue<tracker> q;

  // create root node and push it into the queqe
  m_nodes.reserve(leaves.size()); // binary tree: as many inner nodes as leaves (-1)

  // emplace root in the object pool
  m_nodes.emplace_back(leaves,0,leaves.size());
  q.push(tracker{&m_nodes[0], 0, leaves.size()});

  //nodes.reserve(2 * leaves.size()); // inner nodes and leaves

  while (!q.empty())
  {
    // remove current node from the queue and put it in the nodes
    size_t begin{q.front().begin};
    size_t end{q.front().end};
    bvh_node* current{q.front().ptr};
    //nodes.push_back(current);
    q.pop();

    // create children nodes for current node
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
    constexpr int n_bins{16};
    std::array<size_t,n_bins> bins_counter;
    bins_counter.fill(0u);

    float range{leaves[end-1]->centroid[axis] - leaves[begin]->centroid[axis]};

    if (range != 0)
    {
      // populate the bins according to the centroid coordinate
      for (size_t i = begin; i < end; ++i)
      {
        // M = N * (centroid - lower_bound) / (upper_bound - lower_bound)
        int M{static_cast<int>( n_bins
                              * ( leaves[i]->centroid[axis]
                              -   leaves[begin]->centroid[axis])
                              / range)};
        if (M == n_bins) --M;
        bins_counter[M] += 1;
      }

      // pick the splitting bin
      size_t cumulative_count{0u};
      for (int i = 0; i < n_bins - 1; ++i)
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

    if (begin + 1 == split_at && begin + 2 == end)
    {
      current->left = leaves[begin].get();
      current->right = leaves[begin+1].get();
      continue;
    }

    if (split_at == begin + 1)
    {
      // left branch is a leaf
      current->left = leaves[begin].get();

      // create right branch
      m_nodes.emplace_back(leaves,split_at,end);
      current->right = &m_nodes.back();
      q.push(tracker{&m_nodes.back(), split_at, end});
      continue;
    }

    if (end == split_at + 1)
    {
      // right branch is a leaf
      current->right = leaves[split_at].get();

      // create left branch
      m_nodes.emplace_back(leaves,begin,split_at);
      current->left = &m_nodes.back();
      q.push(tracker{&m_nodes.back(), begin, split_at});
      continue;
    }

    // create left branch
    m_nodes.emplace_back(leaves,begin,split_at);
    current->left = &m_nodes.back();
    q.push(tracker{&m_nodes.back(), begin, split_at});

    // create right branch
    m_nodes.emplace_back(leaves,split_at,end);
    current->right = &m_nodes.back();
    q.push(tracker{&m_nodes.back(), split_at, end});
  }
}

bvh_node::bvh_node(const std::vector<std::unique_ptr<const primitive>>& leaves, size_t begin, size_t end)
{
  for(size_t i = begin; i < end; ++i)
    bounds = surrounding_box(bounds,leaves[i]->bounds);
}

hit_check bvh_tree::hit(const ray& r, float t_max) const
{
  constexpr static float eps{gamma_bound(5)};

  vec3 lower = glm::abs(r.origin-m_nodes[0].bounds.lower());
  lower[0] = next_float_down(lower[0]);
  lower[1] = next_float_down(lower[1]);
  lower[2] = next_float_down(lower[2]);
  vec3 upper = glm::abs(r.origin-m_nodes[0].bounds.upper());
  upper[0] = next_float_up(upper[0]);
  upper[1] = next_float_up(upper[1]);
  upper[2] = next_float_up(upper[2]);
  float max_z = max(lower[r.perm.z],upper[r.perm.z]);
  float err_near_x = next_float_up(lower[r.perm.x]+max_z);
  float err_near_y = next_float_up(lower[r.perm.y]+max_z);
  float org_near_x = next_float_up(r.origin[r.perm.x]+next_float_up(eps*err_near_x));
  float org_near_y = next_float_up(r.origin[r.perm.y]+next_float_up(eps*err_near_y));
  float err_far_x = next_float_up(upper[r.perm.x]+max_z);
  float err_far_y = next_float_up(upper[r.perm.y]+max_z);
  float org_far_x = next_float_down(r.origin[r.perm.x]-next_float_up(eps*err_far_x));
  float org_far_y = next_float_down(r.origin[r.perm.y]-next_float_up(eps*err_far_y));
  if (r.direction[r.perm.x] < 0.0f) std::swap(org_near_x,org_far_x);
  if (r.direction[r.perm.y] < 0.0f) std::swap(org_near_y,org_far_y);

  if (!m_nodes[0].bounds.hit(r,t_max,org_near_x,org_near_y,org_far_x,org_far_y))
    return std::nullopt;

  std::stack<const bounded*> stck;
  stck.push(&m_nodes[0]);

  hit_check res;
  bool first_iteration{true};

  while (!stck.empty())
  {
    const bounded* current{stck.top()};
    stck.pop();

    // check whether in the meantime between the push and the pop we hit something closer
    if (!first_iteration && res && res->t() < t_max)
      continue;

    if (current->is_primitive)
    {
      const primitive* curr_node{static_cast<const primitive*>(current)};
      hit_check check{curr_node->hit(r,t_max)};
      if (check)
      {
        res = check;
        t_max = check->t();
      }
      continue;
    }

    // not a primitive -> node
    const bvh_node* curr_node{static_cast<const bvh_node*>(current)};

    std::optional<float> t_left{curr_node->left->bounds.hit(r,t_max,org_near_x,org_near_y,org_far_x,org_far_y)};
    std::optional<float> t_right{curr_node->right->bounds.hit(r,t_max,org_near_x,org_near_y,org_far_x,org_far_y)};

    if (t_left && t_right)
    {
      if (*t_left < *t_right)
      {
        stck.push(curr_node->right);
        stck.push(curr_node->left);
      } else {
        stck.push(curr_node->left);
        stck.push(curr_node->right);
      }
    } else if (t_left) {
        stck.push(curr_node->left);
    } else if (t_right) {
        stck.push(curr_node->right);
    }
  }
  return res;
}