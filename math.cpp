#include "math.h"
#include "extern/glm/glm/gtx/norm.hpp"

#include <random>

float random_float()
{
    static std::uniform_real_distribution<float> distribution(0.0, 1.0);
    static std::mt19937_64 generator;
    return distribution(generator);
}

float random_float(float min, float max)
{
    static std::uniform_real_distribution<float> distribution(min, max);
    static std::mt19937_64 generator;
    return distribution(generator);
}

float standard_normal_random_float()
{
  static std::normal_distribution<float> distribution(0,1);
  static std::mt19937_64 generator;
  return distribution(generator);
}

float fast_inverse_sqrt(float x) // https://en.wikipedia.org/wiki/Fast_inverse_square_root
{
  constexpr float threehalfs{1.5f};

  float x2{x * 0.5f};
  float y{x};

  long int i;
  i = *(long*) &y;
  i = 0x5F375A86 - (i >> 1);
  y = *(float*) &i;
  y  = y * ( threehalfs - ( x2 * y * y ) );
  y  = y * ( threehalfs - ( x2 * y * y ) ); // decide wether to keep this or not

  return y;
}

//base64 decode snippet from https://stackoverflow.com/a/13935718
namespace base64
{
  typedef unsigned char BYTE;

  static const std::string base64_chars =
               "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
               "abcdefghijklmnopqrstuvwxyz"
               "0123456789+/";


  static inline bool is_base64(BYTE c)
  {
    return (isalnum(c) || (c == '+') || (c == '/'));
  }

  std::vector<BYTE> decode(const std::string_view& encoded_string)
  {
    int in_len = encoded_string.size();
    int i = 0;
    int j = 0;
    int in_ = 0;
    BYTE char_array_4[4], char_array_3[3];
    std::vector<BYTE> ret;

    while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
      char_array_4[i++] = encoded_string[in_]; in_++;
      if (i ==4) {
        for (i = 0; i <4; i++)
          char_array_4[i] = base64_chars.find(char_array_4[i]);

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (i = 0; (i < 3); i++)
            ret.push_back(char_array_3[i]);
        i = 0;
      }
    }

    if (i) {
      for (j = i; j <4; j++)
        char_array_4[j] = 0;

      for (j = 0; j <4; j++)
        char_array_4[j] = base64_chars.find(char_array_4[j]);

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (j = 0; (j < i - 1); j++) ret.push_back(char_array_3[j]);
    }

    return ret;
  }
} // namespace base64

// vectors utility functions

vec3 random_vec3()
{
  return vec3(random_float(), random_float(), random_float());
}

vec3 random_vec3(float min, float max)
{
  return vec3(random_float(min,max), random_float(min,max), random_float(min,max));
}

normed_vec3 normed_vec3::random_unit()
// computed normalizing standard Gaussians for each coordinate to get the uniform distribution on the surface
{
  float rx = standard_normal_random_float();
  float ry = standard_normal_random_float();
  float rz = standard_normal_random_float();
  float inverse_norm = fast_inverse_sqrt(rx*rx + ry*ry + rz*rz);
  return normed_vec3(rx*inverse_norm,ry*inverse_norm,rz*inverse_norm);
}

vec3 random_vec3_in_unit_sphere()
{
  float random_radius = random_float();
  return random_radius * static_cast<vec3>(normed_vec3::random_unit());
}

normed_vec3 reflect(const normed_vec3& incident, const normed_vec3& normal)
{
  vec3 i = static_cast<vec3>(incident);
  vec3 n = static_cast<vec3>(normal);

  return unit(i - 2 * glm::dot(i,n) * n);
}

normed_vec3 refract(const normed_vec3& incident, const normed_vec3& normal, float refractive_indices_ratio)
{
  vec3 i = static_cast<vec3>(incident);
  vec3 n = static_cast<vec3>(normal);

  float cos_incidence_angle = glm::dot(-i,n);
  vec3 refracted_perp = refractive_indices_ratio * (i + cos_incidence_angle * n);
  vec3 refracted_parallel = - std::sqrt(1.0f - glm::length2(refracted_perp)) * n;
  return unit(refracted_perp + refracted_parallel);
}