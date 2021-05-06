#include <vector>
#include "my_vectors.h"

class mesh
{
  public:
    const int n_vertices;
    const int n_triangles;
    std::vector<int> vertex_indices;
    std::vector<point> vertices;
    std::vector<normed_vec> normals;
    std::vector<normed_vec> tangents;

    mesh( int n_vertices
        , int n_triangles
        , std::vector<int> vertex_indices
        , std::vector<point> vertices
        , std::vector<normed_vec> normals = {}
        , std::vector<normed_vec> tangents = {}) :
        n_vertices{n_vertices}, n_triangles{n_triangles},
        vertex_indices{vertex_indices}, vertices{vertices},
        normals{normals}, tangents{tangents} {}
}; // class mesh