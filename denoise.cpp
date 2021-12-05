#include <iostream>
#include <OpenImageDenoise/oidn.hpp>

#include "camera.h"
#include "images.h"

image denoise(const camera& cam, const image& noisy)
{
  // TODO make it more efficient by avoiding copying buffers twice

  // create denoise device
  oidn::DeviceRef device = oidn::newDevice();
  device.commit();

  // create denoise filter
  oidn::FilterRef filter = device.newFilter("RT");

  // flatten image
  const size_t height{noisy.get_height()};
  const size_t width{noisy.get_width()};
  const size_t len{height * width * 3u};
  std::vector<float> noisy_buffer(len);

  for (size_t i = 0; i < len; i+=3)
  {
    size_t j{i/3};
    size_t row{j/width};
    size_t col{j%width};
    noisy_buffer[i]   = clamp(noisy.pixels[j/width][j%width].r,0.0f,1.0f);
    noisy_buffer[i+1] = clamp(noisy.pixels[j/width][j%width].g,0.0f,1.0f);
    noisy_buffer[i+2] = clamp(noisy.pixels[j/width][j%width].b,0.0f,1.0f);
  }

  // create buffer for denoised image
  std::vector<float> denoised_buffer(len);

  filter.setImage("color", noisy_buffer.data(), oidn::Format::Float3, width, height);
  //filter.setImage("albedo", albedoPtr, oidn::Format::Float3, width, height);
  //filter.setImage("normal", normalPtr, oidn::Format::Float3, width, height);
  filter.setImage("output", denoised_buffer.data(), oidn::Format::Float3, width, height);
  filter.set("hdr", false);
  filter.set("srgb", true); // TODO test if the result is better when gamma correcting at the very end
  filter.commit();

  // filter the image
  filter.execute();

  // make sure it worked
  const char* errorMessage;
  if(device.getError(errorMessage) != oidn::Error::None)
  {
    std::cerr << "ERROR: " << errorMessage;
    std::exit(1);
  }

  // convert flat representation to image object
  image res{noisy.get_width(),noisy.get_height(),denoised_buffer.data()};

  return res;
}