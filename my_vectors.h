#pragma once

#include <iostream>

class vec
{
  private:
    float v0, v1, v2;
    
  public:
    vec();
    vec(float w0, float w1, float w2);
    vec& operator=(const vec& w);

    float x() const;
    float y() const;
    float z() const;

    float& x();
    float& y();
    float& z();

    vec& operator+=(const vec& w);
    vec& operator*=(const float t);
    vec& operator/=(const float t);

    vec operator-() const; // negate vector

    virtual float norm() const;
    virtual float norm_squared() const;

    static vec random();
    static vec random(float min, float max);
    static vec random_unit();
    static vec random_in_unit_sphere();

    bool near_zero() const;
};

class normed_vec
{
  private:
    float v0, v1, v2;
    normed_vec(float w0, float w1, float w2);

  public:
    normed_vec() = delete; // no meaningful default value
    explicit normed_vec(const vec& v); // construct a normed vector from an ordinary one by normalizing it

    // coordinates can only be returned by value, to preserve the invariant
    float x() const;
    float y() const;
    float z() const;

    float norm() const;
    float norm_squared() const;

    normed_vec operator-() const;

    vec to_vec() const;
};

class point : public vec
{
  public:
    using vec::vec; // same constructors as vec
    point(vec v);
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

// vec utility functions
bool operator==(const vec& v, const vec& w);
bool operator!=(const vec& v, const vec& w);

vec operator+(const vec &v, const vec &w);
vec operator-(const vec &v, const vec &w);
vec operator*(const vec &v, const vec &w);
vec operator*(float t, const vec &w);
vec operator*(const vec &v, float t);
vec operator/(const vec &v, float t);

std::ostream& operator<<(std::ostream &out, const vec &v);

// hashing for vectors
namespace std
{
  template<>
  struct hash<vec>
  {
    size_t operator()(const vec& v) const
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
float dot(const vec &v, const vec &w);
float dot(const normed_vec &v, const normed_vec &w);
float dot(const vec &v, const normed_vec &w);
float dot(const normed_vec &v, const vec &w);

// cross product of vectors
vec cross(const vec &v, const vec &w);
vec cross(const normed_vec &v, const normed_vec &w);
vec cross(const normed_vec &v, const vec &w);
vec cross(const vec &v, const normed_vec &w);

// return unit vector corresponding to v
normed_vec unit(const vec& v);

normed_vec reflect(const normed_vec& incident, const normed_vec& normal);
normed_vec refract(const normed_vec& incident, const normed_vec& normal, float refractive_indices_ratio);

// color utility functions
color operator+(const color &v, const color &w);
color operator*(const color &v, const color &w);
color operator*(const color &v, float t);
color operator*(float t, const color &v);
color operator/(const color &v, float t);

void gamma2_correct(color& c);
void gamma_correct(color& c, float factor);