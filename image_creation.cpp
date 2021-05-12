#include <iostream>
#include <fstream>
#include <cmath>

#include "render.h"
#include "my_vectors.h"
#include "ray.h"
#include "camera.h"
#include "math.h"
#include "materials.h"
#include "images.h"
#include "gltf_parser.h"

// y = up, x = right, right-handed
int main()
{
  // World
  scene world;

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

  world.add(std::make_shared<sphere>(point( 0.0, -100.5, -1.0), 100.0, material_ground));
  //world.add(std::make_shared<sphere>(point( 0.0,    0.0, -1.0),   0.5, material_center));
  // currently parse_gltf just loads triangle meshes
  //parse_gltf("Box(bin_ref).gltf", world, material_center);
  //parse_gltf("untitled.gltf", world, material_center);
  parse_gltf("torus.gltf", world, material_center);

  // Camera
  const float aspect_ratio = 16.0f/9.0f;
  const float focal_length = 1.0f;
  //float shadow_acne_treshold = 0.001f;
  float shadow_acne_treshold = 0.0f;
  camera cam(point(-2,2,1), vec3(0,0,-1), 90.0f, aspect_ratio, vec3(0,1,0), shadow_acne_treshold);

  // Image
  const int image_width = 500;
  const int image_height = static_cast<int>(image_width / aspect_ratio);
  const int samples_per_pixel = 10;
  const int max_depth = 5;

  // Render

  image picture(image_width,image_height);
  render(picture, samples_per_pixel, max_depth, cam, world);

  std::cerr << "\nExporting file...";
  std::flush(std::cerr);

  picture.write_to_ppm("image");
  picture.write_to_png("image");

  std::cerr << "\nDone!\n";
}