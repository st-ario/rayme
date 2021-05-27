#pragma once

#include "my_vectors.h"

// #################################################################################################
// declarations
// #################################################################################################

class mat4
{
  protected:
    float M[16]; // column-major representation
  public:
    mat4();
    mat4(float x0,float x1,float x2,float x3
        ,float x4,float x5,float x6,float x7
        ,float x8,float x9,float xA,float xB
        ,float xC,float xD,float xE,float xF);

    float& operator[](short unsigned int i);
    float operator[](short unsigned int i) const;

    bool operator ==(const mat4& B) const;
    bool operator !=(const mat4& B) const;
    mat4& operator*=(const mat4& B);
    friend mat4 operator*(const mat4& A, const mat4& B);

    vec4 operator*(const vec4& v) const;
    void apply_to(vec4& v) const;
};

class transformation : public mat4
{
  private:
    // construct using the 3x3 linear part and the translation vector part
    transformation(float x0,float x1,float x2
                  ,float x4,float x5,float x6
                  ,float x8,float x9,float xA
                  ,float xC,float xD,float xE);
  public:
    transformation(); // standard constructor: identity transformation
    transformation(const mat4& matrix); // checks that the matrix is indeed an affine transformation
                                        // exits with an error message if that's not the case

    transformation& operator*=(const transformation& B);
    // slight abuse of notation: apply affine transformation to vec3
    vec3 operator*(const vec3& v) const;
    void apply_to(vec3& v) const;

    friend transformation rotation_matrix(const vec4& unit_quaternion);
    friend transformation translation_matrix(const vec3& translation_vector);
    friend transformation scale_matrix(const vec3& scale_vector);
    friend transformation scale_matrix(float scale_factor);
};

// #################################################################################################
// definitions
// #################################################################################################

inline mat4::mat4()
{
  for (unsigned short int i = 0; i < 16; ++i)
    M[i] = 0.0f;
}

inline mat4::mat4(float x0,float x1,float x2,float x3
                 ,float x4,float x5,float x6,float x7
                 ,float x8,float x9,float xA,float xB
                 ,float xC,float xD,float xE,float xF)
: M{x0,x1,x2,x3,x4,x5,x6,x7,x8,x9,xA,xB,xC,xD,xE,xF} {}

inline transformation::transformation(float x0,float x1,float x2
                                     ,float x4,float x5,float x6
                                     ,float x8,float x9,float xA
                                     ,float xC,float xD,float xE)
{
  M[0]  = x0; M[1]  = x1; M[2]  = x2; M[3]  = 0.0f;
  M[4]  = x4; M[5]  = x5; M[6]  = x6; M[7]  = 0.0f;
  M[8]  = x8; M[9]  = x9; M[10] = xA; M[11] = 0.0f;
  M[12] = xC; M[13] = xD; M[14] = xE; M[15] = 1.0f;
}

inline float& mat4::operator[](short unsigned int i)
{
  if (i > 15);
  // print error and exit

  return M[i];
}

inline float mat4::operator[](short unsigned int i) const
{
  if (i > 15);
  // print error and exit

  return M[i];
}

inline bool mat4::operator ==(const mat4& B) const
{
  for (int i = 0; i < 16; ++i)
  {
    if (M[i] != B[i])
      return false;
  }

  return true;
}
inline bool mat4::operator !=(const mat4& B) const { return !(*this == B); }

inline mat4& mat4::operator*=(const mat4& B)
{
  float N[16];
  for (int i = 0; i < 15; ++i)
    N[i] = M[i];

  M[0]  = N[0] * B[0]  + N[4] * B[1]  + N[8]  * B[2]  + N[12] * B[3];
  M[1]  = N[1] * B[0]  + N[5] * B[1]  + N[9]  * B[2]  + N[13] * B[3];
  M[2]  = N[2] * B[0]  + N[6] * B[1]  + N[10] * B[2]  + N[14] * B[3];
  M[3]  = N[3] * B[0]  + N[7] * B[1]  + N[11] * B[2]  + N[15] * B[3];
  M[4]  = N[0] * B[4]  + N[4] * B[5]  + N[8]  * B[6]  + N[12] * B[7];
  M[5]  = N[1] * B[4]  + N[5] * B[5]  + N[9]  * B[6]  + N[13] * B[7];
  M[6]  = N[2] * B[4]  + N[6] * B[5]  + N[10] * B[6]  + N[14] * B[7];
  M[7]  = N[3] * B[4]  + N[7] * B[5]  + N[11] * B[6]  + N[15] * B[7];
  M[8]  = N[0] * B[8]  + N[4] * B[9]  + N[8]  * B[10] + N[12] * B[11];
  M[9]  = N[1] * B[8]  + N[5] * B[9]  + N[9]  * B[10] + N[13] * B[11];
  M[10] = N[2] * B[8]  + N[6] * B[9]  + N[10] * B[10] + N[14] * B[11];
  M[11] = N[3] * B[8]  + N[7] * B[9]  + N[11] * B[10] + N[15] * B[11];
  M[12] = N[0] * B[12] + N[4] * B[13] + N[8]  * B[14] + N[12] * B[15];
  M[13] = N[1] * B[12] + N[5] * B[13] + N[9]  * B[14] + N[13] * B[15];
  M[14] = N[2] * B[12] + N[6] * B[13] + N[10] * B[14] + N[14] * B[15];
  M[15] = N[3] * B[12] + N[7] * B[13] + N[11] * B[14] + N[15] * B[15];

  return *this;
}

