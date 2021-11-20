#include "render.h"
#include "meshes.h"
#include "materials.h"
#include "camera.h"
#include "integrator.h"

#include <future>
#include <random>
#include <algorithm>

// debug macros
//#define SINGLE_SAMPLE_PP 1

color ray_color( const ray& r
               , const bvh_tree& world
               , uint16_t pixel_x
               , uint16_t pixel_y
               , uint16_t sample)
{
  constexpr uint16_t depth{0};
  return integrate_path(r,world,depth,pixel_x,pixel_y,sample);
}

#ifndef SINGLE_SAMPLE_PP
float mitchell_netravali(float x)
{
  // spline parameters configuration
  constexpr static float b{1.0f / 3.0f};
  constexpr static float c{1.0f / 3.0f};

  // Mitchell--Netravali filter formula (https://en.wikipedia.org/wiki/Mitchell%E2%80%93Netravali_filters)
  x = std::abs(2 * x);

  if (x > 1)
    return ((-b - 6.0f * c) * x*x*x
      + (6.0f * b + 30.0f * c) * x*x
      + (-12.0f * b - 48.0f * c) * x
      + (8.0f * b + 24.0f * c)) / 6.f;
  else
    return ((12.0f - 9.0f * b - 6.0f * c) * x*x*x
      + (-18.0f + 12.0f * b + 6.0f * c) * x*x
      + (6.0f - 2.0f * b)) / 6.f;

}

float blackman_harris(float x)
{
  // Blackman–Harris window (https://en.wikipedia.org/wiki/Window_function#Blackman%E2%80%93Harris_window)
  // assuming x in [0,1]
  constexpr static float a0{0.35875f};
  constexpr static float a1{0.48829f};
  constexpr static float a2{0.14128f};
  constexpr static float a3{0.01168f};
  constexpr static float four_pi{4.0f * pi};
  constexpr static float six_pi{6.0f * pi};

  return a0 - a1 * std::cos(two_pi  * x)
            + a2 * std::cos(four_pi * x)
            - a3 * std::cos(six_pi  * x);
}

float filter(const std::array<float,2>& pair)
{
  // TODO pick the filter based on the number of samples per pixel
  // (determine a good threshold for the negative tails Mitchell--Netravali to pay off)
  //return mitchell_netravali(4.0f * (pair[0] - 0.5f)) * mitchell_netravali(4.0f * (pair[1] - 0.5f));
  return blackman_harris(pair[0]) * blackman_harris(pair[1]);
}
#endif

void render_tile( image* picture
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

  #ifndef SINGLE_SAMPLE_PP
  // weight for pixel reconstruction
  float total_weight{0.0f};
  #endif

  for (uint16_t x = 0; x < tile_size; ++x)
  {
    uint16_t pixel_x{uint16_t(h_offset + x)};
    if (pixel_x > picture->get_width() - 1)
      break;
    for (uint16_t y = 0; y < tile_size; ++y)
    {
      uint16_t pixel_y{uint16_t(v_offset + y)};
      if (pixel_y > picture->get_height() - 1)
        break;

      pixel_color = {0,0,0};

      #ifndef SINGLE_SAMPLE_PP
      total_weight = 0.0f;

      for (uint16_t s = 0; s < samples_per_pixel; ++s)
      #endif
      {
        auto r_pair{cam->get_stochastic_ray(pixel_x, pixel_y)};
        ray r{r_pair.first};
        std::array<float,2> center_offset{r_pair.second};

        #ifndef SINGLE_SAMPLE_PP
        auto filter_weight{filter(center_offset)};
        total_weight += filter_weight;
        pixel_color += filter_weight * ray_color(r, *world, pixel_x, pixel_y, s);
        #else
        pixel_color += ray_color(r, *world, pixel_x, pixel_y, 0);
        #endif

      }
      #ifndef SINGLE_SAMPLE_PP
      pixel_color = pixel_color / total_weight;
      #endif

      gamma_correct(pixel_color,2.2f);

      picture->pixels[v_offset + y][h_offset + x] = pixel_color;
    }
  }
}

void render_tiles_job( image* picture
                     , std::vector<std::pair<uint16_t,uint16_t>>* cart_prod
                     , std::mutex* mtx_prod
                     , uint8_t tiles_size
                     , uint16_t samples_per_pixel
                     , const camera* cam
                     , const bvh_tree* world)
{
  while (true)
  {
    mtx_prod->lock();
    std::optional<std::pair<uint16_t,uint16_t>> pair;
    pair = (cart_prod->size() == 0) ? std::nullopt
      : std::optional<std::pair<uint16_t,uint16_t>>{cart_prod->back()};
    if (pair)
    {
      cart_prod->pop_back();
      std::cout <<"\x1b[2K"<<"\rRemaining tiles to render: " << cart_prod->size();
      std::flush(std::cout);
      mtx_prod->unlock();
    }
    else
    {
      mtx_prod->unlock();
      return;
    }

    render_tile(picture, tiles_size, pair->first, pair->second, samples_per_pixel, cam, world);
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

  // shuffle the cartesian product to reduce the chance of having clusters of costly tiles
  auto rng = std::default_random_engine {};
  std::shuffle(std::begin(cartesian_product), std::end(cartesian_product), rng);

//#define NOTPAR 1
#ifdef NOTPAR
  uint16_t counter{0};
  for (const auto pair : cartesian_product)
  {
      ++counter;
      std::cerr <<"\x1b[2K"<<"\rRemaining tiles to render: "<< (num_rows * num_columns - counter);
      std::flush(std::cerr);
      render_tile(&picture, tile_size, pair.first, pair.second, samples_per_pixel, &cam, &world);
  }
#else

  // get the number of cores available
  uint n_cores{std::thread::hardware_concurrency()};
  if (n_cores == 0)
  {
    // TODO give the user a chance to manually specify how many threads to use
    std::cout << "ERROR: unable to determine the number of CPU cores available to the system";
    exit(1);
  } else {
    std::cout << "Number of cores detected: " << n_cores << "\n";
  }

  std::mutex mtx_prod;
  std::vector<std::future<void>> jobs;

  std::cout << "Rendering in progress...\n";
  std::cout << "Rendering " << n_cores << " tiles concurrently\n";

  for (uint i = 0; i < n_cores; ++i)
  {
    jobs.push_back(std::async(std::launch::async, render_tiles_job, &picture, &cartesian_product,
     &mtx_prod, tile_size, samples_per_pixel, &cam, &world));
  }

#endif
}