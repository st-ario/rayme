#pragma once

#include <string>
#include <vector>
#include <memory>

class primitive;
class camera;

void parse_gltf( const std::string& filename
               , std::vector<std::shared_ptr<const primitive>>& primitives
               , std::shared_ptr<camera>& cam);