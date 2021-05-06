#include "extern/simdjson/singleheader/simdjson.h"
#include "meshes.h"
#include "math.h"

#include <fstream>

namespace gltf {

union number { int i; float f; };

using buffer = std::vector<unsigned char>;

struct scene {};

struct primitive
{
  int attr_vertices = -1;
  int attr_normals  = -1;
  int attr_tangents = -1;
  int attr_texcoord0 = -1;
  int attr_texcoord1 = -1;
  int attr_color0 = -1;
  int indices = -1;
  int material = -1;
  int mode = 4;

};

struct camera {};
struct material {};
struct texture {};
struct image {};
struct sampler {};

struct node
{
  int camera;
  std::vector<int> children;
  std::array<number,16> matrix{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
  int mesh;
  std::array<number,4> rotation{0,0,0,1};
  std::array<number,3> scale{1,1,1};
  std::array<number,3> translation{0,0,0};
};

struct buffer_view
{
  int buffer_index = -1;
  int byte_length  = -1;
  int byte_offset  =  0;
  int byte_stride  = -1;
};

struct accessor
{
  // TODO sparse accessors
  int buffer_view = -1;
  int byte_offset = 0;
  int component_type = -1;
    // 5120 byte,               size 1
    // 5121 unsigned byte,      size 1
    // 5122 short int,          size 2
    // 5123 unsigned short int, size 2
    // 5125 unsigned int,       size 4
    // 5126 float,              size 4
  bool is_normalized = false;
  int count = -1;
  std::string type;
    // "SCALAR" , number of components  1
    // "VEC2"   , number of components  2
    // "VEC3"   , number of components  3
    // "VEC4"   , number of components  4
    // "MAT2"   , number of components  4
    // "MAT3"   , number of components  9
    // "MAT4"   , number of components 16
  // int min;
  // int max;
  // simdjson::ondemand::object sparse;
};

int component_size(const accessor& acc)
{
  int s;
  switch(acc.component_type)
  {
    case 5120: s = 1; break;
    case 5121: s = 1; break;
    case 5122: s = 2; break;
    case 5123: s = 2; break;
    case 5125: s = 4; break;
    case 5126: s = 4; break;
    default: ; // print error message and exit
  }

  return s;
}

int n_components(const accessor& acc)
{
  int n;
  if (acc.type == "SCALAR") {
    n = 1;
  } else if (acc.type == "VEC2") {
    n = 2;
  } else if (acc.type == "VEC3") {
    n = 3;
  } else if (acc.type == "VEC4") {
    n = 4;
  } else if (acc.type == "MAT2") {
    n = 4;
  } else if (acc.type == "MAT3") {
    n = 9;
  } else if (acc.type == "MAT4") {
    n = 16;
  } else {
    //print error message and exit
  }
  return n;
}

int element_size(const accessor& acc)
{
  return component_size(acc) * n_components(acc);
}

void parse(std::string filename)
//void parse_gltf(std::string filename, std::vector<mesh>& meshes)
{
  simdjson::ondemand::parser parser;
  auto gltf = simdjson::padded_string::load(filename);
  simdjson::ondemand::document doc = parser.iterate(gltf);

  {
    auto version = std::string_view(doc["asset"]["version"]); // might throw
    if (version != "2.0")
    {
      // print error and exit
    }
    if (version == "2.0")
    {
      std::cout << "glTF 2.0 file detected\n";
    }
  }

  // ###############################################################################################
  // store which nodes are the root nodes of the scene
  // ###############################################################################################

  // TODO; register doc["scene"] and only process the relevant resources
  // if the "scene" property is not defined, print out an error message and exit

  // ###############################################################################################
  // store information about buffers and buffer views
  // ###############################################################################################
  std::vector<gltf::buffer> buffers;

  { // unnamed scope
    simdjson::ondemand::array buffs = doc["buffers"];
    for (auto x : buffs)
    {
      auto b = x.get_object();
      int byte_length;
      std::string_view uri;
      for (auto property : b)
      {
        // ignoring "name", "extensions" and "extras"
        if (property.key() == "byteLength")
          byte_length = uint64_t(property.value());
        if (property.key() == "uri")
          uri = std::string_view(property.value());
      }

      buffer current;
      current.reserve(byte_length);

      constexpr char signature[38] = "data:application/octet-stream;base64,";
      if (uri.size() > 37 && uri.rfind(signature, 0) == 0)
      {
        current = base64::decode(&uri[37]);
      } else {
        std::ifstream file(std::string(uri), std::ios::binary);
        if (!file.is_open())
        {
          std::cerr << "Unable to open " << uri << "\n";
          return;
        }
        current.insert(current.begin(),
                       std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());

        file.close();
      }
      buffers.emplace_back(std::move(current));
    }
  } // unnamed scope

  std::vector<gltf::buffer_view> views;
  { // unnamed scope
    simdjson::ondemand::array buffer_views = doc["bufferViews"];
    for (auto b : buffer_views)
    {
      auto buffer_view = b.get_object();
      gltf::buffer_view current;

      for (auto property : buffer_view)
      {
        // ignoring target, name, extensions and extras
        if (property.key() == "buffer")
          current.buffer_index = uint64_t(property.value());
        if (property.key() == "byteOffset")
          current.byte_offset = uint64_t(property.value());
        if (property.key() == "byteLength")
          current.byte_length = uint64_t(property.value());
        if (property.key() == "byteStride")
          current.byte_stride = uint64_t(property.value());
      }
      views.push_back(current);
    }
  } // unnamed scope

  std::vector<gltf::accessor> accessors;
  { // unnamed scope
    simdjson::ondemand::array accs = doc["accessors"];
    for (auto a : accs)
    {
      auto accessor = a.get_object();
      gltf::accessor current;

      for (auto property : accessor)
      {
        // ignoring name, extension and extra
        // currently ignoring also min, max and sparse
        if (property.key() == "bufferView")
          current.buffer_view = uint64_t(property.value());
        if (property.key() == "byteOffset")
          current.byte_offset = uint64_t(property.value());
        if (property.key() == "componentType")
          current.component_type = uint64_t(property.value());
        if (property.key() == "normalized")
          current.is_normalized = bool(property.value());
        if (property.key() == "count")
          current.count = uint64_t(property.value());
        if (property.key() == "type")
          current.type = std::string(std::string_view(property.value()));
      }
      accessors.push_back(current);
    }
  } // unnamed scope

  // ###############################################################################################
  // store meshes
  // ###############################################################################################
  std::vector<mesh> meshes;

  { // unnamed scope
    simdjson::ondemand::array gltf_meshes = doc["meshes"];
    for (auto x : gltf_meshes)
    {
      auto gltf_mesh = x.get_object();
      // ignoring "weights", "name", "extensions" and "extras"
      simdjson::ondemand::array primitives = gltf_mesh["primitives"];
      for (auto x : primitives)
      {
        primitive prim;

        auto p = x.get_object();
        for (auto property : p)
        {
          // ignoring "targets", "extensions" and "extras"
          if (property.key() == "attributes")
          {
            simdjson::ondemand::object attr_dict = property.value();
            for (auto attr : attr_dict)
            {
              // ignoring "JOINTS_0" and "WEIGHTS_0"
              if (attr.key() == "POSITION")
                prim.attr_vertices = uint64_t(attr.value());
              if (attr.key() == "NORMAL")
                prim.attr_normals = uint64_t(attr.value());
              if (attr.key() == "TANGENT")
                prim.attr_tangents = uint64_t(attr.value());
              if (attr.key() == "TEXCOORD_0")
                prim.attr_texcoord0 = uint64_t(attr.value());
              if (attr.key() == "TEXCOORD_1")
                prim.attr_texcoord1 = uint64_t(attr.value());
              if (attr.key() == "COLOR_1")
                prim.attr_color0 = uint64_t(attr.value());
            }
          }
          if (property.key() == "indices")
          {
            prim.indices = uint64_t(property.value());
            // if not defined, the mesh has to be created differently
            // (i.e. following GL's drawArrays() instead of drawElements())
          }
          if (property.key() == "material")
            prim.material = uint64_t(property.value());
          if (property.key() == "mode")
          {
            prim.mode = uint64_t(property.value());
            // if anything other than 4, print error message "currently supporting only triangle meshes as primitives" and exit
          }
        }
        // ######## CREATE MESH OBJECT
        // stuff to move into the mesh
        int n_vertices{accessors[prim.attr_vertices].count};
        int n_triangles{accessors[prim.indices].count / 3};
        std::vector<vec> vertices;

        int offset = views[accessors[prim.attr_vertices].buffer_view].byte_offset
                   + accessors[prim.attr_vertices].byte_offset;
        std::cout << "vertices offset: " << offset << "\n";
        int length = 12 * accessors[prim.attr_vertices].count;
        std::cout << "vertices length: " << length << "\n";
        std::vector<unsigned char>& data = buffers[views[accessors[prim.attr_vertices].buffer_view].buffer_index];

        for (int i = offset; i < offset + length; i+=12)
        {
          point p;
          float f;
          std::memcpy(&f, &data[i], 4);
          p.x() = f;
          std::memcpy(&f, &data[i+4], 4);
          p.y() = f;
          std::memcpy(&f, &data[i+8], 4);
          p.z() = f;
          vertices.push_back(p);
        }

        std::vector<int> vertex_indices;

        offset = views[accessors[prim.indices].buffer_view].byte_offset
                   + accessors[prim.indices].byte_offset;
        std::cout << "vertices offset: " << offset << "\n";
        length = 2 * accessors[prim.indices].count;
        std::cout << "vertices length: " << length << "\n";
        data = buffers[views[accessors[prim.indices].buffer_view].buffer_index];

        for (int i = offset; i < offset + length ; i+=2)
          vertex_indices.push_back((uint16_t)data[i]);

        std::cout << "indices:\n";
        for (auto i : vertex_indices)
          std::cout << i << "\n";

        std::cout << "vertices:\n";
        for (auto p : vertices)
          std::cout << p.x() << " " << p.y() << " " << p.z() << "\n";
      }
    }
  } // unnamed scope

  // TODO add transform objects to the project
}
} // namespace gltf

int main()
{
  //gltf::parse("Box.gltf");
  gltf::parse("Box(bin_ref).gltf");
  return 0;
}