#include <random>

#include "math.h"

float degrees_to_radians(float degrees)
{
    return degrees * pi / 180.0;
}

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

float clamp(float value, float min, float max)
{
  if (value < min) return min;
  if (value > max) return max;
  return value;
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