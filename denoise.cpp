#include <iostream>
#include <OpenImageDenoise/oidn.hpp>

#include "camera.h"
#include "images.h"

image denoise(image& noisy)
{
  // create denoise device
  oidn::DeviceRef device = oidn::newDevice();
  device.commit();

  // create denoise filter
  oidn::FilterRef filter = device.newFilter("RT");

  // create buffer for denoised image
  std::vector<float> denoised_buffer(noisy.image_buffer.size());

  filter.setImage("color", noisy.image_buffer.data(), oidn::Format::Float3, noisy.get_width(), noisy.get_height());
  //filter.setImage("albedo", albedoPtr, oidn::Format::Float3, width, height);
  //filter.setImage("normal", normalPtr, oidn::Format::Float3, width, height);
  filter.setImage("output", denoised_buffer.data(), oidn::Format::Float3, noisy.get_width(), noisy.get_height());
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
  image res{noisy.get_width(),noisy.get_height(),std::move(denoised_buffer)};

  return res;
}