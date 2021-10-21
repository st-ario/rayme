#include "math.h"
#include "extern/glm/glm/gtx/norm.hpp"

#include <random>
#include <thread>
#include <time.h>

#define STATIC_RANDOM 1
#ifdef STATIC_RANDOM
#include "rng.h"
#include <unordered_map>
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
  static thread_local std::mt19937_64 generator(std::clock()
    + std::hash<std::thread::id>()(std::this_thread::get_id()));
  static std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
  return distribution(generator);
}

std::array<float,2> random_float_pair(uint16_t x, uint16_t y, uint16_t z)
{
  #ifdef STATIC_RANDOM
  static thread_local std::unordered_map<uint64_t,size_t> indices;
  static thread_local size_t shuffle_filter{random_size_t(0,SIZE_RNG_SAMPLES-1)};
  uint64_t hash{uint64_t(x) | (uint64_t(y) << 16) | (uint64_t(z) << 32)};

  if (indices.find(hash) == indices.end())
    indices[hash] = 0;
  else
    indices[hash] += 1;

  uint16_t sample{uint16_t(hash % N_RNG_SAMPLES)};
  /* the following is too slow;
  // for now, if the index exceeds SIZE_RNG_SAMPLES increments, it cycles back
  // it should be fine like this
  // TODO test for how many times a wraparound actually happens
  uint16_t offset{uint16_t(indices[hash] / SIZE_RNG_SAMPLES)};

  if (offset > 0)
  {
    sample += offset;
    sample %= N_RNG_SAMPLES;
  }
  */

  return (*samples_2d[sample])[(indices[hash] % SIZE_RNG_SAMPLES) ^ uint16_t(shuffle_filter)];
  #else
  return std::array<float,2>{random_float(x,y,z), random_float(x,y,z)};
  #endif
}

float random_float(float min, float max, uint16_t x, uint16_t y, uint16_t z)
{
  static thread_local std::mt19937_64 generator(std::clock()
    + std::hash<std::thread::id>()(std::this_thread::get_id()));
  std::uniform_real_distribution<float> distribution(min, max);
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