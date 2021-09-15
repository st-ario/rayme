#include "gltf_parser.h"
#include "render.h"
#include "meshes.h"
#include "camera.h"

int main(int argc, char* argv[])
{
  const uint16_t image_height = 800;

  std::vector<std::shared_ptr<const primitive>> primitives;
  std::shared_ptr<camera> cam;

  std::string filename{argv[1]};
  parse_gltf(filename, primitives, cam, image_height);

  bvh_tree scene_tree{primitives};

  const uint16_t samples_per_pixel = 1;

  image picture(cam->get_image_width(),cam->get_image_height());
  render(picture, samples_per_pixel, *cam, scene_tree);

  std::cerr << "\nExporting file...";
  std::flush(std::cerr);

  picture.write_to_png("image");

  std::cerr << "\nDone!\n";
}