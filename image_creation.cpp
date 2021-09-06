#include "gltf_parser.h"
#include "render.h"

// y = up, x = right, right-handed
int main()
{
  std::vector<std::shared_ptr<primitive>> primitives;

  auto material_ground = std::make_shared<lambertian>(color(0.8, 0.8, 0.0));
  auto material_center = std::make_shared<lambertian>(color(0.1, 0.2, 0.5));
  //auto material_left   = std::make_shared<dielectric>(1.5);
  auto material_right  = std::make_shared<metal>(color(0.8, 0.6, 0.2), 0.0);
  auto material_left  = std::make_shared<metal>(color(0.1, 0.1, 0.2), 0.0);

  /*
  world.add(std::make_shared<sphere>(point( 0.0, -100.5, -1.0), 100.0, material_ground));
  //world.add(std::make_shared<sphere>(point( 0.0,    0.0, -1.0),   0.5, material_center));
  world.add(std::make_shared<sphere>(point(-1.0,    0.0, -1.0),   0.5, material_left));
  //world.add(std::make_shared<sphere>(point(-1.0,    0.0, -1.0), -0.45, material_left));
  world.add(std::make_shared<sphere>(point( 1.0,    0.0, -1.0),   0.5, material_right));
  */

  // Debugging camera: gets replaced by parse_gltf
  // remove eventually
  const float aspect_ratio = 16.0f/9.0f;
  std::shared_ptr<camera> cam{std::make_shared<camera>(point(2,1.5,1), vec3(0,0,-1), 50.0f, aspect_ratio, vec3(0,1,0))};
  // std::shared_ptr<camera> cam{std::make_shared<camera>(point(-2,2,1), vec3(0,0,-1), 50.0f, aspect_ratio, vec3(0,1,0))};
  // std::shared_ptr<camera> cam{std::make_shared<camera>(point(-6,6,5), vec3(0,0,-1), 50.0f, aspect_ratio, vec3(0,1,0))};

  //primitives.emplace_back(std::make_shared<sphere>(point( 0.0, -100.5, -1.0), 100.0, material_ground));
  //primitives.emplace_back(std::make_shared<sphere>(point( 0.0,    0.0, -1.0),   0.5, material_center));
  // currently parse_gltf just loads triangle meshes
  parse_gltf("camera_experiment.gltf", primitives, cam, material_center);

  bvh_tree scene_tree{std::move(primitives)};

  // Image
  const int image_width = 800;
  const int image_height = static_cast<int>(image_width / cam->get_aspect_ratio());
  const int samples_per_pixel = 10;
  const int max_depth = 5;

  // Render
  image picture(image_width,image_height);
  render(picture, samples_per_pixel, max_depth, *cam, scene_tree);

  std::cerr << "\nExporting file...";
  std::flush(std::cerr);

  picture.write_to_ppm("image");
  picture.write_to_png("image");

  std::cerr << "\nDone!\n";
}