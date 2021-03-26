#pragma once

#include <iostream>

// Using 3d vectors; the interface for points and colors should be such that
// migrating to 4d vectors at any time works seamlessly

class vec
{
  private:
    double v0, v1, v2;
    
  public:
    vec();
    vec(double w0, double w1, double w2);
    vec& operator=(const vec& w);

    double x()  const;
    double y()  const;
    double z()  const;

    double& x();
    double& y();
    double& z();

    vec& operator+=(const vec &w);
    vec& operator*=(const double t);
    vec& operator/=(const double t);

    vec operator-() const; // negate vector

    virtual double norm() const;
    virtual double norm_squared() const;

    static vec random();
    static vec random(double min, double max);
    static vec random_unit();
    static vec random_in_unit_sphere();

    bool near_zero() const;
};

class normed_vec
{
  private:
    double v0, v1, v2;
    normed_vec(double w0, double w1, double w2);

  public:
    normed_vec() = delete; // no meaningful default value
    explicit normed_vec(const vec& v); // construct a normed vector from an ordinary one by normalizing it

    // coordinates can only be returned by value, to preserve the invariant
    double x() const;
    double y() const;
    double z() const;

    double norm() const;
    double norm_squared() const;

    normed_vec operator-() const;

    vec to_vec() const;
};

class point : public vec
{
  public:
    using vec::vec; // same constructors as vec
    point(vec v);
};

class color : public vec
// TODO should enforce taking values in the positive unit cube and throw exceptions if the
// client tries to go out of bounds
{
  public:
    using vec::vec; // same constructors as vec
    color(vec v);

    double r() const;
    double g() const;
    double b() const;

    double& r();
    double& g();
    double& b();

    color operator-() const = delete;
    double norm() = delete;
    double norm_squared() = delete;
  
  private:
    double x() const;
    double y() const;
    double z() const;

    double& x();
    double& y();
    double& z();
};

// vec utility functions
bool operator==(const vec& v, const vec& w);
bool operator!=(const vec& v, const vec& w);

vec operator+(const vec &v, const vec &w);
vec operator-(const vec &v, const vec &w);
vec operator*(const vec &v, const vec &w);
vec operator*(double t, const vec &w);
vec operator*(const vec &v, double t);
vec operator/(const vec &v, double t);

std::ostream& operator<<(std::ostream &out, const vec &v);

// dot product of vectors
double dot(const vec &v, const vec &w);
double dot(const normed_vec &v, const normed_vec &w);
double dot(const vec &v, const normed_vec &w);
double dot(const normed_vec &v, const vec &w);
double dot(const color &v, const vec   &w) = delete;
double dot(const vec   &v, const color &w) = delete;
double dot(const color &v, const color &w) = delete;

// cross product of vectors
vec cross(const vec &v, const vec &w);
vec cross(const normed_vec &v, const normed_vec &w);
vec cross(const normed_vec &v, const vec &w);
vec cross(const vec &v, const normed_vec &w);
vec cross(const vec   &v, const color &w) = delete;
vec cross(const color &v, const vec   &w) = delete;
vec cross(const color &v, const color &w) = delete;

// return unit vector corresponding to v
normed_vec unit(const vec& v);
vec unit(color v) = delete;

normed_vec reflect(const normed_vec& incident, const normed_vec& normal);
normed_vec refract(const normed_vec& incident, const normed_vec& normal, double refractive_indices_ratio);

// color utility functions
void write_color(std::ostream &out, color pixel_color); // write down a single pixel color in PPM format
void write_color(std::ostream &out, color pixel_color, int samples_per_pixel); // write down a single pixel color in PPM format