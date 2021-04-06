#include "images.h"
#include "math.h"
#include <fstream>

image::image(int pixel_width, int pixel_height) : width{pixel_width}, height{pixel_height}
{
  for (int r = 0; r < pixel_height; ++r)
  {
    std::vector<color> row;
    for (int c = 0; c < pixel_width; ++c)
    {
      row.push_back(color(0,0,0));
    }
    pixels.push_back(row);
  }
}

void image::write_to_ppm(std::string file_name)
{
  std::ofstream image_file;
  image_file.open(file_name);

  image_file << "P3\n" << width <<  ' ' << height << "\n255\n";
  int ppm_red;
  int ppm_green;
  int ppm_blue;

  for (std::vector<color> row : pixels)
  {
    for (color c : row)
    {
      ppm_red = static_cast<int>(255 * clamp(c.r(), 0, 1.0));
      ppm_green = static_cast<int>(255 * clamp(c.g(), 0, 1.0));
      ppm_blue = static_cast<int>(255 * clamp(c.b(), 0, 1.0));
      image_file << std::string{std::to_string(ppm_red) + ' ' + std::to_string(ppm_green) + ' ' + std::to_string(ppm_blue) + '\n'};
    }
  }

  image_file.close();
}

int image::get_height() const { return height; }
int image::get_width() const { return width; }