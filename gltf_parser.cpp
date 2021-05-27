#include "gltf_parser.h"
#include "meshes.h"
#include "camera.h"
#include "extern/simdjson/singleheader/simdjson.h"

union  gltf_numeric { int i; float f; };
using  gltf_buffer = std::vector<unsigned char>;

struct gltf_node
{
  std::vector<int> children_indices;
  std::vector<std::shared_ptr<gltf_node>> children;
  std::weak_ptr<gltf_node> parent;
  std::shared_ptr<mesh> m_mesh;
  std::shared_ptr<camera> cam;
  std::shared_ptr<transformation> transform;
};

struct raw_gltf_node
{
  bool has_camera = false;
  bool has_mesh = false;
  bool has_children = false;
  bool has_matrix = false;
  bool has_rotation = false;
  bool has_scale = false;
  bool has_translation = false;
  int camera;
  int mesh;
  mat4 matrix;
  vec4 rotation;
  vec3 scale;
  vec3 translation;
  std::vector<int> children;
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
  transformation id;
  if (node.transform && *(node.transform) != id)
    apply_pointwise_transformation(*(node.transform), mesh);

  if (auto p_p = node.parent.lock())
    apply_mesh_transformations(*p_p, mesh);
}

void apply_mesh_transformations(gltf_node& node)
{
  transformation id;
  if (node.transform && *(node.transform) != id)
    apply_pointwise_transformation(*(node.transform), *(node.m_mesh));

  if (auto p_p = node.parent.lock())
    apply_mesh_transformations(*p_p, *(node.m_mesh));
}

void apply_camera_transformations(gltf_node& node, camera& camera)
{
  transformation id;
  if (node.transform && *(node.transform) != id)
    camera.transform_by(*(node.transform));

  if (auto p_p = node.parent.lock())
    apply_camera_transformations(*p_p, camera);
}

void apply_camera_transformations(gltf_node& node)
{
  transformation id;
  if (node.transform && *(node.transform) != id)
    node.cam->transform_by(*(node.transform));

  if (auto p_p = node.parent.lock())
    apply_camera_transformations(*p_p, *(node.cam));
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
      {
        ++j;
        break;
      }
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
  std::cerr << "ERROR: unable to find mesh\n";
  std::exit(1);
}

camera store_camera(int camera_index, simdjson::ondemand::document& doc)
{
  simdjson::ondemand::array doc_cameras;
  auto error = doc["cameras"].get(doc_cameras);
  if (error)
  {
    std::cerr << "ERROR: invalid cameras\n";
    std::exit(1);
  }
  int i = 0;
  for (auto doc_camera : doc_cameras)
  {
    if (i == camera_index)
    {
      simdjson::ondemand::object cam_obj = doc_camera.get_object();
      // type is a required field
      if (cam_obj["type"].get_string() == std::string_view("orthographic"))
      {
        std::cerr << "ERROR: orthographic camera detected, ";
        std::cerr << "currently only perspective cameras are supported\n";
        std::exit(2);
      } else if (cam_obj["type"].get_string() != std::string_view("perspective")) {
        std::cerr << "ERROR: invalid camera\n";
        std::exit(1);
      }
      simdjson::ondemand::object persp_obj = cam_obj["perspective"].get_object();
      float yfov = persp_obj["yfov"].get_double();
      float znear = persp_obj["znear"].get_double();
      camera cam{yfov,znear};
      double aspect_ratio{-1};
      error = persp_obj["aspectRatio"].get(aspect_ratio);
      if (!error)
        cam.set_aspect_ratio(aspect_ratio);
      double zfar{-1};
      error = persp_obj["zfar"].get(zfar);
      if (!error)
        cam.set_zfar(zfar);

      return cam;
    }
    ++i;
  }
  std::cerr << "ERROR: unable to find camera\n";
  std::exit(1);
}

