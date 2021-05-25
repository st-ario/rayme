#pragma once

#include "extern/simdjson/singleheader/simdjson.h"
#include "meshes.h"
#include "transformations.h"

#include <fstream>

union  gltf_numeric { int i; float f; };
using  gltf_buffer = std::vector<unsigned char>;
struct gltf_scene { std::vector<int> roots; };

struct gltf_primitive
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

struct gltf_camera {};
struct gltf_material {};
struct glft_texture {};
struct gltf_image {};
struct gltf_sampler {};

struct gltf_node
{
  int camera = -1;
  std::vector<int> children;
  bool is_root = false;
  gltf_node* parent = nullptr;
  int mesh = -1;
  std::shared_ptr<transformation> matrix;
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

// the ptr_mat argument is a temporary hack, to pass to all meshes the same standard material for rendering
// to be removed as soon as materials are properly dealt with
void parse_gltf(const std::string& filename, std::vector<std::shared_ptr<primitive>>& primitives, const std::shared_ptr<material>& ptr_mat)
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

  // TODO if the "scene" property is not defined, print out an error message and exit

  simdjson::ondemand::value document_scene = doc["scene"]; // might throw
  int selected_scene;
  selected_scene = document_scene.get_int64();
  gltf_scene scene;
  simdjson::ondemand::array document_scenes = doc["scenes"];
  { // unnamed scope
    int j = 0;
    for (auto s : document_scenes)
    {
      if (j == selected_scene)
      {
        auto scene_obj = s.get_object();
        simdjson::ondemand::array scene_nodes = scene_obj["nodes"];
        for (auto node : scene_nodes)
          scene.roots.push_back(node.get_int64());
      }
      ++j;
    }
  } // unnamed scope

  // ###############################################################################################
  // traverse scene tree
  // ###############################################################################################

  // initialize root nodes of the scene
  simdjson::ondemand::array document_nodes = doc["nodes"];
  std::vector<gltf_node> scene_root_nodes;
  for (int j : scene.roots)
  {
    int i = 0;
    for (auto node : document_nodes)
    {
      if (i == j)
      {
        auto node_obj = node.get_object();
        gltf_node current_node;
        current_node.is_root = true;

        for (auto property : node_obj)
        {
          if (property.key() == "camera")
            current_node.camera = property.value().get_uint64();

          if (property.key() == "mesh")
            current_node.mesh = property.value().get_uint64();

          if (property.key() == "children")
          {
            simdjson::ondemand::array children_iterator = property.value();
            for (auto k : children_iterator)
              current_node.children.emplace_back(k.get_uint64());
          }
          if (property.key() == "matrix")
          {
            mat4 mat;
            simdjson::ondemand::array components = property.value();
            unsigned short int s = 0;
            for (auto k : components)
            {
              mat[s] = k.get_double();
              ++s;
            }
            std::shared_ptr<transformation> mat_ptr{std::make_shared<transformation>(mat)};
            current_node.matrix = mat_ptr;
          }
          if (property.key() == "rotation")
          {
            vec4 q;
            simdjson::ondemand::array components = property.value();
            unsigned short int s = 0;
            for (auto k : components)
            {
              q[s] = k.get_double();
              ++s;
            }
            std::shared_ptr<transformation> rot_ptr{std::make_shared<transformation>(rotation_matrix(q))};
            current_node.matrix = rot_ptr;
          }
          if (property.key() == "scale")
          {
            vec3 scale;
            simdjson::ondemand::array components = property.value();
            unsigned short int s = 0;
            for (auto k : components)
            {
              scale[s] = k.get_double();
              ++s;
            }
            std::shared_ptr<transformation> scale_ptr{std::make_shared<transformation>(scale_matrix(scale))};
            current_node.matrix = scale_ptr;
          }
          if (property.key() == "translation")
          {
            vec3 translation;
            simdjson::ondemand::array components = property.value();
            unsigned short int s = 0;
            for (auto k : components)
            {
              translation[s] = k.get_double();
              ++s;
            }
            std::shared_ptr<transformation> tr_ptr{std::make_shared<transformation>(translation_matrix(translation))};
            current_node.matrix = tr_ptr;
          }
        }
        scene_root_nodes.push_back(current_node);
      }
    }
  }

  // initialize the children of each root node
  // TODO


  // ###############################################################################################
  // store information about buffers and buffer views
  // ###############################################################################################
  std::vector<gltf_buffer> buffers;

