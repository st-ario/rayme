#pragma once

#include <string>
#include <vector>
#include <memory>

class primitive;
class camera;

void parse_gltf( const std::string& filename
               , std::vector<std::unique_ptr<const primitive>>& primitives
               , std::unique_ptr<camera>& cam
               , uint16_t image_height);