#include "gltf_parser.h"
#include "meshes.h"
#include "extern/simdjson/singleheader/simdjson.h"

union  gltf_numeric { int i; float f; };
using  gltf_buffer = std::vector<unsigned char>;

struct gltf_node
{
  int camera = -1;
  std::vector<int> children_indices;
  std::vector<std::shared_ptr<gltf_node>> children;
  std::weak_ptr<gltf_node> parent;
  std::shared_ptr<mesh> m_mesh;
  std::shared_ptr<transformation> matrix;
  std::shared_ptr<transformation> translate{ std::make_shared<transformation>() };
  std::shared_ptr<transformation> scale{ std::make_shared<transformation>() };
  std::shared_ptr<transformation> rotate{ std::make_shared<transformation>() };
};

struct gltf_scene { std::shared_ptr<gltf_node> root_node{std::make_shared<gltf_node>()}; };

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

void apply_pointwise_transformation(const transformation& M, mesh& mesh)
{
  for (point& p : mesh.vertices)
    M.apply_to(p);

  for (normed_vec3& n : mesh.normals); // TODO
  for (vec4& v : mesh.tangents); // TODO
}

// TODO can make it faster by multiplying all the matrices first, then acting on the mesh

void apply_mesh_transformations(gltf_node& node, mesh& mesh)
{
  if (node.matrix)
    apply_pointwise_transformation(*(node.matrix), mesh);

  transformation id;
  if (*(node.translate) != id || *(node.rotate) != id || *(node.scale) != id)
  {
    transformation total{*(node.translate) * *(node.rotate) * *(node.scale)};
    apply_pointwise_transformation(total, mesh);
  }

  if (auto p_p = node.parent.lock())
    apply_mesh_transformations(*p_p, mesh);
}

void apply_mesh_transformations(gltf_node& node)
{
  if (node.matrix)
    apply_pointwise_transformation(*(node.matrix), *(node.m_mesh));

  transformation id;
  if (*(node.translate) != id || *(node.rotate) != id || *(node.scale) != id)
  {
    transformation total{*(node.translate) * *(node.rotate) * *(node.scale)};
    apply_pointwise_transformation(total, *(node.m_mesh));
  }

  if (auto p_p = node.parent.lock())
    apply_mesh_transformations(*p_p, *(node.m_mesh));
}

