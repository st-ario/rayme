#include "render.h"
#include "meshes.h"
#include "materials.h"
#include "camera.h"
#include "integrator.h"

#include <future>

static color ray_color(const ray& r, const element& world)
{
  const uint16_t integration_samples_N{300};
  const uint16_t depth{0};
  return sample_bdf(r,world,integration_samples_N,0);
  //return sample_lights(r,world,integration_samples_N,0);
}

static void render_tile( image* picture
                       , uint8_t tile_size
                       , uint16_t row
                       , uint16_t column
                       , uint16_t samples_per_pixel
                       , const camera* cam
                       , const bvh_tree* world)
{
  color pixel_color(0,0,0);
  const uint16_t h_offset = column * tile_size;
  const uint16_t v_offset = row * tile_size;

  for (uint16_t x = 0; x < tile_size; ++x)
  {
    if (h_offset + x > picture->get_width() - 1)
      break;
    for (uint16_t y = 0; y < tile_size; ++y)
    {
      if (v_offset + y > picture->get_height() - 1)
        break;
      pixel_color = {0,0,0};
      for (uint16_t s = 0; s < samples_per_pixel; ++s)
      {
        ray r{cam->get_ray(h_offset + x, v_offset + y)};
        //ray r{cam->get_stochastic_ray(h_offset + x, v_offset + y)};
        pixel_color += ray_color(r, *world);
      }
      pixel_color = pixel_color / static_cast<float>(samples_per_pixel);
      gamma_correct(pixel_color,3);

      picture->pixels[v_offset + y][h_offset + x] = pixel_color;
    }
  }
}

void render( image& picture
           , uint16_t samples_per_pixel
           , const camera& cam
           , const bvh_tree& world)
{
  const uint8_t tile_size{16};
  const uint16_t num_columns{static_cast<uint16_t>(
    std::ceil(static_cast<float>(picture.get_width() / static_cast<float>(tile_size)))
    )};
  const uint16_t num_rows{static_cast<uint16_t>(
    std::ceil(static_cast<float>(picture.get_height() / static_cast<float>(tile_size)))
    )};


  std::vector<std::pair<uint16_t,uint16_t>> cartesian_product;

  for (uint16_t r = 0; r < num_rows; ++r)
  {
    for(uint16_t c = 0; c < num_columns; ++c)
    {
      cartesian_product.emplace_back(std::make_pair(r,c));
    }
  }

//#define NOTPAR 1
#ifdef NOTPAR
  uint16_t counter{0};
  for (const auto pair : cartesian_product)
  {
      ++counter;
      std::cerr << "\x1b[2K" << "\rRemaining tiles to render: " << (num_rows * num_columns - counter);
      std::flush(std::cerr);
      render_tile(&picture, tile_size, pair.first, pair.second, samples_per_pixel, &cam, &world);
  }
#else
  std::vector<std::future<void>> future_tiles;

  std::cout << "Rendering in progress...\n";

  for (const auto& pair : cartesian_product)
  {
    future_tiles.push_back(std::async(std::launch::async, render_tile, &picture, tile_size, pair.first, pair.second, samples_per_pixel, &cam, &world));
  }
#endif
}