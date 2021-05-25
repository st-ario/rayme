#pragma once

#include "transformations.h"

class material;
class primitive;

// the ptr_mat argument is a temporary hack, to pass to all meshes the same standard material for
// rendering, to be removed as soon as materials are properly dealt with
void parse_gltf( const std::string& filename, std::vector<std::shared_ptr<primitive>>& primitives
               , const std::shared_ptr<material>& ptr_mat);