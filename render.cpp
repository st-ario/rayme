#include "render.h"
#include "math.h"
#include "my_vectors.h"
#include "ray.h"
#include "elements.h"
#include "materials.h"

#include "third-parties/stb/stb_image_write.h"

// temporary solution, to use before properly dealing with lights
static color ray_color(const ray& r, const element& world, int depth, double znear, double zfar)
{
  if (depth <= 0)
    return color(0,0,0);

  auto rec = world.hit(r, znear, zfar);
  if (rec)
  {
    auto record = rec.value();
    color attenuation;
    auto scattered_ray = rec.value().ptr_mat->scatter(r, record, attenuation);
    if (scattered_ray)
      return attenuation * ray_color(scattered_ray.value(), world, depth-1, znear, zfar);
    return color(0,0,0);
  }

  // else: gradient background, depending only on the y coordinate;
  double t = 0.5 * (1 + (r.direction.y()));
  return ((1.0 - t) * color(1.0, 1.0, 1.0) + t * color(0.5, 0.7, 1.0));
}

static void render_tile( image& picture
                       , int tile_size
                       , int row
                       , int column
                       , int samples_per_pixel
                       , int depth
                       , const camera& cam
                       , const element& world)
{
  color pixel_color(0,0,0);
  int h_offset = column * tile_size;
  int v_offset = row * tile_size;

  for (int x = 0; x < tile_size; ++x)
  {
    if (h_offset + x > picture.get_width() - 1)
      break;
    for (int y = 0; y < tile_size; ++y)
    {
      if (v_offset + y > picture.get_height() - 1)
        break;
      for (int s = 0; s < samples_per_pixel + 1; ++s)
      {
        double horiz_factor = (h_offset + x + random_double())/(picture.get_width()-1);
        double vert_factor = (v_offset + y + random_double())/(picture.get_height()-1);
        ray r = cam.get_ray(horiz_factor, vert_factor);
        pixel_color += ray_color(r, world, depth, cam.get_znear(),cam.get_zfar());
      }
      pixel_color = pixel_color / samples_per_pixel;
      gamma2_correct(pixel_color);
      picture.pixels[v_offset + y][h_offset + x] = pixel_color;
    }
  }
}

// call in parallel the rendering for each tile
// write in order the elements of the vector in the file

void render( image& picture
           , int samples_per_pixel
           , int depth
           , const camera& cam
           , const element& world)
{
  const int tile_size{16};
  const int num_columns{static_cast<int>(std::ceil(double(picture.get_width() / double(tile_size))))};
  const int num_rows{static_cast<int>(std::ceil(double(picture.get_height() / double(tile_size))))};

  int counter{0};

  for (int r = 0; r < num_rows; ++r)
  {
    for(int c = 0; c < num_columns; ++c)
    {
      ++counter;
      std::cerr << "\x1b[2K" << "\rRemaining tiles to render: " << (num_rows * num_columns - counter);
      std::flush(std::cerr);
      render_tile(picture, tile_size, r, c, samples_per_pixel, depth, cam, world);
    }
  }
}