mesh store_mesh( int index
               , simdjson::ondemand::document& doc
               , const std::vector<gltf_buffer>& buffers
               , const std::vector<buffer_view>& views
               , const std::vector<accessor>& accessors
               , const std::shared_ptr<material>& ptr_mat)
{
    simdjson::ondemand::array document_meshes = doc["meshes"];
    int j = 0;
    for (auto mesh_iterator : document_meshes)
    {
      if (j != index)
        break;

      auto json_mesh = mesh_iterator.get_object();
      simdjson::ondemand::array mesh_primitives = json_mesh["primitives"];
      for (auto primitive_iterator : mesh_primitives)
      {
        gltf_primitive prim;

        auto json_primitive = primitive_iterator.get_object();
        for (auto property : json_primitive)
        {
          if (property.key() == "attributes")
          {
            simdjson::ondemand::object attr_dict = property.value();
            for (auto attr : attr_dict)
            {
              if (attr.key() == "POSITION")
                prim.attr_vertices = attr.value().get_uint64();
              if (attr.key() == "NORMAL")
                prim.attr_normals = attr.value().get_uint64();
              if (attr.key() == "TANGENT")
                prim.attr_tangents = attr.value().get_uint64();
              if (attr.key() == "TEXCOORD_0")
                prim.attr_texcoord0 = attr.value().get_uint64();
              if (attr.key() == "TEXCOORD_1")
                prim.attr_texcoord1 = attr.value().get_uint64();
              if (attr.key() == "COLOR_0")
                prim.attr_color0 = attr.value().get_uint64();
            }
          }
          if (property.key() == "indices")
          {
            prim.indices = property.value().get_uint64();
            // TODO if not defined, the mesh has to be created differently
            // (i.e. following GL's drawArrays() instead of drawElements())
          }
          if (property.key() == "material")
            prim.material = property.value().get_uint64();
          if (property.key() == "mode")
          {
            prim.mode = property.value().get_uint64();
            // if anything other than 4, print error message "currently supporting only triangle meshes as primitives" and exit
          }
        }
        // ######## CREATE MESH OBJECT
        std::vector<point> vertices;
        int n_vertices{0};
        { // unnamed scope
          const accessor& acc{accessors[prim.attr_vertices]};
          n_vertices = acc.count;
          int offset = views[acc.buffer_view].byte_offset
                     + acc.byte_offset;
          int s_component = 4;
          int s_element = 12;
          int length = s_element * accessors[prim.attr_vertices].count;

          const gltf_buffer& data{buffers[views[acc.buffer_view].buffer_index]};

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
          const accessor& acc{accessors[prim.indices]};
          n_triangles = acc.count / 3;

          int offset = views[acc.buffer_view].byte_offset
                     + acc.byte_offset;
          int s_component = component_size(acc);
          int length = s_component * acc.count;
          const gltf_buffer& data{buffers[views[acc.buffer_view].buffer_index]};

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
          const accessor& acc{accessors[prim.attr_normals]};
          int offset = views[acc.buffer_view].byte_offset
                     + acc.byte_offset;
          int s_component = 4;
          int s_element = 12;
          int length = s_element * accessors[prim.attr_normals].count;
          const gltf_buffer& data{buffers[views[acc.buffer_view].buffer_index]};

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
          const accessor& acc{accessors[prim.attr_tangents]};
          int offset = views[acc.buffer_view].byte_offset
                     + acc.byte_offset;
          int s_component = 4;
          int s_element = 16;
          int length = s_element * accessors[prim.attr_tangents].count;
          const gltf_buffer& data{buffers[views[acc.buffer_view].buffer_index]};

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

        return mesh{n_vertices, n_triangles, vertex_indices, vertices, ptr_mat, normals, tangents};
      }
      ++j;
    }
}

// the ptr_mat argument is a temporary hack, to pass to all meshes the same standard material for
// rendering, to be removed as soon as materials are properly dealt with
void initialize_tree( std::shared_ptr<gltf_node>& parent
                    , simdjson::ondemand::document& doc
                    , const std::vector<gltf_buffer>& buffers
                    , const std::vector<buffer_view>& views
                    , const std::vector<accessor>& accessors
                    , const std::shared_ptr<material>& ptr_mat
                    , std::vector<std::shared_ptr<primitive>>& primitives)
{
  for (int child_index : parent->children_indices)
  {
    simdjson::ondemand::array document_nodes = doc["nodes"];
    int i = 0;
    for (auto node_iterator : document_nodes)
    {
      if (i == child_index)
      {
        simdjson::ondemand::object node_obj = node_iterator.get_object();
        std::shared_ptr<gltf_node> child_node{std::make_shared<gltf_node>()};
        child_node->parent = parent;
        for (auto property : node_obj)
        {
          if (property.key() == "camera")
            child_node->camera = property.value().get_uint64();

          if (property.key() == "mesh")
          {
            child_node->m_mesh = std::make_shared<mesh>(store_mesh(property.value().get_uint64(), doc, buffers, views, accessors, ptr_mat));
          }

          if (property.key() == "children")
          {
            simdjson::ondemand::array children_iterator = property.value();
            for (auto k : children_iterator)
              child_node->children_indices.emplace_back(k.get_uint64());
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
            child_node->matrix = mat_ptr;
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
            child_node->rotate = rot_ptr;
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
            child_node->scale = scale_ptr;
          }
          if (property.key() == "translation")
          {
            vec3 tr;
            simdjson::ondemand::array components = property.value();
            unsigned short int s = 0;
            for (auto k : components)
            {
              tr[s] = k.get_double();
              ++s;
            }
            std::shared_ptr<transformation> tr_ptr{std::make_shared<transformation>(translation_matrix(tr))};
            child_node->translate = tr_ptr;
          }
        }

        if (child_node->m_mesh)
        {
          apply_mesh_transformations(*child_node);
          primitives.reserve(primitives.size() + child_node->m_mesh->n_triangles);

          for (auto& tri : child_node->m_mesh->get_triangles())
            primitives.emplace_back(std::move(tri));
        }
        /*
        if (child_node->camera)
        {
          apply_camera_transformations(child_node);
        }
        */

        parent->children.push_back(child_node);
        initialize_tree(child_node, doc, buffers, views, accessors, ptr_mat, primitives);
      }
      ++i;
    }
  }
}

void parse_gltf( const std::string& filename, std::vector<std::shared_ptr<primitive>>& primitives
               , const std::shared_ptr<material>& ptr_mat)
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
  selected_scene = document_scene.get_uint64();
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
          scene.root_node->children_indices.emplace_back(node.get_uint64());
      }
      ++j;
    }
  } // unnamed scope

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
          byte_length = property.value().get_uint64();
        if (property.key() == "uri")
          uri = property.value().get_string();
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
          current.buffer_index = property.value().get_uint64();
        if (property.key() == "byteOffset")
          current.byte_offset = property.value().get_uint64();
        if (property.key() == "byteLength")
          current.byte_length = property.value().get_uint64();
        if (property.key() == "byteStride")
          current.byte_stride = property.value().get_uint64();
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
          current.buffer_view = property.value().get_uint64();
        if (property.key() == "byteOffset")
          current.byte_offset = property.value().get_uint64();
        if (property.key() == "componentType")
          current.component_type = property.value().get_uint64();
        if (property.key() == "normalized")
          current.is_normalized = property.value().get_bool();
        if (property.key() == "count")
          current.count = property.value().get_uint64();
        if (property.key() == "type")
          current.type = std::string(std::string_view(property.value()));
      }
      accessors.push_back(current);
    }
  } // unnamed scope

  initialize_tree(scene.root_node, doc, buffers, views, accessors, ptr_mat, primitives);
}