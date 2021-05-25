#pragma once

#include <iostream>
#include <cmath>
#include "math.h"

// #################################################################################################
// iterators
// #################################################################################################

template<typename Container>
class my_iterator
{
  public:
    using Number = typename Container::NumberType;

  private:
    Number* current;

  public:
    constexpr my_iterator() noexcept : current{nullptr} {}
    explicit constexpr my_iterator(Number* p) noexcept : current{p} {}

    constexpr Number&      operator*()  const noexcept { return *current; }
    constexpr Number*      operator->() const noexcept { return  current; }
    constexpr my_iterator& operator++()       noexcept { ++current; return *this; }
    constexpr my_iterator  operator++(int)    noexcept { return my_iterator(current++); }
    constexpr my_iterator& operator--()       noexcept { --current; return *this; }
    constexpr my_iterator  operator--(int)    noexcept { return my_iterator(current--); }

    constexpr friend bool operator==(const my_iterator<Container>& lhs,
                                     const my_iterator<Container>& rhs) noexcept
    { return lhs.current == rhs.current; }
    constexpr friend bool operator!=(const my_iterator<Container>& lhs,
                                     const my_iterator<Container>& rhs) noexcept
    { return lhs.current != rhs.current; }

    constexpr const Number* base() const noexcept { return current; }
};

template<typename Container>
class my_const_iterator
{
  public:
    using Number = typename Container::NumberType;

  private:
    Number* current;

  public:
    constexpr my_const_iterator() noexcept : current{nullptr} {}
    explicit constexpr my_const_iterator(Number* p) noexcept : current{p} {}
    constexpr my_const_iterator(const my_iterator<Container>& iter) : current{iter.base()} {}

    constexpr const Number&      operator*()  const noexcept { return *current; }
    constexpr const Number*      operator->() const noexcept { return  current; }
    constexpr my_const_iterator& operator++()    noexcept { ++current; return *this; }
    constexpr my_const_iterator  operator++(int) noexcept { return my_const_iterator(current++); }
    constexpr my_const_iterator& operator--()    noexcept { --current; return *this; }
    constexpr my_const_iterator  operator--(int) noexcept { return my_const_iterator(current--); }

    constexpr friend bool operator==(const my_const_iterator<Container>& lhs,
                                     const my_const_iterator<Container>& rhs) noexcept
    { return lhs.current == rhs.current; }
    constexpr friend bool operator!=(const my_const_iterator<Container>& lhs,
                                     const my_const_iterator<Container>& rhs) noexcept
    { return lhs.current != rhs.current; }

    constexpr const Number* base() const noexcept { return current; }
};

// #################################################################################################
// containers declarations
// #################################################################################################

// parent class for both normed_vec3 and vec3
template<typename Number, unsigned short int Size>
class rigid_container
{
  public:
    using NumberType = Number;
    using ConstIterator = my_const_iterator<rigid_container<Number,Size>>;

  protected:
    Number m_array[Size];
    rigid_container();

  public:
    rigid_container(const rigid_container<Number,Size>& w);
    rigid_container& operator=(const rigid_container<Number,Size>& w);
    ~rigid_container() = default;

    Number operator[](unsigned short int i) const;
    rigid_container& operator-();
    rigid_container<Number,Size> operator-() const;

    constexpr ConstIterator cbegin() const noexcept {return ConstIterator(&m_array[0]); }
    constexpr ConstIterator cend()   const noexcept {return ConstIterator(&m_array[Size]); }
};

template<typename Number, unsigned short int Size>
class my_container : public rigid_container<Number,Size>
{
  public:
    using Iterator = my_iterator<my_container<Number,Size>>;

  protected:
    using rigid_container<Number,Size>::m_array;

  public:
    // same constructors as parent
    using rigid_container<Number,Size>::rigid_container;

    my_container& operator+=(Number t);
    my_container& operator-=(Number t);
    my_container& operator*=(Number t);
    my_container& operator/=(Number t);
    my_container& operator+=(const my_container<Number,Size>& w);
    my_container& operator-=(const my_container<Number,Size>& w);
    my_container& operator*=(const my_container<Number,Size>& w);
    my_container& operator/=(const my_container<Number,Size>& w);
    using rigid_container<Number,Size>::operator[];
    Number& operator[](unsigned short int i);
    constexpr Iterator begin() noexcept { return Iterator(&m_array[0]); }
    constexpr Iterator end()   noexcept { return Iterator(&m_array[Size]); }

