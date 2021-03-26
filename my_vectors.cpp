#include "my_vectors.h"
#include "math.h"

#include <cmath>

// vec member functions
vec::vec() : v0{0}, v1{0}, v2{0} {}
vec::vec(double x, double y, double z) : v0{x}, v1{y}, v2{z} {}
vec::vec(const vec& w) : v0{w.x()}, v1{w.y()}, v2{w.z()} {}

vec& vec::operator=(const vec& w)
{
  v0 = w.x();
  v1 = w.y();
  v2 = w.z();
  return *this;
}

double vec::x()  const { return v0; }
double vec::y()  const { return v1; }
double vec::z()  const { return v2; }

double& vec::x()  { return v0; }
double& vec::y()  { return v1; }
double& vec::z()  { return v2; }

vec& vec::operator+=(const vec &w)
{
  v0 += w.x();
  v1 += w.y();
  v2 += w.z();
  return *this;
}

vec& vec::operator*=(const double t)
{
  v0 *= t;
  v1 *= t;
  v2 *= t;
  return *this;
}

vec&   vec::operator/=(const double t)       { return *this *= 1/t; }
vec    vec::operator-()                const { return point(-x(),-y(),-z()); }
double vec::norm()                     const { return std::sqrt(norm_squared()); }
double vec::norm_squared()             const { return x()*x() + y()*y() + z()*z(); }

vec vec::random()
{
  return vec( random_double()
            , random_double()
            , random_double()
            );
}

vec vec::random(double min, double max)
{
  return vec( random_double(min,max)
            , random_double(min,max)
            , random_double(min,max)
            );
}

vec vec::random_unit() // computed normalizing standard Gaussians for each coordinate to get the uniform distribution on the surface
{
  double rx = standard_normal_random_double();
  double ry = standard_normal_random_double();
  double rz = standard_normal_random_double();
  double norm = std::sqrt(rx*rx + ry*ry + rz*rz);
  if (norm == 0)
    //return vec(0,0,0);
    return vec::random_unit();
  return (vec(rx,ry,rz) / norm);
}

vec vec::random_in_unit_sphere()
{
  double random_radius = random_double();
  return random_radius * vec::random_unit();
}

bool vec::near_zero() const // return true if the vector is close to being the zero vector
{
  const double epsilon = 1e-8;
  return (fabs(v0) < epsilon) && (fabs(v1) < epsilon) && (fabs(v2) < epsilon);
}

// normed_vec member functions
normed_vec::normed_vec(const vec& v) // construct a normed vector from an ordinary one by normalizing it
{
  double norm = v.norm();
  x() = v.x() / norm;
  y() = v.y() / norm;
  z() = v.z() / norm;
}

normed_vec& normed_vec::operator=(const normed_vec& w)
{
  x() = w.x();
  y() = w.y();
  z() = w.z();
}

double normed_vec::norm()         const { return 1.0; }
double normed_vec::norm_squared() const { return 1.0; };

vec normed_vec::to_vec() const { return vec(x(),y(),z()); }

// color member functions
color::color(vec v) : vec{v.x(), v.y(), v.z()} {}

double color::x() const { return vec::x(); }
double color::y() const { return vec::y(); }
double color::z() const { return vec::z(); }

double& color::x() { return vec::x(); }
double& color::y() { return vec::y(); }
double& color::z() { return vec::z(); }

double color::r() const { return x(); }
double color::g() const { return y(); }
double color::b() const { return z(); }

double& color::r() { return x(); }
double& color::g() { return y(); }
double& color::b() { return z(); }

// point member functions
point::point(vec v) : vec{v.x(), v.y(), v.z()} {}

// vec utility functions
inline bool operator==(const vec& v, const vec& w)
{
  return ((v.x() == w.x()) && (v.y() == w.y()) && (v.z() == w.z()));
}

inline bool operator!=(const vec& v, const vec& w)
{
  return !(v == w);
}

inline vec operator+(const vec &v, const vec &w)
{
  double x = v.x() + w.x();
  double y = v.y() + w.y();
  double z = v.z() + w.z();
  return vec(x,y,z);
}

