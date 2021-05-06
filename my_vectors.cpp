#include "my_vectors.h"
#include "math.h"

#include <cmath>
#include <string>

// vec member functions
vec::vec() : v0{0}, v1{0}, v2{0} {}
vec::vec(float x, float y, float z) : v0{x}, v1{y}, v2{z} {}

vec& vec::operator=(const vec& w)
{
  v0 = w.x();
  v1 = w.y();
  v2 = w.z();
  return *this;
}

float vec::x()  const { return v0; }
float vec::y()  const { return v1; }
float vec::z()  const { return v2; }

float& vec::x()  { return v0; }
float& vec::y()  { return v1; }
float& vec::z()  { return v2; }

vec& vec::operator+=(const vec &w)
{
  v0 += w.x();
  v1 += w.y();
  v2 += w.z();
  return *this;
}

vec& vec::operator*=(const float t)
{
  v0 *= t;
  v1 *= t;
  v2 *= t;
  return *this;
}

vec&   vec::operator/=(const float t) { return *this *= 1/t; }
vec    vec::operator-()    const { return point(-x(),-y(),-z()); }
float vec::norm()         const { return std::sqrt(norm_squared()); }
float vec::norm_squared() const { return x()*x() + y()*y() + z()*z(); }

vec vec::random() { return vec(random_float(), random_float(), random_float()); }

vec vec::random(float min, float max) { return vec(random_float(min,max), random_float(min,max), random_float(min,max)); }

vec vec::random_unit() // computed normalizing standard Gaussians for each coordinate to get the uniform distribution on the surface
{
  float rx = standard_normal_random_float();
  float ry = standard_normal_random_float();
  float rz = standard_normal_random_float();
  float norm = std::sqrt(rx*rx + ry*ry + rz*rz);
  if (norm == 0)
    //return vec(0,0,0);
    return vec::random_unit();
  return (vec(rx,ry,rz) / norm);
}

vec vec::random_in_unit_sphere()
{
  float random_radius = random_float();
  return random_radius * vec::random_unit();
}

bool vec::near_zero() const // return true if the vector is close to being the zero vector
{
  const float epsilon = 1e-8;
  return (fabs(v0) < epsilon) && (fabs(v1) < epsilon) && (fabs(v2) < epsilon);
}

// normed_vec member functions
normed_vec::normed_vec(const vec& v) // construct a normed vector from an ordinary one by normalizing it
{
  float norm = v.norm();
  // if norm == 0 throw
  v0 = v.x() / norm;
  v1 = v.y() / norm;
  v2 = v.z() / norm;
}
normed_vec::normed_vec(float w0, float w1, float w2) : v0{w0}, v1{w1}, v2{w2} {}

float normed_vec::norm()         const { return 1.0; }
float normed_vec::norm_squared() const { return 1.0; };

vec normed_vec::to_vec() const { return vec(v0,v1,v2); }

float normed_vec::x() const { return v0; }
float normed_vec::y() const { return v1; }
float normed_vec::z() const { return v2; }

// color member functions
color::color() : red{0}, green{0}, blue{0} {}
color::color(float r, float g, float b) : red{r}, green{g}, blue{b} {}

color& color::operator+=(const color& c)
{
  red += c.r();
  green += c.g();
  blue += c.b();

  return *this;
}

float color::r() const { return red; }
float color::g() const { return green; }
float color::b() const { return blue; }

float& color::r() { return red; }
float& color::g() { return green; }
float& color::b() { return blue; }

// point member functions
point::point(vec v) : vec{v.x(), v.y(), v.z()} {}

// vec utility functions
bool operator==(const vec& v, const vec& w) { return ((v.x() == w.x()) && (v.y() == w.y()) && (v.z() == w.z())); }
bool operator!=(const vec& v, const vec& w) { return !(v == w); }

vec operator+(const vec &v, const vec &w)
{
  float x = v.x() + w.x();
  float y = v.y() + w.y();
  float z = v.z() + w.z();
  return vec(x,y,z);
}

vec operator-(const vec &v, const vec &w)
{
  float x = v.x() - w.x();
  float y = v.y() - w.y();
  float z = v.z() - w.z();
  return vec(x,y,z);
}

vec operator*(const vec &v, const vec &w)
{
  float x = v.x() * w.x();
  float y = v.y() * w.y();
  float z = v.z() * w.z();
  return vec(x,y,z);
}

vec operator*(float t, const vec &w)
{
  float x = t * w.x();
  float y = t * w.y();
  float z = t * w.z();
  return vec(x,y,z);
}

