#include <future>

#include "render.h"
#include "math.h"
#include "my_vectors.h"
#include "ray.h"
#include "elements.h"
#include "materials.h"

// temporary solution, to use before properly dealing with lights
static color ray_color(const ray& r, const element& world, int depth, float zfar)
{
  if (depth < 1)
    return color(0,0,0);

  auto rec = world.hit(r, zfar);
  if (rec)
  {
    auto record = rec.value();
    color attenuation;
    auto scattered_ray = rec.value().ptr_mat->scatter(r, record, attenuation);
    if (scattered_ray)
      return attenuation * ray_color(scattered_ray.value(), world, depth-1, zfar);
    return color(0,0,0);
  }

  // else: gradient background, depending only on the y coordinate;
  double t = 0.5 * (1 + (r.direction.y()));
  return ((1.0 - t) * color(1.0, 1.0, 1.0) + t * color(0.5, 0.7, 1.0));
}

static void render_tile( image* picture
                       , int tile_size
                       , int row
                       , int column
                       , int samples_per_pixel
                       , int depth
                       , const camera* cam
                       , const element* world)
{
  color pixel_color(0,0,0);
  int h_offset = column * tile_size;
  int v_offset = row * tile_size;

  for (int x = 0; x < tile_size; ++x)
  {
    if (h_offset + x > picture->get_width() - 1)
      break;
    for (int y = 0; y < tile_size; ++y)
    {
      if (v_offset + y > picture->get_height() - 1)
        break;
      for (int s = 0; s < samples_per_pixel + 1; ++s)
      {
        float horiz_factor = (h_offset + x + random_float())/(picture->get_width()-1);
        float vert_factor = (v_offset + y + random_float())/(picture->get_height()-1);
        ray r = cam->get_ray(horiz_factor, vert_factor);
        pixel_color += ray_color(r, *world, depth, cam->get_zfar());
      }
      pixel_color = pixel_color / double(samples_per_pixel);
      gamma2_correct(pixel_color);

      picture->pixels[v_offset + y][h_offset + x] = pixel_color;
    }
  }
}

void render( image& picture
           , int samples_per_pixel
           , int depth
           , const camera& cam
           , const element& world)
{
  const int tile_size{16};
  const int num_columns{static_cast<int>(std::ceil(float(picture.get_width() / float(tile_size))))};
  const int num_rows{static_cast<int>(std::ceil(float(picture.get_height() / float(tile_size))))};


  std::vector<std::pair<int,int>> cartesian_product;

  for (int r = 0; r < num_rows; ++r)
  {
    for(int c = 0; c < num_columns; ++c)
    {
      cartesian_product.emplace_back(std::make_pair(r,c));
    }
  }

//#define NOTPAR 1
#ifdef NOTPAR
  int counter{0};
  for (const auto pair : cartesian_product)
  {
      ++counter;
      std::cerr << "\x1b[2K" << "\rRemaining tiles to render: " << (num_rows * num_columns - counter);
      std::flush(std::cerr);
      render_tile(&picture, tile_size, pair.first, pair.second, samples_per_pixel, depth, &cam, &world);
  }
#else
  std::vector<std::future<void>> future_tiles;

  std::cout << "Rendering in progress...\n";

  for (const auto pair : cartesian_product)
  {
    future_tiles.push_back(std::async(std::launch::async, render_tile, &picture, tile_size, pair.first, pair.second, samples_per_pixel, depth, &cam, &world));
  }
#endif
}