#include "gltf_parser.h"
#include "render.h"
#include "meshes.h"
#include "camera.h"

// y = up, x = right, right-handed
int main(int argc, char* argv[])
{
  std::vector<std::shared_ptr<primitive>> primitives;
  std::shared_ptr<camera> cam;

  std::string filename{argv[1]};
  parse_gltf(filename, primitives, cam);

  bvh_tree scene_tree{std::move(primitives)};

  // Image
  const int image_width = 800;
  const int image_height = static_cast<int>(image_width / cam->get_aspect_ratio());
  const int samples_per_pixel = 10;

  // Render
  image picture(image_width,image_height);
  render(picture, samples_per_pixel, *cam, scene_tree);

  std::cerr << "\nExporting file...";
  std::flush(std::cerr);

  picture.write_to_ppm("image");
  picture.write_to_png("image");

  std::cerr << "\nDone!\n";
}