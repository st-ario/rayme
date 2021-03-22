#pragma once

#include <limits>

// Constants

const double infinity = std::numeric_limits<double>::infinity();
const double pi = 3.1415926535897932385;

// Utility Functions

double degrees_to_radians(double degrees);

double random_double();
double random_double(double min, double max);

double clamp(double value, double min, double max);