    my_container& apply_componentwise(void(*function)(Number&));

    // apply the function componentwise and write the result in place
    my_container& inplace_zip_with(const my_container<Number,Size>& w,
                                   void(*function)(Number&, Number));

    // apply function(Number, Number) componentwise and return a container of results
    my_container<Number,Size> zip_with(const my_container<Number,Size>& w,
                                       Number(*function)(Number, Number)) const;
};

template<typename Number, unsigned short int Size>
bool operator==(const my_container<Number,Size>& v, const my_container<Number,Size>& w);
template<typename Number, unsigned short int Size>
bool operator!=(const my_container<Number,Size>& v, const my_container<Number,Size>& w);
template<typename Number, unsigned short int Size>
my_container<Number,Size> operator+(const my_container<Number,Size>& v,
                                    const my_container<Number,Size>& w);
template<typename Number, unsigned short int Size>
my_container<Number,Size> operator-(const my_container<Number,Size>& v,
                                    const my_container<Number,Size>& w);
template<typename Number, unsigned short int Size>
my_container<Number,Size> operator*(const my_container<Number,Size>& v,
                                    const my_container<Number,Size>& w);
template<typename Number, unsigned short int Size>
my_container<Number,Size> operator*(Number t, const my_container<Number,Size>& w);
template<typename Number, unsigned short int Size>
my_container<Number,Size> operator*(const my_container<Number,Size>&v, Number t);
template<typename Number, unsigned short int Size>
my_container<Number,Size> operator/(const my_container<Number,Size>&v, Number t);

class vec3 : public my_container<float,3>
{
  public:
    // same constructors as parent
    using my_container<float,3>::my_container;
    vec3 (const rigid_container<float,3>& v);
    vec3 (float x, float y, float z);

    using my_container<float,3>::operator[];

    float x() const;
    float y() const;
    float z() const;

    float& x();
    float& y();
    float& z();

    float norm_squared() const;

    friend vec3 permute(const vec3& v, int x, int y, int z);

    static vec3 random();
    static vec3 random(float min, float max);
    static vec3 random_in_unit_sphere();

    bool near_zero() const;
};

class normed_vec3 : public rigid_container<float,3>
{
  private:
    // unsafe constructor, doesn't check the invariant
    normed_vec3(float w0, float w1, float w2);

  public:
    // construct a normed vector from an ordinary one by normalizing it
    explicit normed_vec3(const rigid_container<float,3>& w);

    // coordinates can only be returned by value, to preserve the invariant
    float x() const;
    float y() const;
    float z() const;

    normed_vec3& operator-();
    normed_vec3 operator-() const;

    static normed_vec3 random_unit();

    friend normed_vec3 permute(const normed_vec3& v, int x, int y, int z);

    vec3 to_vec3() const;
};

typedef vec3 point;

// color values are stored as doubles to prevent mixing color and vec3 operations
class color : public my_container<double,3>
{
  private:
    using my_container<double,3>::m_array;

  public:
    using my_container<double,3>::my_container;

    color(double r, double g, double b);
    color(const rigid_container<double,3>& v);

    double r() const;
    double g() const;
    double b() const;

    double& r();
    double& g();
    double& b();
};

// TODO change when they'll be actually used (if needed)
typedef my_container<float, 2> vec2;
typedef my_container<float, 4> vec4;
typedef my_container<float, 4> mat2;
typedef my_container<float, 9> mat3;

// dot product of vectors
template<typename Number, unsigned short int Size>
Number dot(const rigid_container<Number,Size>& v, const rigid_container<Number,Size>& w);

double dot(const color&, const color&) = delete;
template<typename Number, unsigned short int Size>
double dot(const color&, const rigid_container<double,3>&) = delete;
template<typename Number, unsigned short int Size>
double dot(const rigid_container<double,3>&, const color&) = delete;

// cross product of vectors
vec3 cross(const rigid_container<float,3>& v, const rigid_container<float,3>& w);

