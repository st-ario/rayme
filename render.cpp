#include "render.h"
#include "meshes.h"
#include "materials.h"
#include "camera.h"

#include <future>

static color ray_color(const ray& r, const element& world)
{
  color pixel_color;
  auto rec = world.hit(r, infinity);

  if (!rec)
    return {0,0,0};

  hit_record record = rec.value().first->get_record(r,rec.value().second);

  if (record.ptr_mat->emitter)
    pixel_color = record.ptr_mat->emissive_factor;

  return pixel_color;
}

static void render_tile( image* picture
                       , int tile_size
                       , int row
                       , int column
                       , int samples_per_pixel
                       , const camera* cam
                       , const bvh_tree* world)
{
  color pixel_color(0,0,0);
  const int h_offset = column * tile_size;
  const int v_offset = row * tile_size;

  for (int x = 0; x < tile_size; ++x)
  {
    if (h_offset + x > picture->get_width() - 1)
      break;
    for (int y = 0; y < tile_size; ++y)
    {
      if (v_offset + y > picture->get_height() - 1)
        break;
      pixel_color = {0,0,0};
      for (int s = 0; s < samples_per_pixel; ++s)
      {
        float horiz_wiggle = static_cast<float>(h_offset + x) / static_cast<float>(picture->get_height());
        float vert_wiggle = static_cast<float>(v_offset + y) / static_cast<float>(picture->get_width());
        float horiz_factor = (h_offset + x + random_float(-horiz_wiggle, 1.0f - horiz_wiggle))/(picture->get_width()-1);
        float vert_factor = (v_offset + y + random_float(-vert_wiggle, 1.0f - vert_wiggle))/(picture->get_height()-1);
        ray r = cam->get_ray(horiz_factor, vert_factor,picture->get_height());
        pixel_color += ray_color(r, *world);
      }
      pixel_color = pixel_color / double(samples_per_pixel);
      gamma2_correct(pixel_color);

      picture->pixels[v_offset + y][h_offset + x] = pixel_color;
    }
  }
}

void render( image& picture
           , int samples_per_pixel
           , const camera& cam
           , const bvh_tree& world)
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