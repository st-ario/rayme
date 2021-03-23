#include <iostream>
#include <cmath>

#include "my_vectors.h"
#include "ray.h"
#include "elements.h"
#include "camera.h"
#include "math.h"
#include "materials.h"

color ray_color(const ray& r, const element& world, int depth)
{
  hit_record rec;

  if (depth <= 0)
    return color(0,0,0);

  double shadow_acne_treshold = 0.001;
  if (world.hit(r, shadow_acne_treshold, infinity, rec))
  {
    ray scattered;
    color attenuation;
    if (rec.ptr_mat->scatter(r, rec, attenuation, scattered))
      return attenuation * ray_color(scattered, world, depth-1);
    return color(0,0,0);
    /*
    point offset = rec.get_normal() + vec::random_unit();
    return 0.5 * ray_color(ray(rec.p, offset), world, depth - 1);
    */
  }

  // else: gradient background, depending only on the z coordinate;
  double t = 0.5 * (1 + (r.direction().z()));
  return ((1.0 - t) * color(1.0, 1.0, 1.0) + t * color(0.5, 0.7, 1.0));
}

// z = up, x = right, y = forward
int main()
{
  // World
  scene world;
  /*
  world.add(std::make_shared<sphere>(sphere{point(0,1,0), 0.5}));
  world.add(std::make_shared<sphere>(sphere{point(0,1,-100.5), 100}));
  */
  auto material_ground = std::make_shared<lambertian>(color(0.8, 0.8, 0.0));
  auto material_center = std::make_shared<lambertian>(color(0.7, 0.3, 0.3));
  auto material_left   = std::make_shared<metal>(color(0.8, 0.8, 0.8),0.3);
  auto material_right  = std::make_shared<metal>(color(0.8, 0.6, 0.2), 1.0);

  world.add(std::make_shared<sphere>(point( 0.0, 1.0, -100.5), 100.0, material_ground));
  world.add(std::make_shared<sphere>(point( 0.0, 1.0,    0.0),   0.5, material_center));
  world.add(std::make_shared<sphere>(point(-1.0, 1.0,    0.0),   0.5, material_left));
  world.add(std::make_shared<sphere>(point( 1.0, 1.0,    0.0),   0.5, material_right));

  // Camera
  camera cam;

  // Image
  const double aspect_ratio = cam.get_aspect_ratio();
  //const int image_width = 1920;
  //const int image_width = 1280;
  const int image_width = 800;
  const int image_height = static_cast<int>(image_width / aspect_ratio);
  const int samples_per_pixel = 500;
  const int max_depth = 200;

  // Render
  std::cout << "P3\n" << image_width <<  ' ' << image_height << "\n255\n";

  for (int j = 0; j< image_height; ++j)
  {
    std::cerr << "\x1b[2K" << "\rRemaining lines to render: " << (image_height - j - 1);
    std::flush(std::cerr);
    for (int i = 0; i < image_width; ++i)
    {
      color pixel_color(0,0,0);
      for (int s = 0; s < samples_per_pixel + 1; ++s)
      {
        double x_factor = (i + random_double())/(image_width-1);
        double z_factor = (j + random_double())/(image_height-1);
        ray r = cam.get_ray(x_factor, z_factor);
        pixel_color += ray_color(r, world, max_depth);
      }
      write_color(std::cout, pixel_color, samples_per_pixel); }
  }

  std::cerr << "\nDone!\n";
}