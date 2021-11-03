#include "math.h"
#include "extern/glm/glm/gtx/norm.hpp"

#include <thread>
#include <time.h>

// if not defined, random_float_pair() generates unstratified pairs
//#define PMJ02_RANDOM_PAIRS 1
#ifdef PMJ02_RANDOM_PAIRS
#include "rng.h"
#include <unordered_map>
#endif

// macros determining which prng is used
// always comment all but one

//#define XORSHIFT_RANDOM 1
#ifdef XORSHIFT_RANDOM
#include <bitset>
#endif

#define XOROSHIRO128PLUS_RANDOM 1
#ifdef XOROSHIRO128PLUS_RANDOM
#include <bitset>
#endif

//#define STD_RNG 1
#ifdef STD_RNG
#include <random>
#endif

static inline uint64_t xoroshiro_rotl(const uint64_t x, int k)
{
  return (x << k) | (x >> (64 - k));
}

#ifdef STD_RNG
uint32_t random_uint32_t()
{
  static thread_local std::mt19937_64 generator(std::clock()
    + std::hash<std::thread::id>()(std::this_thread::get_id()));
  std::uniform_int_distribution<uint32_t> distribution(0,std::numeric_limits<uint32_t>::max());
  return distribution(generator);
}
#endif

uint32_t random_uint32_t(uint16_t x, uint16_t y, uint16_t z)
{
  #ifdef XORSHIFT_RANDOM
  // xorshift
  // seed depending on x, y, and z that is nontrivial even if they're all 0
  static thread_local uint32_t hash{
    ((uint32_t(x) ^ (uint32_t(z) << 8)) | ((uint32_t(y) ^ (uint32_t(z) >> 8)) << 16)) | 0xE6D5258A
  };
  uint32_t n{hash};
  n ^= n >> 13;
  n ^= n << 17;
  n ^= n >> 5;
  hash = n;

  return n;
  #elif defined(XOROSHIRO128PLUS_RANDOM)
  //xoroshiro128+

  // seeds depending on x, y, and z that are nontrivial even if they're all 0
  static thread_local std::array<uint64_t,2> s{
     uint64_t(x) |
     (uint64_t(y) << 16) |
     (uint64_t(z) << 32) |
     (uint64_t(x ^ y) << 48) |
     uint64_t(0xBD6CB273039BC9C0),

     uint64_t(y^z) |
     (uint64_t(x) << 16) |
     (uint64_t(y) << 32) |
     (uint64_t(z) << 48) |
     uint64_t(0xC7B38377DB5B4910)
  };

  const uint64_t s0{s[0]};
  uint64_t s1{s[1]};
  const uint32_t res{uint32_t((s0 + s1) >> 32)};

  // update seed
  s1 ^= s0;
  s[0] = xoroshiro_rotl(s0, 24) ^ s1 ^ (s1 << 16); // a, b
  s[1] = xoroshiro_rotl(s1, 37); // c

  return res;
  #elif defined(STD_RNG)
  return random_uint32_t();
  #endif
}

uint32_t random_uint32_t(uint16_t x, uint16_t y, uint16_t z, uint32_t range)
{
  // unbiased fast ranged rng, due to D. Lemire and M.E. O'Neill
  // (see https://www.pcg-random.org/posts/bounded-rands.html)
  uint32_t n{random_uint32_t(x,y,z)};
  uint64_t m{uint64_t(n) * uint64_t(range)};
  uint32_t l{uint32_t(m)};
  if (l < range)
  {
    uint32_t t{-range};
    if (t >= range)
    {
        t -= range;
        if (t >= range)
            t %= range;
    }
    while (l < t)
    {
        n = random_uint32_t(x,y,z);
        m = uint64_t(n) * uint64_t(range);
        l = uint32_t(m);
    }
  }
  return m >> 32;
}

#ifdef STD_RNG
float random_float()
{
  static thread_local std::mt19937_64 generator(std::clock()
    + std::hash<std::thread::id>()(std::this_thread::get_id()));
  static std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
  return distribution(generator);
}
#endif

float random_float(uint16_t x, uint16_t y, uint16_t z)
{
  #if defined(XORSHIFT_RANDOM) || defined(XOROSHIRO128PLUS_RANDOM)
  uint32_t n{random_uint32_t(x,y,z)};
  // convert the uint32 into a float:
  // mask out the 23 bits of the mantissa, and fix the first 9 bits to make it positive and
  // concentrated in [1,2) (set the exponent to 127)
  std::bitset<32> float_bits{(n & 0x007FFFFF) | 0x3F800000};
  float res;
  std::memcpy(&res,&float_bits,32);

  // shift the result to make it concentrated in [0,1)
  return res - 1.0f;
  #elif defined(STD_RNG)
  return random_float();
  #endif
}

std::array<float,2> random_float_pair(uint16_t x, uint16_t y, uint16_t z)
{
  #ifdef PMJ02_RANDOM_PAIRS
  static thread_local std::unordered_map<uint64_t,size_t> indices;
  static thread_local const uint16_t
    shuffle_filter{uint16_t(random_uint32_t(x,y,z,SIZE_RNG_SAMPLES-1) >> 16)};
  // scramble_filter, to scramble the (mantissa bits of) the sample
  // see Fredel--Keller, Fast Generation of Randomized Low-Discrepancy Point Sets
  // and Kollig--Keller, Efficient Multidimensional Sampling
  static thread_local std::unordered_map<uint64_t,uint32_t> scramble_filters;
  uint64_t hash{uint64_t(x) | (uint64_t(y) << 16) | (uint64_t(z) << 32)};

  if (indices.find(hash) == indices.end())
  {
    indices[hash] = 0;
    scramble_filters[hash] = random_uint32_t(x,y,z) >> 9;
  }
  else
    indices[hash] += 1;

  uint16_t sample{uint16_t(hash % N_RNG_SAMPLES)};

  uint16_t offset{uint16_t(indices[hash] / SIZE_RNG_SAMPLES)};

  if (offset > 0)
  {
    sample += offset;
    sample %= N_RNG_SAMPLES;
  }

  float res0{(*samples_2d[sample])[(indices[hash] % SIZE_RNG_SAMPLES) ^ shuffle_filter][0]};
  float res1{(*samples_2d[sample])[(indices[hash] % SIZE_RNG_SAMPLES) ^ shuffle_filter][1]};

  uint32_t temp0{reinterpret_cast<uint32_t&>(res0) ^ scramble_filters[hash]};
  uint32_t temp1{reinterpret_cast<uint32_t&>(res1) ^ scramble_filters[hash]};

  std::memcpy(&res0,&temp0,32);
  std::memcpy(&res1,&temp1,32);

  return std::array<float,2>{res0,res1};
  #else
  return std::array<float,2>{random_float(x,y,z), random_float(x,y,z)};
  #endif
}

float random_float(float min, float max, uint16_t x, uint16_t y, uint16_t z)
{
  #if defined(XORSHIFT_RANDOM) || defined(XOROSHIRO128PLUS_RANDOM)
  return min + (max - min) * random_float(x,y,z);
  #elif defined(STD_RNG)
  static thread_local std::mt19937_64 generator(std::clock()
    + std::hash<std::thread::id>()(std::this_thread::get_id()));
  std::uniform_real_distribution<float> distribution(min, max);
  return distribution(generator);
  #endif
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