  { // unnamed scope
    simdjson::ondemand::array document_buffers = doc["buffers"];
    for (auto document_buffer : document_buffers)
    {
      auto buffer_obj = document_buffer.get_object();
      int byte_length;
      std::string_view uri;
      for (auto property : buffer_obj)
      {
        // ignoring "name", "extensions" and "extras"
        if (property.key() == "byteLength")
          byte_length = uint64_t(property.value());
        if (property.key() == "uri")
          uri = std::string_view(property.value());
      }

      gltf_buffer current;
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

  std::vector<buffer_view> views;
  { // unnamed scope
    simdjson::ondemand::array buffer_views = doc["bufferViews"];
    for (auto b : buffer_views)
    {
      auto b_view_obj = b.get_object();
      buffer_view current;

      for (auto property : b_view_obj)
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

  std::vector<accessor> accessors;
  { // unnamed scope
    simdjson::ondemand::array accs = doc["accessors"];
    for (auto a : accs)
    {
      auto acc_obj = a.get_object();
      accessor current;

      for (auto property : acc_obj)
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
  std::vector<std::shared_ptr<mesh>> meshes;

  { // unnamed scope
    simdjson::ondemand::array document_meshes = doc["meshes"];
    for (auto mesh_iterator : document_meshes)
    {
      auto json_mesh = mesh_iterator.get_object();
      // ignoring "weights", "name", "extensions" and "extras"
      simdjson::ondemand::array mesh_primitives = json_mesh["primitives"];
      for (auto primitive_iterator : mesh_primitives)
      {
        gltf_primitive prim;

        auto json_primitive = primitive_iterator.get_object();
        for (auto property : json_primitive)
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
              if (attr.key() == "COLOR_0")
                prim.attr_color0 = uint64_t(attr.value());
            }
          }
          if (property.key() == "indices")
          {
            prim.indices = uint64_t(property.value());
            // TODO if not defined, the mesh has to be created differently
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
        std::vector<point> vertices;
        int n_vertices{0};
        { // unnamed scope
          accessor& acc{accessors[prim.attr_vertices]};
          n_vertices = acc.count;
          int offset = views[acc.buffer_view].byte_offset
                     + acc.byte_offset;
          int s_component = 4;
          int s_element = 12;
          int length = s_element * accessors[prim.attr_vertices].count;

          gltf_buffer& data{buffers[views[acc.buffer_view].buffer_index]};

          for (int i = offset; i < offset + length; i+=s_element)
          {
            point p;
            float f;
            std::memcpy(&f, &data[i], s_component);
            p.x() = f;
            std::memcpy(&f, &data[i+s_component], s_component);
            p.y() = f;
            std::memcpy(&f, &data[i+2*s_component], s_component);
            p.z() = f;
            vertices.push_back(p);
          }
        } // unnamed scope

        std::vector<int> vertex_indices;
        int n_triangles{0};
        { // unnamed scope
          accessor& acc{accessors[prim.indices]};
          n_triangles = acc.count / 3;

          int offset = views[acc.buffer_view].byte_offset
                     + acc.byte_offset;
          int s_component = component_size(acc);
          int length = s_component * acc.count;
          gltf_buffer& data{buffers[views[acc.buffer_view].buffer_index]};

          for (int i = offset; i < offset + length ; i+=s_component)
          {
            uint16_t current_index;
            std::memcpy(&current_index, &data[i], s_component);
            vertex_indices.push_back(current_index);
          }
        } // unnamed scope

        std::vector<normed_vec3> normals;
        if (prim.attr_normals != -1)
        {
          accessor& acc{accessors[prim.attr_normals]};
          int offset = views[acc.buffer_view].byte_offset
                     + acc.byte_offset;
          int s_component = 4;
          int s_element = 12;
          int length = s_element * accessors[prim.attr_normals].count;
          gltf_buffer& data{buffers[views[acc.buffer_view].buffer_index]};

          for (int i = offset; i < offset + length; i+=s_element)
          {
            vec3 v;
            float f;
            std::memcpy(&f, &data[i], s_component);
            v.x() = f;
            std::memcpy(&f, &data[i+s_component], s_component);
            v.y() = f;
            std::memcpy(&f, &data[i+2*s_component], s_component);
            v.z() = f;
            normals.emplace_back(normed_vec3(v));
          }
        }

        std::vector<vec4> tangents;
        if (prim.attr_tangents != -1)
        {
          accessor& acc{accessors[prim.attr_tangents]};
          int offset = views[acc.buffer_view].byte_offset
                     + acc.byte_offset;
          int s_component = 4;
          int s_element = 16;
          int length = s_element * accessors[prim.attr_tangents].count;
          gltf_buffer& data{buffers[views[acc.buffer_view].buffer_index]};

          for (int i = offset; i < offset + length; i+=s_element)
          {
            vec4 v;
            float f;
            std::memcpy(&f, &data[i], s_component);
            v[0] = f;
            std::memcpy(&f, &data[i+s_component], s_component);
            v[1] = f;
            std::memcpy(&f, &data[i+2*s_component], s_component);
            v[2] = f;
            std::memcpy(&f, &data[i+2*s_component], s_component);
            v[3] = f;

            tangents.push_back(v);
          }
        }

        std::shared_ptr<mesh> current_mesh = std::make_shared<mesh>(
          n_vertices, n_triangles, vertex_indices, vertices, ptr_mat, normals, tangents);

        primitives.reserve(primitives.size() + n_triangles);

        for (auto& tri : current_mesh->get_triangles())
          primitives.emplace_back(std::move(tri));
      }
    }
  } // unnamed scope
}