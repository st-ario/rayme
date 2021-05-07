#include "my_vectors.h"
#include "math.h"

#include <cmath>
#include <string>

// vec member functions
vec3::vec3() : v0{0}, v1{0}, v2{0} {}
vec3::vec3(float x, float y, float z) : v0{x}, v1{y}, v2{z} {}

vec3& vec3::operator=(const vec3& w)
{
  v0 = w.x();
  v1 = w.y();
  v2 = w.z();
  return *this;
}

float vec3::x()  const { return v0; }
float vec3::y()  const { return v1; }
float vec3::z()  const { return v2; }

float& vec3::x()  { return v0; }
float& vec3::y()  { return v1; }
float& vec3::z()  { return v2; }

vec3& vec3::operator+=(const vec3 &w)
{
  v0 += w.x();
  v1 += w.y();
  v2 += w.z();
  return *this;
}

vec3& vec3::operator*=(const float t)
{
  v0 *= t;
  v1 *= t;
  v2 *= t;
  return *this;
}

vec3&   vec3::operator/=(const float t) { return *this *= 1/t; }
vec3    vec3::operator-()    const { return point(-x(),-y(),-z()); }
float vec3::norm()         const { return std::sqrt(norm_squared()); }
float vec3::norm_squared() const { return x()*x() + y()*y() + z()*z(); }

vec3 vec3::random() { return vec3(random_float(), random_float(), random_float()); }

vec3 vec3::random(float min, float max) { return vec3(random_float(min,max), random_float(min,max), random_float(min,max)); }

vec3 vec3::random_unit() // computed normalizing standard Gaussians for each coordinate to get the uniform distribution on the surface
{
  float rx = standard_normal_random_float();
  float ry = standard_normal_random_float();
  float rz = standard_normal_random_float();
  float norm = std::sqrt(rx*rx + ry*ry + rz*rz);
  if (norm == 0)
    //return vec3(0,0,0);
    return vec3::random_unit();
  return (vec3(rx,ry,rz) / norm);
}

vec3 vec3::random_in_unit_sphere()
{
  float random_radius = random_float();
  return random_radius * vec3::random_unit();
}

bool vec3::near_zero() const // return true if the vector is close to being the zero vector
{
  const float epsilon = 1e-8;
  return (fabs(v0) < epsilon) && (fabs(v1) < epsilon) && (fabs(v2) < epsilon);
}

// normed_vec3 member functions
normed_vec3::normed_vec3(const vec3& v) // construct a normed vector from an ordinary one by normalizing it
{
  float norm = v.norm();
  // if norm == 0 throw
  v0 = v.x() / norm;
  v1 = v.y() / norm;
  v2 = v.z() / norm;
}
normed_vec3::normed_vec3(float w0, float w1, float w2) : v0{w0}, v1{w1}, v2{w2} {}

float normed_vec3::norm()         const { return 1.0; }
float normed_vec3::norm_squared() const { return 1.0; };

vec3 normed_vec3::to_vec3() const { return vec3(v0,v1,v2); }

float normed_vec3::x() const { return v0; }
float normed_vec3::y() const { return v1; }
float normed_vec3::z() const { return v2; }

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
point::point(vec3 v) : vec3{v.x(), v.y(), v.z()} {}

// vec3 utility functions
bool operator==(const vec3& v, const vec3& w) { return ((v.x() == w.x()) && (v.y() == w.y()) && (v.z() == w.z())); }
bool operator!=(const vec3& v, const vec3& w) { return !(v == w); }

vec3 operator+(const vec3 &v, const vec3 &w)
{
  float x = v.x() + w.x();
  float y = v.y() + w.y();
  float z = v.z() + w.z();
  return vec3(x,y,z);
}

vec3 operator-(const vec3 &v, const vec3 &w)
{
  float x = v.x() - w.x();
  float y = v.y() - w.y();
  float z = v.z() - w.z();
  return vec3(x,y,z);
}

vec3 operator*(const vec3 &v, const vec3 &w)
{
  float x = v.x() * w.x();
  float y = v.y() * w.y();
  float z = v.z() * w.z();
  return vec3(x,y,z);
}

vec3 operator*(float t, const vec3 &w)
{
  float x = t * w.x();
  float y = t * w.y();
  float z = t * w.z();
  return vec3(x,y,z);
}

vec3 operator*(const vec3 &v, float t)
{
  float x = v.x() * t;
  float y = v.y() * t;
  float z = v.z() * t;
  return vec3(x,y,z);
}

vec3 operator/(const vec3 &v, float t)
{
  float x = v.x() / t;
  float y = v.y() / t;
  float z = v.z() / t;
  return vec3(x,y,z);
}

std::ostream& operator<<(std::ostream &out, const vec3 &v)
{
  return out << v.x() << ' ' << v.y() << ' ' << v.z();
}

// dots: TODO find a more compact way to write this
float dot(const vec3 &v, const vec3 &w) // dot product of vectors
{
  return v.x() * w.x()
       + v.y() * w.y()
       + v.z() * w.z();
}

float dot(const normed_vec3 &v, const normed_vec3 &w)
{
  return v.x() * w.x()
       + v.y() * w.y()
       + v.z() * w.z();
}

float dot(const vec3 &v, const normed_vec3 &w)
{
  return v.x() * w.x()
       + v.y() * w.y()
       + v.z() * w.z();
}

float dot(const normed_vec3 &v, const vec3 &w)
{
  return v.x() * w.x()
       + v.y() * w.y()
       + v.z() * w.z();
}

// crosses: TODO find a more compact way to do it
vec3 cross(const vec3 &v, const vec3 &w) // cross product of vectors
{
  float x = v.y() * w.z() - v.z() * w.y();
  float y = v.z() * w.x() - v.x() * w.z();
  float z = v.x() * w.y() - v.y() * w.x();
  return vec3(x,y,z);
}

vec3 cross(const normed_vec3 &v, const normed_vec3 &w)
{
  float x = v.y() * w.z() - v.z() * w.y();
  float y = v.z() * w.x() - v.x() * w.z();
  float z = v.x() * w.y() - v.y() * w.x();
  return vec3(x,y,z);
}

vec3 cross(const normed_vec3 &v, const vec3 &w)
{
  float x = v.y() * w.z() - v.z() * w.y();
  float y = v.z() * w.x() - v.x() * w.z();
  float z = v.x() * w.y() - v.y() * w.x();
  return vec3(x,y,z);
}

vec3 cross(const vec3 &v, const normed_vec3 &w)
{
  float x = v.y() * w.z() - v.z() * w.y();
  float y = v.z() * w.x() - v.x() * w.z();
  float z = v.x() * w.y() - v.y() * w.x();
  return vec3(x,y,z);
}

// return unit vector corresponding to v
inline normed_vec3 unit(const vec3& v) { return normed_vec3(v); }

// normed_vec3 utility functions
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

  float cos_incidence_angle = dot(-incident, normal);
  vec3 refracted_perp = refractive_indices_ratio * (i + cos_incidence_angle * n);
  vec3 refracted_parallel = - std::sqrt(1.0 - refracted_perp.norm_squared()) * n;
  return unit(refracted_perp + refracted_parallel);
}

normed_vec3 normed_vec3::operator-() const { return normed_vec3(-x(),-y(),-z()); }

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