// the ptr_mat argument is a temporary hack, to pass to all meshes the same standard material for
// rendering, to be removed as soon as materials are properly dealt with
void initialize_tree( std::shared_ptr<gltf_node>& parent
                    , simdjson::ondemand::document& doc
                    , const std::vector<raw_gltf_node>& raw_nodes
                    , const std::vector<gltf_buffer>& buffers
                    , const std::vector<buffer_view>& views
                    , const std::vector<accessor>& accessors
                    , const std::shared_ptr<material>& ptr_mat
                    , std::vector<std::shared_ptr<primitive>>& primitives
                    , std::shared_ptr<camera>& cam)
{
  for (int child_index : parent->children_indices)
  {
    auto current_raw_node = raw_nodes[child_index];

    std::shared_ptr<gltf_node> child_node{std::make_shared<gltf_node>()};
    child_node->parent = parent;

    // get total transformation matrix the node
    // default = identity
    std::shared_ptr<transformation> transform_ptr = std::make_shared<transformation>();
    bool matrix_set = false;

    // get transformation matrix, if provided
    if (current_raw_node.has_matrix)
    {
      transform_ptr = std::make_shared<transformation>(current_raw_node.matrix);
      matrix_set = true;
    }

    // get TRS matrix, if TRS transformations are provided
    // (by the glTF 2.0 standard, this can happen only if "matrix" is not set)
    if (!matrix_set)
    {
      // get scale matrix
      transformation tr_scale;
      if (current_raw_node.has_scale)
        tr_scale = scale_matrix(current_raw_node.scale);

      // get rotation matrix
      transformation tr_rotation;
      if (current_raw_node.has_rotation)
        tr_rotation = rotation_matrix(current_raw_node.rotation);

      // get translation matrix
      transformation tr_translation;
      if (current_raw_node.has_translation)
        tr_translation = translation_matrix(current_raw_node.translation);

      transformation id;
      transformation total;
      if (tr_translation != id || tr_rotation != id || tr_scale != id)
      {
        total = tr_translation * tr_rotation * tr_scale;
      }
      if (total != id)
      {
        transform_ptr = std::make_shared<transformation>(total);
      }
    }

    child_node->transform = transform_ptr;

    // get childred indices
    if (current_raw_node.has_children)
    {
      child_node->children_indices = current_raw_node.children;
      initialize_tree(child_node,doc, raw_nodes, buffers, views, accessors, ptr_mat, primitives, cam);
    }

    // process mesh
    if (current_raw_node.has_mesh)
    {
      child_node->m_mesh = std::make_shared<mesh>(store_mesh(current_raw_node.mesh,doc,buffers,views,accessors,ptr_mat));
      apply_mesh_transformations(*child_node);
      primitives.reserve(primitives.size() + child_node->m_mesh->n_triangles);

      for (auto& tri : child_node->m_mesh->get_triangles())
        primitives.emplace_back(std::move(tri));
    }

    // process camera
    if (current_raw_node.has_camera)
    {
      child_node->cam = std::make_shared<camera>(store_camera(current_raw_node.camera,doc));
      apply_camera_transformations(*child_node);
      cam = child_node->cam;
    }

    parent->children.push_back(child_node);
  }
}

