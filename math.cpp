#include "math.h"
#include "extern/glm/glm/gtx/norm.hpp"

#include <random>
#include <thread>
#include <time.h>

//#define STATIC_RANDOM 1
#ifdef STATIC_RANDOM
#include "rng.h"
#endif

size_t random_size_t(size_t min, size_t max)
{
  static thread_local std::mt19937_64 generator(std::clock()
    + std::hash<std::thread::id>()(std::this_thread::get_id()));
  std::uniform_int_distribution<size_t> distribution(min, max);
  return distribution(generator);
}

float random_float(uint16_t x, uint16_t y, uint16_t z)
{
  #ifdef STATIC_RANDOM
  uint32_t hash{uint32_t(x + 2069*y)}; // simple hash: yx in base 2069 (prime)
  hash %= N_RNG_SAMPLES;
  uint16_t sample{uint16_t(z % SIZE_RNG_SAMPLES)};
  return (*samples_1d[hash])[sample];
  #else
  static thread_local std::mt19937_64 generator(std::clock()
    + std::hash<std::thread::id>()(std::this_thread::get_id()));
  static std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
  return distribution(generator);
  #endif
}

std::array<float,2> random_float_pair(uint16_t x, uint16_t y, uint16_t z)
{
  #ifdef STATIC_RANDOM
  uint32_t hash{uint32_t(x + 2069*y)}; // simple hash: yx in base 2069 (prime)
  hash %= N_RNG_SAMPLES;
  uint16_t sample{uint16_t(z % SIZE_RNG_SAMPLES)};
  return (*samples_2d[hash])[sample];
  #else
  return std::array<float,2>{random_float(x,y,z), random_float(x,y,z)};
  #endif
}

float random_float(float min, float max, uint16_t x, uint16_t y, uint16_t z)
{
  #ifdef STATIC_RANDOM
  return min + random_float(x,y,z) * max;
  #else
  static thread_local std::mt19937_64 generator(std::clock()
    + std::hash<std::thread::id>()(std::this_thread::get_id()));
  std::uniform_real_distribution<float> distribution(min, max);
  return distribution(generator);
  #endif
}

/*
float standard_normal_random_float()
{
  static thread_local std::mt19937_64 generator(std::clock()
    + std::hash<std::thread::id>()(std::this_thread::get_id()));
  static std::normal_distribution<float> distribution(0.0f,1.0f);
  return distribution(generator);
}
*/

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

normed_vec3 reflect(const normed_vec3& incident, const normed_vec3& normal)
{
  vec3 i = incident.to_vec3();
  vec3 n = normal.to_vec3();

  return unit(i - 2 * dot(i,n) * n);
}

normed_vec3 refract(const normed_vec3& incident, const normed_vec3& normal, float refractive_indices_ratio)
{
  vec3 i = incident.to_vec3();
  vec3 n = normal.to_vec3();

  float cos_incidence_angle = dot(-i,n);
  vec3 refracted_perp = refractive_indices_ratio * (i + cos_incidence_angle * n);
  vec3 refracted_parallel = - std::sqrt(1.0f - glm::length2(refracted_perp)) * n;
  return unit(refracted_perp + refracted_parallel);
}