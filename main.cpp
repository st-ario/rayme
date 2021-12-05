#include "gltf_parser.h"
#include "render.h"
#include "bvh.h"
#include "camera.h"

#ifndef NO_DENOISE
#include "denoise.h"
//#define EXPORT_DENOISE_MAPS 1
#endif

#include <boost/program_options.hpp>
namespace po = boost::program_options;

void initialize_arguments( int argc
                         , char* argv[]
                         , int32_t& image_height
                         , int32_t& samples_per_pixel
                         , int32_t& min_depth
                         , std::string& input_filename
                         , std::string& output_filename
                         , bool& allowdenoise)
{
  po::options_description desc("Allowed options");
  desc.add_options()
    ("help,h",
      "output this help message")
		("input-filename,i", po::value<std::string>(&input_filename)->value_name("FILE"),
      "specify name of the input glTF 2.0 file (including extension)")
		("height,H", po::value<int32_t>(&image_height)->value_name("HEIGHT"),
      "specify height of the output image (default: 400)")
		("spp,s", po::value<int32_t>(&samples_per_pixel)->value_name("N-SAMPLES"),
      "specify the number of samples per pixel (default: 128)")
		("min-depth,d", po::value<int32_t>(&min_depth)->value_name("DEPTH"),
      "specify the minimum number of ray bounces (default: 5)")
    #ifndef NO_DENOISE
    ("no-denoise,N", "disable image denoising (enabled by default)")
    #endif
		("output-filename,o", po::value<std::string>(&output_filename)->value_name("FILENAME"),
      "specify name of the output PNG file (without extension)")
    ;

  po::positional_options_description posdesc;
  posdesc.add("input-filename",1);

  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).options(desc).positional(posdesc).run(),vm);
  po::notify(vm);

  if (!vm.count("input-filename") || vm.count("help"))
  {
    std::cout << "Usage: rayme [-i] gltf-file [options]\n";
    std::cout << "Example: rayme scene.gltf -H 1080 -s 512 -o rendered-scene\n\n";
    std::cout << desc << "\n";
    std::exit(0);
  }

  if (image_height < 0 || image_height > std::numeric_limits<uint16_t>::max())
  {
    std::cerr << "ERROR: invalid image height";
    std::exit(1);
  }
  if (samples_per_pixel < 0 || samples_per_pixel > std::numeric_limits<uint16_t>::max())
  {
    std::cerr << "ERROR: invalid number of samples per pixel";
    std::exit(1);
  }
  if (min_depth < 0 || min_depth > std::numeric_limits<uint16_t>::max())
  {
    std::cerr << "ERROR: invalid min-depth";
    std::exit(1);
  }

  if (!vm.count("height"))
    std::cout << "output image height not set, using default value: " << image_height
              << "\n";
  if (!vm.count("spp"))
    std::cout << "number of samples per pixel not set, using default value: " << samples_per_pixel
              << "\n";
  if (!vm.count("min-depth"))
    std::cout << "minumum number of ray bounces not set, using default value: " << min_depth
              << "\n";
  if (!vm.count("output-filename"))
    std::cout << "output file name not set, using default value: \"" << output_filename
              << "\"\n";
  if (vm.count("no-denoise"))
    allowdenoise = false;
}

int main(int argc, char* argv[])
{
  // process arguments
  int32_t image_height{400};
  int32_t samples_per_pixel{128};
  int32_t min_depth{5};
  std::string input_filename;
  std::string output_filename{"output"};
  bool allowdenoise{true};

  initialize_arguments( argc
                      , argv
                      , image_height
                      , samples_per_pixel
                      , min_depth
                      , input_filename
                      , output_filename
                      , allowdenoise);

  // initialize scene elements
  std::vector<std::unique_ptr<const primitive>> primitives;
  std::unique_ptr<camera> cam;

  std::cout << "\nLoading scene...\n";
  parse_gltf(input_filename, primitives, cam, static_cast<uint16_t>(image_height));

  std::cout << "Creating BVH...\n";
  bvh_tree scene_tree{std::move(primitives)};

  // begin rendering
  std::cout << "\nReady to render!\n";
  image picture(cam->get_image_width(),cam->get_image_height());

  #ifdef NO_DENOISE
  render( picture
        , nullptr
        , nullptr
        , static_cast<uint16_t>(samples_per_pixel)
        , static_cast<uint16_t>(min_depth)
        , *cam
        , scene_tree);
  #endif

  #ifndef NO_DENOISE
  image albedo_map(cam->get_image_width(),cam->get_image_height());
  image normal_map(cam->get_image_width(),cam->get_image_height());
  if (allowdenoise)
  {
    render( picture
          , &albedo_map
          , &normal_map
          , static_cast<uint16_t>(samples_per_pixel)
          , static_cast<uint16_t>(min_depth)
          , *cam
          , scene_tree);
  } else {
    render( picture
          , nullptr
          , nullptr
          , static_cast<uint16_t>(samples_per_pixel)
          , static_cast<uint16_t>(min_depth)
          , *cam
          , scene_tree);
  }
  #endif

  // denoise result
  image denoised{denoise(picture,albedo_map,normal_map)};

  // export file
  std::cout << "\nExporting file...";
  std::flush(std::cout);
  #ifndef NO_DENOISE
  #ifdef EXPORT_DENOISE_MAPS
  picture.write_to_png(output_filename + "_noisy");
  denoised.write_to_png(output_filename + "_denoised");

  albedo_map.write_to_png(output_filename + "_albedo");

  for(auto& x : normal_map.image_buffer)
    x = (x+1.0f)/2.0f;

  normal_map.write_to_png(output_filename + "_normal");
  #else
  if (allowdenoise)
    denoised.write_to_png(output_filename);
  else
    picture.write_to_png(output_filename);
  #endif
  #else
  picture.write_to_png(output_filename);
  #endif

  std::cout << "\nDone!\n";
}