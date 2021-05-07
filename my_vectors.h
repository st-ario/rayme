#pragma once

#include <iostream>

class vec3
{
  private:
    float v0, v1, v2;
    
  public:
    vec3();
    vec3(float w0, float w1, float w2);
    vec3& operator=(const vec3& w);

    float x() const;
    float y() const;
    float z() const;

    float& x();
    float& y();
    float& z();

    vec3& operator+=(const vec3& w);
    vec3& operator*=(const float t);
    vec3& operator/=(const float t);

    vec3 operator-() const;

    virtual float norm() const;
    virtual float norm_squared() const;

    static vec3 random();
    static vec3 random(float min, float max);
    static vec3 random_unit();
    static vec3 random_in_unit_sphere();

    bool near_zero() const;
};

class normed_vec3
{
  private:
    float v0, v1, v2;
    normed_vec3(float w0, float w1, float w2);

  public:
    normed_vec3() = delete; // no meaningful default value
    explicit normed_vec3(const vec3& v); // construct a normed vector from an ordinary one by normalizing it

    // coordinates can only be returned by value, to preserve the invariant
    float x() const;
    float y() const;
    float z() const;

    float norm() const;
    float norm_squared() const;

    normed_vec3 operator-() const;

    vec3 to_vec3() const;
};

class point : public vec3
{
  public:
    using vec3::vec3; // same constructors as vec3
    point(vec3 v);
};

class color
// TODO should enforce taking values in the positive unit cube and throw exceptions if the
// client tries to go out of bounds
{
  public:
    color();
    color(float r, float g, float b);

    float r() const;
    float g() const;
    float b() const;

    float& r();
    float& g();
    float& b();

    color& operator+=(const color& c);
  
  private:
    float red;
    float green;
    float blue;
};

// vec3 utility functions
bool operator==(const vec3& v, const vec3& w);
bool operator!=(const vec3& v, const vec3& w);

vec3 operator+(const vec3 &v, const vec3 &w);
vec3 operator-(const vec3 &v, const vec3 &w);
vec3 operator*(const vec3 &v, const vec3 &w);
vec3 operator*(float t, const vec3 &w);
vec3 operator*(const vec3 &v, float t);
vec3 operator/(const vec3 &v, float t);

std::ostream& operator<<(std::ostream &out, const vec3 &v);

// hashing for vectors
namespace std
{
  template<>
  struct hash<vec3>
  {
    size_t operator()(const vec3& v) const
    {
      // multiply the standard hashes by some big primes and XOR them
      // following https://stackoverflow.com/a/5929567
      return   (hash<float>()(v.x()) * 73856093)
             ^ (hash<float>()(v.y()) * 19349669)
             ^ (hash<float>()(v.z()) * 83492791);
    }
  };
} // namespace std

// dot product of vectors
float dot(const vec3 &v, const vec3 &w);
float dot(const normed_vec3 &v, const normed_vec3 &w);
float dot(const vec3 &v, const normed_vec3 &w);
float dot(const normed_vec3 &v, const vec3 &w);

// cross product of vectors
vec3 cross(const vec3 &v, const vec3 &w);
vec3 cross(const normed_vec3 &v, const normed_vec3 &w);
vec3 cross(const normed_vec3 &v, const vec3 &w);
vec3 cross(const vec3 &v, const normed_vec3 &w);

// return unit vector corresponding to v
normed_vec3 unit(const vec3& v);

normed_vec3 reflect(const normed_vec3& incident, const normed_vec3& normal);
normed_vec3 refract(const normed_vec3& incident, const normed_vec3& normal, float refractive_indices_ratio);

// color utility functions
color operator+(const color &v, const color &w);
color operator*(const color &v, const color &w);
color operator*(const color &v, float t);
color operator*(float t, const color &v);
color operator/(const color &v, float t);

void gamma2_correct(color& c);
void gamma_correct(color& c, float factor);