void parse_gltf( const std::string& filename
               , std::vector<std::shared_ptr<primitive>>& primitives
               , std::shared_ptr<camera>& cam
               , const std::shared_ptr<material>& ptr_mat)
{
  simdjson::ondemand::parser parser;
  auto gltf = simdjson::padded_string::load(filename);
  simdjson::ondemand::document doc = parser.iterate(gltf);

  // check version
  {
    std::string_view version;
    auto error = doc["asset"]["version"].get(version);
    if (error)
    {
      std::cerr << "invalid glTF document\n";
      std::exit(1);
    }
    if (version != "2.0")
    {
      std::cerr << "unsupported glTF version\n";
      std::exit(1);
    }
    if (version == "2.0")
    {
      std::cout << "glTF 2.0 file detected\n";
    }
  }

  // get root nodes of the scene
  gltf_scene scene;
  { // unnamed scope
    uint64_t selected_scene;
    auto error = doc["scene"].get(selected_scene);
    if (error)
    {
        std::cerr << "ERROR: missing \"scene\" field\n";
        std::exit(1);
    }

    simdjson::ondemand::array document_scenes;
    error = doc["scenes"].get(document_scenes);
    if (error)
    {
        std::cerr << "ERROR: invalid scenes\n";
        std::exit(1);
    }

    int j = 0;
    for (auto s : document_scenes)
    {
      if (j == selected_scene)
      {
        auto scene_obj = s.get_object();
        simdjson::ondemand::array scene_nodes;
        error = scene_obj["nodes"].get(scene_nodes);
        if (error)
        {
          std::cerr << "ERROR: invalid nodes\n";
          std::exit(1);
        }
        for (auto node : scene_nodes)
          scene.root_node->children_indices.emplace_back(node.get_uint64());
      }
      ++j;
    }
  } // unnamed scope

  // store buffers
  std::vector<gltf_buffer> buffers;
  { // unnamed scope
    simdjson::ondemand::array document_buffers;
    auto error = doc["buffers"].get(document_buffers);
    if (error)
    {
      std::cerr << "ERROR: invalid buffers\n";
      std::exit(1);
    }

    for (auto document_buffer : document_buffers)
    {
      auto buffer_obj = document_buffer.get_object();
      int byte_length;
      std::string_view uri;
      for (auto property : buffer_obj)
      {
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
          std::exit(1);
        }
        current.insert(current.begin(),
                       std::istreambuf_iterator<char>(file),
                       std::istreambuf_iterator<char>());

        file.close();
      }
      buffers.emplace_back(std::move(current));
    }
  } // unnamed scope

  // store buffer views
  std::vector<buffer_view> views;
  { // unnamed scope
    simdjson::ondemand::array document_views;
    auto error = doc["bufferViews"].get(document_views);
    if (error)
    {
      std::cerr << "ERROR: invalid buffer views\n";
      std::exit(1);
    }
    for (auto b : document_views)
    {
      auto b_view_obj = b.get_object();
      buffer_view current;

      for (auto property : b_view_obj)
      {
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

  // store accessors
  std::vector<accessor> accessors;
  { // unnamed scope
    simdjson::ondemand::array document_accessors;
    auto error = doc["accessors"].get(document_accessors);
    if (error)
    {
      std::cerr << "ERROR: invalid accessors\n";
      std::exit(1);
    }
    for (auto a : document_accessors)
    {
      auto acc_obj = a.get_object();
      accessor current;

      for (auto property : acc_obj)
      {
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

  // store local representation of nodes, for recursive traversal
  std::vector<raw_gltf_node> raw_nodes;
  { // unnamed scope
    simdjson::ondemand::array doc_nodes;
    auto error = doc["nodes"].get(doc_nodes);
    if (error)
    {
      std::cerr << "ERROR: invalid nodes\n";
      std::exit(1);
    }
    for (auto node_iterator : doc_nodes)
    {
      auto node_obj = node_iterator.get_object();
      raw_gltf_node current_node;
      { // camera
        uint64_t camera;
        error = node_obj["camera"].get(camera);
        if (!error)
        {
          current_node.has_camera = true;
          current_node.camera = camera;
        }
      }
      { // mesh
        uint64_t mesh;
        error = node_obj["mesh"].get(mesh);
        if (!error)
        {
          current_node.has_mesh = true;
          current_node.mesh = mesh;
        }
      }
      { // children
        simdjson::ondemand::array children;
        error = node_obj["children"].get(children);
        if (!error)
        {
          current_node.has_children = true;
          for (uint64_t k : children)
            current_node.children.push_back(k);
        }
      }
      { // matrix
        simdjson::ondemand::array matrix;
        error = node_obj["matrix"].get(matrix);
        if (!error)
        {
          current_node.has_matrix = true;
          int i = 0;
          for (double k : matrix)
          {
            current_node.matrix[i] = k;
            ++i;
          }
        }
      }
      { // rotation
        simdjson::ondemand::array rotation;
        error = node_obj["rotation"].get(rotation);
        if (!error)
        {
          current_node.has_rotation = true;
          int i = 0;
          for (double k : rotation)
          {
            current_node.rotation[i] = k;
            ++i;
          }
        }
      }
      { // scale
        simdjson::ondemand::array scale;
        error = node_obj["scale"].get(scale);
        if (!error)
        {
          current_node.has_scale = true;
          int i = 0;
          for (double k : scale)
          {
            current_node.scale[i] = k;
            ++i;
          }
        }
      }
      { // translation
        simdjson::ondemand::array translation;
        error = node_obj["translation"].get(translation);
        if (!error)
        {
          current_node.has_translation = true;
          int i = 0;
          for (double k : translation)
          {
            current_node.translation[i] = k;
            ++i;
          }
        }
      }
      raw_nodes.push_back(current_node);
    }
  } // unnamed scope

  // recursively process the scene tree
  initialize_tree(scene.root_node, doc, raw_nodes, buffers, views, accessors, ptr_mat, primitives, cam);
}