#include <random>

#include "math.h"

double degrees_to_radians(double degrees)
{
    return degrees * pi / 180.0;
}

double random_double()
{
    static std::uniform_real_distribution<double> distribution(0.0, 1.0);
    static std::mt19937_64 generator;
    return distribution(generator);
}

double random_double(double min, double max)
{
    static std::uniform_real_distribution<double> distribution(min, max);
    static std::mt19937_64 generator;
    return distribution(generator);
}

double clamp(double value, double min, double max)
{
  if (value < min) return min;
  if (value > max) return max;
  return value;
}