// return unit vector corresponding to v
inline normed_vec3 unit(const rigid_container<float,3>& v) { return normed_vec3(v); }

int max_dimension(const vec3& v);
float max_component(const vec3& v);

inline vec3 abs(const rigid_container<float,3>& v);

normed_vec3 reflect(const normed_vec3& incident, const normed_vec3& normal);
normed_vec3 refract(const normed_vec3& incident, const normed_vec3& normal,
                    float refractive_indices_ratio);

void gamma2_correct(color& c);
void gamma_correct(color& c, double gamma);

// hashing functions
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

// #################################################################################################
// containers definitions
// #################################################################################################
template<typename Number, unsigned short int Size>
rigid_container<Number,Size>::rigid_container()
{
  for (size_t i = 0; i < Size; ++i)
    m_array[i] = 0.0f;
}

template<typename Number, unsigned short int Size>
inline rigid_container<Number,Size>::rigid_container(const rigid_container<Number,Size>& w)
{
  for (size_t i = 0; i < Size; ++i)
    m_array[i] = w[i];
}

template<typename Number, unsigned short int Size>
inline
rigid_container<Number,Size>& rigid_container<Number,Size>::operator=(
                                                              const rigid_container<Number,Size>& w)
{
  for (size_t i = 0; i < Size; ++i)
    m_array[i] = w[i];
  return *this;
}

template<typename Number, unsigned short int Size>
rigid_container<Number,Size>& rigid_container<Number,Size>::operator-()
{
  for (size_t i = 0; i < Size; ++i)
    m_array[i] = - m_array[i];

  return *this;
}

template<typename Number, unsigned short int Size>
rigid_container<Number,Size> rigid_container<Number,Size>::operator-() const
{
  rigid_container<Number,Size> res;
  for (size_t i = 0; i < Size; ++i)
    res.m_array[i] = - this->m_array[i];

  return res;
}

template<typename Number, unsigned short int Size>
inline my_container<Number,Size>& my_container<Number,Size>::operator+=(Number t)
{
  for (size_t i = 0; i < Size; ++i)
    m_array[i] += t;
  return *this;
}

template<typename Number, unsigned short int Size>
inline my_container<Number,Size>& my_container<Number,Size>::operator-=(Number t)
{
  for (size_t i = 0; i < Size; ++i)
    m_array[i] -= t;
  return *this;
}

template<typename Number, unsigned short int Size>
inline my_container<Number,Size>& my_container<Number,Size>::operator*=(Number t)
{
  for (size_t i = 0; i < Size; ++i)
    m_array[i] *= t;
  return *this;
}

template<typename Number, unsigned short int Size>
inline my_container<Number,Size>& my_container<Number,Size>::operator/=(Number t)
{
  for (size_t i = 0; i < Size; ++i)
    m_array[i] /= t;
  return *this;
}

template<typename Number, unsigned short int Size>
inline my_container<Number,Size>& my_container<Number,Size>::operator+=(
                                                               const my_container<Number,Size>& w)
{
  this->inplace_zip_with(w,[](Number& n, Number m){n+=m;});
  return *this;
}

template<typename Number, unsigned short int Size>
inline my_container<Number,Size>& my_container<Number,Size>::operator-=(
                                                               const my_container<Number,Size>& w)
{
  this->inplace_zip_with(w,[](Number& n, Number m){n-=m;});
  return *this;
}

template<typename Number, unsigned short int Size>
inline my_container<Number,Size>& my_container<Number,Size>::operator*=(
                                                               const my_container<Number,Size>& w)
{
  this->inplace_zip_with(w,[](Number& n, Number m){n*=m;});
  return *this;
}

template<typename Number, unsigned short int Size>
inline
my_container<Number,Size>& my_container<Number,Size>::operator/=(const my_container<Number,Size>& w)
{
  this->inplace_zip_with(w,[](Number& n, Number m){n/=m;});
  return *this;
}

template<typename Number, unsigned short int Size>
inline bool operator==(const my_container<Number,Size>& v, const my_container<Number,Size>& w)
{
  for (size_t i = 0; i < Size; ++i)
  {
    if (v[i] != w[i])
      return false;
  }
  return true;
}