vec operator*(const vec &v, float t)
{
  float x = v.x() * t;
  float y = v.y() * t;
  float z = v.z() * t;
  return vec(x,y,z);
}

vec operator/(const vec &v, float t)
{
  float x = v.x() / t;
  float y = v.y() / t;
  float z = v.z() / t;
  return vec(x,y,z);
}

std::ostream& operator<<(std::ostream &out, const vec &v)
{
  return out << v.x() << ' ' << v.y() << ' ' << v.z();
}

// dots: TODO find a more compact way to write this
float dot(const vec &v, const vec &w) // dot product of vectors
{
  return v.x() * w.x()
       + v.y() * w.y()
       + v.z() * w.z();
}

float dot(const normed_vec &v, const normed_vec &w)
{
  return v.x() * w.x()
       + v.y() * w.y()
       + v.z() * w.z();
}

float dot(const vec &v, const normed_vec &w)
{
  return v.x() * w.x()
       + v.y() * w.y()
       + v.z() * w.z();
}

float dot(const normed_vec &v, const vec &w)
{
  return v.x() * w.x()
       + v.y() * w.y()
       + v.z() * w.z();
}

// crosses: TODO find a more compact way to do it
vec cross(const vec &v, const vec &w) // cross product of vectors
{
  float x = v.y() * w.z() - v.z() * w.y();
  float y = v.z() * w.x() - v.x() * w.z();
  float z = v.x() * w.y() - v.y() * w.x();
  return vec(x,y,z);
}

vec cross(const normed_vec &v, const normed_vec &w)
{
  float x = v.y() * w.z() - v.z() * w.y();
  float y = v.z() * w.x() - v.x() * w.z();
  float z = v.x() * w.y() - v.y() * w.x();
  return vec(x,y,z);
}

vec cross(const normed_vec &v, const vec &w)
{
  float x = v.y() * w.z() - v.z() * w.y();
  float y = v.z() * w.x() - v.x() * w.z();
  float z = v.x() * w.y() - v.y() * w.x();
  return vec(x,y,z);
}

vec cross(const vec &v, const normed_vec &w)
{
  float x = v.y() * w.z() - v.z() * w.y();
  float y = v.z() * w.x() - v.x() * w.z();
  float z = v.x() * w.y() - v.y() * w.x();
  return vec(x,y,z);
}

// return unit vector corresponding to v
inline normed_vec unit(const vec& v) { return normed_vec(v); }

// normed_vec utility functions
normed_vec reflect(const normed_vec& incident, const normed_vec& normal)
{
  vec i = incident.to_vec();
  vec n = normal.to_vec();

  return unit(i - 2 * dot(i,n) * n);
}

normed_vec refract(const normed_vec& incident, const normed_vec& normal, float refractive_indices_ratio)
{
  vec i = incident.to_vec();
  vec n = normal.to_vec();

  float cos_incidence_angle = dot(-incident, normal);
  vec refracted_perp = refractive_indices_ratio * (i + cos_incidence_angle * n);
  vec refracted_parallel = - std::sqrt(1.0 - refracted_perp.norm_squared()) * n;
  return unit(refracted_perp + refracted_parallel);
}

normed_vec normed_vec::operator-() const { return normed_vec(-x(),-y(),-z()); }

// color utility functions

color operator+(const color &v, const color &w)
{
  float r = v.r() + w.r();
  float g = v.g() + w.g();
  float b = v.b() + w.b();
  return color(r,g,b);
}

color operator*(const color &v, const color &w)
{
  float r = v.r() * w.r();
  float g = v.g() * w.g();
  float b = v.b() * w.b();
  return color(r,g,b);
}

color operator*(const color &v, float t)
{
  float r = v.r() * t;
  float g = v.g() * t;
  float b = v.b() * t;
  return color(r,g,b);
}

color operator*(float t, const color &v)
{
  float r = t * v.r();
  float g = t * v.g();
  float b = t * v.b();
  return color(r,g,b);
}

color operator/(const color &v, float t)
{
  float r = v.r() / t;
  float g = v.g() / t;
  float b = v.b() / t;
  return color(r,g,b);
}

void gamma2_correct(color& c)
{
  c.r() = std::sqrt(c.r());
  c.g() = std::sqrt(c.g());
  c.b() = std::sqrt(c.b());
}

void gamma_correct(color& c, float gamma)
{
  c.r() = std::pow(c.r(), 1.0/gamma);
  c.g() = std::pow(c.g(), 1.0/gamma);
  c.b() = std::pow(c.b(), 1.0/gamma);
}