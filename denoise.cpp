#include <iostream>
#include <OpenImageDenoise/oidn.hpp>

#include "camera.h"
#include "images.h"

image denoise(image& noisy, image& albedo_map, image& normal_map)
{
  // create denoise device
  oidn::DeviceRef device = oidn::newDevice();
  device.commit();

  // create denoise filter
  oidn::FilterRef filter = device.newFilter("RT");

  // create buffer for denoised image
  std::vector<float> denoised_buffer(noisy.image_buffer.size());

  filter.setImage("color", noisy.image_buffer.data(), oidn::Format::Float3
    , noisy.get_width(), noisy.get_height());
  filter.setImage("albedo", albedo_map.image_buffer.data(), oidn::Format::Float3
    , noisy.get_width(), noisy.get_height());
  filter.setImage("normal", normal_map.image_buffer.data(), oidn::Format::Float3
    , noisy.get_width(), noisy.get_height());
  filter.setImage("output", denoised_buffer.data(), oidn::Format::Float3
    , noisy.get_width(), noisy.get_height());
  filter.set("hdr", true); // beauty image is HDR
  filter.set("srgb", false); // beauty image is in linear space
  filter.set("cleanAux", true); // auxiliary images will be prefiltered
  filter.commit();

  // separate filter for denoising the albedo map (in-place)
  oidn::FilterRef albedoFilter = device.newFilter("RT");
  albedoFilter.setImage("albedo", albedo_map.image_buffer.data(), oidn::Format::Float3
    , noisy.get_width(), noisy.get_height());
  albedoFilter.setImage("output", albedo_map.image_buffer.data(), oidn::Format::Float3
    , noisy.get_width(), noisy.get_height());
  albedoFilter.commit();

  // separate filter for denoising the normal map (in-place)
  oidn::FilterRef normalFilter = device.newFilter("RT");
  normalFilter.setImage("normal", normal_map.image_buffer.data(), oidn::Format::Float3
    , noisy.get_width(), noisy.get_height());
  normalFilter.setImage("output", normal_map.image_buffer.data(), oidn::Format::Float3
    , noisy.get_width(), noisy.get_height());
  normalFilter.commit();

  // prefilter auxiliary maps
  albedoFilter.execute();
  normalFilter.execute();

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