template<typename Number, unsigned short int Size>
inline bool operator!=(const my_container<Number,Size>& v, const my_container<Number,Size>& w)
{ return !(v == w); }

template<typename Number, unsigned short int Size>
Number& my_container<Number,Size>::operator[](unsigned short int i)
{
  //if (i < Size)
    return m_array[i];

  // TODO print error message and throw
}

template<typename Number, unsigned short int Size>
Number rigid_container<Number,Size>::operator[](unsigned short int i) const
{
  //if (i < Size)
    return m_array[i];

  // TODO print error message and throw
}

template<typename Number, unsigned short int Size>
inline
my_container<Number,Size>& my_container<Number,Size>::apply_componentwise(void(*function)(Number&))
{
  for (Number& x : *this)
    function(x);
  return *this;
}

template<typename Number, unsigned short int Size>
inline
my_container<Number,Size>& my_container<Number,Size>::inplace_zip_with(
                                                        const my_container<Number,Size>& w,
                                                        void(*function)(Number&, Number))
{
  for (size_t i = 0; i < Size; ++i)
    function(m_array[i],w[i]);
  return *this;
}

template<typename Number, unsigned short int Size>
inline
my_container<Number,Size> my_container<Number,Size>::zip_with(
                                                       const my_container<Number,Size>& w,
                                                       Number(*function)(Number, Number))
const
{
  my_container<Number,Size> res;
  for (size_t i = 0; i < Size; ++i)
    res[i] = function(m_array[i],w[i]);
  return res;
}

template<typename Number, unsigned short int Size>
inline my_container<Number,Size> operator+(const my_container<Number,Size>& v,
                                           const my_container<Number,Size>& w)
{
  return v.zip_with(w,[](Number n, Number m){return n+m;});
}

template<typename Number, unsigned short int Size>
inline my_container<Number,Size> operator-(const my_container<Number,Size>& v,
                                           const my_container<Number,Size>& w)
{
  return v.zip_with(w,[](Number n, Number m){return n-m;});
}

template<typename Number, unsigned short int Size>
inline my_container<Number,Size> operator*(const my_container<Number,Size>& v,
                                           const my_container<Number,Size>& w)
{
  return v.zip_with(w,[](Number n, Number m){return n*m;});
}

template<typename Number, unsigned short int Size>
inline my_container<Number,Size> operator*(Number t, const my_container<Number,Size>& w)
{
  my_container<Number,Size> res;
  for (size_t i = 0; i < Size; ++i)
    res[i] = t * w[i];
  return res;
}

template<typename Number, unsigned short int Size>
inline my_container<Number,Size> operator*(const my_container<Number,Size>&v, Number t)
{
  my_container<Number,Size> res;
  for (size_t i = 0; i < Size; ++i)
    res[i] = v[i] * t;
  return res;
}

template<typename Number, unsigned short int Size>
inline my_container<Number,Size> operator/(const my_container<Number,Size>&v, Number t)
{
  my_container<Number,Size> res;
  for (size_t i = 0; i < Size; ++i)
    res[i] = v[i] / t;
  return res;
}


template<typename Number, unsigned short int Size>
inline Number dot(const rigid_container<Number,Size>& v, const rigid_container<Number,Size>& w)
{
  Number n = 0;
  for (size_t i = 0; i < Size; ++i)
    n += v[i] * w[i];

  return n;
}

inline vec3 cross(const rigid_container<float,3>& v, const rigid_container<float,3>& w)
{
  return vec3( v[1] * w[2] - v[2] * w[1]
             , v[2] * w[0] - v[0] * w[2]
             , v[0] * w[1] - v[1] * w[0]);
}

inline vec3::vec3(float x, float y, float z)
{
  m_array[0] = x;
  m_array[1] = y;
  m_array[2] = z;
}

inline float vec3::x()  const { return m_array[0]; }
inline float vec3::y()  const { return m_array[1]; }
inline float vec3::z()  const { return m_array[2]; }

inline float& vec3::x()  { return m_array[0]; }
inline float& vec3::y()  { return m_array[1]; }
inline float& vec3::z()  { return m_array[2]; }