inline transformation& transformation::operator*=(const transformation& B)
{
  float N[16];
  for (int i = 0; i < 15; ++i)
    N[i] = M[i];

  M[0]  = N[0] * B[0]  + N[4] * B[1]  + N[8]  * B[2];
  M[1]  = N[1] * B[0]  + N[5] * B[1]  + N[9]  * B[2];
  M[2]  = N[2] * B[0]  + N[6] * B[1]  + N[10] * B[2];

  M[4]  = N[0] * B[4]  + N[4] * B[5]  + N[8]  * B[6];
  M[5]  = N[1] * B[4]  + N[5] * B[5]  + N[9]  * B[6];
  M[6]  = N[2] * B[4]  + N[6] * B[5]  + N[10] * B[6];

  M[8]  = N[0] * B[8]  + N[4] * B[9]  + N[8]  * B[10];
  M[9]  = N[1] * B[8]  + N[5] * B[9]  + N[9]  * B[10];
  M[10] = N[2] * B[8]  + N[6] * B[9]  + N[10] * B[10];

  M[12] = N[0] * B[12] + N[4] * B[13] + N[8]  * B[14] + N[12];
  M[13] = N[1] * B[12] + N[5] * B[13] + N[9]  * B[14] + N[13];
  M[14] = N[2] * B[12] + N[6] * B[13] + N[10] * B[14] + N[14];

  return *this;
}

inline void mat4::apply_to(vec4& v) const
{
  vec4 w{v};
  v[0] = M[0] * w[0] + M[4] * w[1] + M[8]  * w[2] + M[12] * w[3];
  v[1] = M[1] * w[0] + M[5] * w[1] + M[9]  * w[2] + M[13] * w[3];
  v[2] = M[2] * w[0] + M[6] * w[1] + M[10] * w[2] + M[14] * w[3];
  v[3] = M[3] * w[0] + M[7] * w[1] + M[11] * w[2] + M[15] * w[3];
}

inline mat4 operator*(const mat4& A, const mat4& B)
{
  mat4 res{A};
  res *= B;

  return res;
}

inline vec4 mat4::operator*(const vec4& v) const
{
  vec4 res{v};
  this->apply_to(res);

  return res;
}

inline transformation::transformation()
{
  for (unsigned short int i = 0; i < 16; ++i)
  {
    if (i%5 == 0)
      M[i] = 1.0f;
    else
      M[i] = 0.0f;
  }
}

inline transformation::transformation(const mat4& matrix)
{
  if (matrix[3] != 0.0f || matrix[7] != 0.0f || matrix[11] != 0.0f || matrix[15] != 1.0f);
    // write error message and exit

  for (unsigned short int i = 0; i < 16; ++i)
    M[i] = matrix[i];
}

inline vec3 transformation::operator*(const vec3& v) const
{
  vec3 res{v};
  this->apply_to(res);

  return res;
}

inline void transformation::apply_to(vec3& v) const
{
  point w{v};
  v[0] = M[0] * w[0] + M[4] * w[1] + M[8]  * w[2] + M[12];
  v[1] = M[1] * w[0] + M[5] * w[1] + M[9]  * w[2] + M[13];
  v[2] = M[2] * w[0] + M[6] * w[1] + M[10] * w[2] + M[14];
}

inline transformation rotation_matrix(const vec4& q)
{
  return transformation{
    1.0f - 2.0f * (q[1] * q[1] + q[2] * q[2]),
    2.0f * (q[0] * q[1] + q[2] * q[3]),
    2.0f * (q[0] * q[2] - q[1] * q[3]),
    2.0f * (q[0] * q[1] - q[2] * q[3]),
    1.0f - 2.0f * (q[0] * q[0] + q[2] * q[2]),
    2.0f * (q[1] * q[2] + q[0] * q[3]),
    2.0f * (q[0] * q[2] + q[1] * q[3]),
    2.0f * (q[1] * q[2] - q[0] * q[3]),
    1.0f - 2.0f * (q[0] * q[0] + q[1] * q[1]),
    0.0f, 0.0f, 0.0f
  };
}

inline transformation translation_matrix(const vec3& translation_vector)
{
  return transformation{ 1.0f,0.0f,0.0f
                       , 0.0f,1.0f,0.0f
                       , 0.0f,0.0f,1.0f
                       , translation_vector.x()
                       , translation_vector.y()
                       , translation_vector.z()};
}

inline transformation scale_matrix(const vec3& scale_vector)
{
  return transformation{ scale_vector.x(),0.0f,0.0f
                       , 0.0f, scale_vector.y(),0.0f
                       , 0.0f,0.0f, scale_vector.z()
                       , 0.0f,0.0f,0.0f};
}

inline transformation scale_matrix(float scale_factor)
{
  return transformation{ scale_factor,0.0f,0.0f
                       , 0.0f, scale_factor,0.0f
                       , 0.0f,0.0f, scale_factor
                       , 0.0f,0.0f,0.0f};
}