inline vec operator-(const vec &v, const vec &w)
{
  double x = v.x() - w.x();
  double y = v.y() - w.y();
  double z = v.z() - w.z();
  return vec(x,y,z);
}

inline vec operator*(const vec &v, const vec &w)
{
  double x = v.x() * w.x();
  double y = v.y() * w.y();
  double z = v.z() * w.z();
  return vec(x,y,z);
}

inline vec operator*(double t, const vec &w)
{
  double x = t * w.x();
  double y = t * w.y();
  double z = t * w.z();
  return vec(x,y,z);
}

inline vec operator*(const vec &v, double t)
{
  double x = v.x() * t;
  double y = v.y() * t;
  double z = v.z() * t;
  return vec(x,y,z);
}

inline vec operator/(const vec &v, double t)
{
  double x = v.x() / t;
  double y = v.y() / t;
  double z = v.z() / t;
  return vec(x,y,z);
}

inline std::ostream& operator<<(std::ostream &out, const vec &v)
{
  return out << v.x() << ' ' << v.y() << ' ' << v.z();
}

double dot(const vec &v, const vec &w) // dot product of vectors
{
  return v.x() * w.x()
       + v.y() * w.y()
       + v.z() * w.z();
}

inline vec cross(const vec &v, const vec &w) // cross product of vectors
{
  double x = v.y() * w.z() - v.z() * w.y();
  double y = v.z() * w.x() - v.x() * w.z();
  double z = v.x() * w.y() - v.y() * w.x();
  return vec(x,y,z);
}

inline normed_vec cross(const normed_vec &v, const normed_vec &w) // cross product of vectors
{
  double x = v.y() * w.z() - v.z() * w.y();
  double y = v.z() * w.x() - v.x() * w.z();
  double z = v.x() * w.y() - v.y() * w.x();
  return unit(vec(x,y,z)); // correct for possible rounding errors
}

inline normed_vec unit(const vec& v) // return unit vector corresponding to v
{
  return normed_vec(v);
}

normed_vec reflect(const normed_vec& incident, const normed_vec& normal)
{
  vec i = incident.to_vec();
  vec n = normal.to_vec();

  return unit(i - 2 * dot(i,n) * n);
}

normed_vec refract(const normed_vec& incident, const normed_vec& normal, double refractive_indices_ratio)
{
  vec i = incident.to_vec();
  vec n = normal.to_vec();

  double cos_incidence_angle = dot(-incident,normal);
  vec refracted_perp = refractive_indices_ratio * (i + cos_incidence_angle * n);
  vec refracted_parallel = - std::sqrt(1.0 - refracted_perp.norm_squared()) * n;
  return unit(refracted_perp + refracted_parallel);
}

// color utility functions
void write_color(std::ostream &out, color pixel_color) // write down a single pixel color in PPM format
{
  out << static_cast<int>(255 * pixel_color.r()) << ' '
      << static_cast<int>(255 * pixel_color.g()) << ' '
      << static_cast<int>(255 * pixel_color.b()) << '\n';
}

void write_color(std::ostream &out, color pixel_color, int samples_per_pixel) // write down a single pixel color in PPM format
{
  double r = pixel_color.r();
  double g = pixel_color.g();
  double b = pixel_color.b();

  // Divide the color by the number of samples, then gamma-correct it
  // Gamma correction: raising to 1/gamma
  double scale = 1.0 / samples_per_pixel;
  /*
  r = std::pow(scale * r, 1.0/5.0);
  g = std::pow(scale * g, 1.0/5.0);
  b = std::pow(scale * b, 1.0/5.0);
  */
  r = std::sqrt(scale * r);
  g = std::sqrt(scale * g);
  b = std::sqrt(scale * b);

  out << static_cast<int>(255 * clamp(r, 0, 1.0)) << ' '
      << static_cast<int>(255 * clamp(g, 0, 1.0)) << ' '
      << static_cast<int>(255 * clamp(b, 0, 1.0)) << '\n';
}