// inline float vec3::norm()         const { return std::sqrt(norm_squared()); }
inline float vec3::norm_squared() const { return x()*x() + y()*y() + z()*z(); }

// return true if the vector is close to being the zero vector
inline bool vec3::near_zero() const
{
  const float epsilon = 1e-8;
  return (std::fabs(x()) < epsilon) && (std::fabs(y()) < epsilon) && (std::fabs(z()) < epsilon);
}

// construct a normed vector from an ordinary one by normalizing it
inline normed_vec3::normed_vec3(const rigid_container<float,3>& w)
{
  float squared_norm = w[0]*w[0] + w[1]*w[1] + w[2]*w[2];
  float inverse_norm = fast_inverse_sqrt(squared_norm);
  m_array[0] = w[0] * inverse_norm;
  m_array[1] = w[1] * inverse_norm;
  m_array[2] = w[2] * inverse_norm;
}

inline normed_vec3::normed_vec3(float w0, float w1, float w2)
{
  m_array[0] = w0;
  m_array[1] = w1;
  m_array[2] = w2;
}

inline vec3 normed_vec3::to_vec3() const
{
  return vec3(m_array[0],m_array[1],m_array[2]);
}

inline float normed_vec3::x() const { return m_array[0]; }
inline float normed_vec3::y() const { return m_array[1]; }
inline float normed_vec3::z() const { return m_array[2]; }

inline normed_vec3& normed_vec3::operator-()
{
  m_array[0] = - m_array[0];
  m_array[1] = - m_array[1];
  m_array[2] = - m_array[2];

  return *this;
}
inline normed_vec3 normed_vec3::operator-() const
{
  return normed_vec3(-m_array[0],-m_array[1],-m_array[2]);
}

inline vec3::vec3(const rigid_container<float,3>& v)
{
  m_array[0] = v[0];
  m_array[1] = v[1];
  m_array[2] = v[2];
}

inline color::color(double r, double g, double b)
{
  m_array[0] = r;
  m_array[1] = g;
  m_array[2] = b;
}

inline color::color(const rigid_container<double,3>& v)
{
  m_array[0] = v[0];
  m_array[1] = v[1];
  m_array[2] = v[2];
}

inline double color::r() const { return m_array[0]; }
inline double color::g() const { return m_array[1]; }
inline double color::b() const { return m_array[2]; }

inline double& color::r() { return m_array[0]; }
inline double& color::g() { return m_array[1]; }
inline double& color::b() { return m_array[2]; }

inline void gamma2_correct(color& c)
{
  c.r() = std::sqrt(c.r());
  c.g() = std::sqrt(c.g());
  c.b() = std::sqrt(c.b());
}

inline void gamma_correct(color& c, double gamma)
{
  c.r() = std::pow(c.r(), 1.0/gamma);
  c.g() = std::pow(c.g(), 1.0/gamma);
  c.b() = std::pow(c.b(), 1.0/gamma);
}

inline int max_dimension(const vec3& v)
{
  if (v.x() > v.y())
  {
    if (v.x() > v.z())
      return 0;

    // z >= x > y
    return 2;
  }

  // y >= x
  if (v.y() > v.z())
    return 1;
  // z >= y >= x
  return 2;
}

inline float max_component(const vec3& v)
{
  if (v.x() > v.y())
  {
    if (v.x() > v.z())
      return v.x();

    // z >= x > y
    return v.z();
  }

  // y >= x
  if (v.y() > v.z())
    return v.y();
  // z >= y >= x
  return v.z();
}

inline float min_component(const vec3& v)
{
  if (v.x() < v.y())
  {
    if (v.x() < v.z())
      return v.x();

    return v.z();
  }

  if (v.y() < v.z())
    return v.y();

  return v.z();
}

inline vec3 abs(const rigid_container<float,3>& v)
{
  return vec3(std::abs(v[0]), std::abs(v[1]), std::abs(v[2]));
}

inline vec3 permute(const vec3& v, int x, int y, int z)
{
  return vec3{v[x], v[y], v[z]};
}

inline normed_vec3 permute(const normed_vec3& v, int x, int y, int z)
{
  return normed_vec3{v[x], v[y], v[z]};
}