#include "render.h"
#include "meshes.h"
#include "materials.h"
#include "camera.h"
#include "integrator.h"

#include <future>
#include <random>
#include <algorithm>

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
  return blackman_harris(pair[0]) * blackman_harris(pair[1]);
}

// auxiliary function for denoising purposes, computes the albedo and the normal of the first hit
// of a camera ray
void accumulate_albedo_normal( ray& r
                             , color& albedo_color
                             , color& normal_color
                             , const bvh_tree& world)
{
  // IMPORTANT change after implementing transmissive materials
  auto rec{world.hit(r, infinity)};
  if (!rec)
    return;

  hit_properties info{rec->what()->get_info(r,rec->uvw)};

  // albedo map channels take values in [0,1], no matter whether the render image is HDR or LDR
  // TODO see if there's any noticeable difference in the denoising quality if this is tonemapped
  // rather than clamped
  albedo_color.r += clamp(info.ptr_mat()->base_color.r,0.0f,1.0f);
  albedo_color.g += clamp(info.ptr_mat()->base_color.g,0.0f,1.0f);
  albedo_color.b += clamp(info.ptr_mat()->base_color.b,0.0f,1.0f);

  // normal map channels take values in [-1,1]; they don't have to be normalized, though
  normal_color += info.snormal().to_vec3();
}

void render_tile( image* picture
                , image* albedo_map
                , image* normal_map
                , uint8_t tile_size
                , uint16_t row
                , uint16_t column
                , uint32_t samples_per_pixel
                , uint16_t min_depth
                , const camera* cam
                , const bvh_tree* world)
{
  color pixel_color{0.0f,0.0f,0.0f};
  color albedo_color{0.0f,0.0f,0.0f};
  color normal_color{0.0f,0.0f,0.0f};
  const uint16_t h_offset = column * tile_size;
  const uint16_t v_offset = row * tile_size;

  // weight for pixel reconstruction
  float total_weight{0.0f};

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
      albedo_color = {0,0,0};
      normal_color = {0,0,0};
      total_weight = 0.0f;
      uint32_t seed{uint32_t(pixel_x) << 16 | uint32_t(pixel_y)};
      sampler_2d sampler{seed};

      for (uint16_t s = 0; s < samples_per_pixel; ++s)
      {
        std::array<float,2> center_offset{sampler.rnd_float_pair()};
        ray r{cam->get_offset_ray(pixel_x, pixel_y,center_offset)};

        if (albedo_map && normal_map)
          accumulate_albedo_normal(r,albedo_color,normal_color,*world);

        uint64_t seed( pixel_x
                     | (uint32_t(pixel_y) << 16)
                     | ((uint64_t(s) ^ uint64_t(0x3436484629)) << 32));
        integrator path_integrator(seed);

        auto filter_weight{filter(center_offset)};
        total_weight += filter_weight;
        pixel_color += filter_weight * path_integrator.integrate_path(r,*world,min_depth);
      }

      pixel_color = pixel_color / total_weight;
      albedo_color /= samples_per_pixel;
      normal_color /= samples_per_pixel;

      size_t pos{(cam->get_image_width() * (v_offset + y) + h_offset + x)*3u};

      if (albedo_map && normal_map)
      {
        // red channels
        picture->   image_buffer[pos]   = pixel_color.r;
        albedo_map->image_buffer[pos]   = albedo_color.r;
        normal_map->image_buffer[pos]   = normal_color.r;

        // green channels
        ++pos;
        picture   ->image_buffer[pos] = pixel_color.g;
        albedo_map->image_buffer[pos] = albedo_color.g;
        normal_map->image_buffer[pos] = normal_color.g;

        // blue channels
        ++pos;
        picture   ->image_buffer[pos] = pixel_color.b;
        albedo_map->image_buffer[pos] = albedo_color.b;
        normal_map->image_buffer[pos] = normal_color.b;
      } else {
        picture->image_buffer[pos]   = pixel_color.r;
        picture->image_buffer[++pos] = pixel_color.g;
        picture->image_buffer[++pos] = pixel_color.b;
      }
    }
  }
}

void render_tiles_job( image* picture
                     , image* albedo_map
                     , image* normal_map
                     , std::vector<std::pair<uint16_t,uint16_t>>* cart_prod
                     , std::mutex* mtx_prod
                     , uint8_t tiles_size
                     , uint16_t samples_per_pixel
                     , uint16_t min_depth
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
      std::cout <<"\x1b[2K"<<"\rRemaining tiles to fully render: " << cart_prod->size();
      std::flush(std::cout);
      mtx_prod->unlock();
    }
    else
    {
      mtx_prod->unlock();
      return;
    }

    render_tile( picture
               , albedo_map
               , normal_map
               , tiles_size
               , pair->first
               , pair->second
               , samples_per_pixel
               , min_depth
               , cam
               , world);
  }
}

void render( image& picture
           , image* albedo_map
           , image* normal_map
           , uint16_t samples_per_pixel
           , uint16_t min_depth
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
      render_tile( &picture
                 , albedo_map
                 , normal_map
                 , tile_size
                 , pair.first
                 , pair.second
                 , samples_per_pixel
                 , min_depth
                 , &cam
                 , &world);
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
    jobs.push_back(std::async(std::launch::async,
      render_tiles_job, &picture
                      , albedo_map
                      , normal_map
                      , &cartesian_product
                      , &mtx_prod
                      , tile_size
                      , samples_per_pixel
                      , min_depth
                      , &cam
                      , &world));
  }

#endif
}