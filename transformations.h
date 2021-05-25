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
    vec3 operator*(const vec3& v);
    void apply_to(vec3& v);

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

inline mat4& mat4::operator*=(const mat4& B)
{
  M[0]  = M[0] * B[0]  + M[4] * B[1]  + M[8]  * B[2]  + M[12] * B[3];
  M[1]  = M[1] * B[0]  + M[5] * B[1]  + M[9]  * B[2]  + M[13] * B[3];
  M[2]  = M[2] * B[0]  + M[6] * B[1]  + M[10] * B[2]  + M[14] * B[3];
  M[3]  = M[3] * B[0]  + M[7] * B[1]  + M[11] * B[2]  + M[15] * B[3];
  M[4]  = M[0] * B[4]  + M[4] * B[5]  + M[8]  * B[6]  + M[12] * B[7];
  M[5]  = M[1] * B[4]  + M[5] * B[5]  + M[9]  * B[6]  + M[13] * B[7];
  M[6]  = M[2] * B[4]  + M[6] * B[5]  + M[10] * B[6]  + M[14] * B[7];
  M[7]  = M[3] * B[4]  + M[7] * B[5]  + M[11] * B[6]  + M[15] * B[7];
  M[8]  = M[0] * B[8]  + M[4] * B[9]  + M[8]  * B[10] + M[12] * B[11];
  M[9]  = M[1] * B[8]  + M[5] * B[9]  + M[9]  * B[10] + M[13] * B[11];
  M[10] = M[2] * B[8]  + M[6] * B[9]  + M[10] * B[10] + M[14] * B[11];
  M[11] = M[3] * B[8]  + M[7] * B[9]  + M[11] * B[10] + M[15] * B[11];
  M[12] = M[0] * B[12] + M[4] * B[13] + M[8]  * B[14] + M[12] * B[15];
  M[13] = M[1] * B[12] + M[5] * B[13] + M[9]  * B[14] + M[13] * B[15];
  M[14] = M[2] * B[12] + M[6] * B[13] + M[10] * B[14] + M[14] * B[15];
  M[15] = M[3] * B[12] + M[7] * B[13] + M[11] * B[14] + M[15] * B[15];

  return *this;
}

inline transformation& transformation::operator*=(const transformation& B)
{
  M[0]  = M[0] * B[0]  + M[4] * B[1]  + M[8]  * B[2];
  M[1]  = M[1] * B[0]  + M[5] * B[1]  + M[9]  * B[2];
  M[2]  = M[2] * B[0]  + M[6] * B[1]  + M[10] * B[2];

  M[4]  = M[0] * B[4]  + M[4] * B[5]  + M[8]  * B[6];
  M[5]  = M[1] * B[4]  + M[5] * B[5]  + M[9]  * B[6];
  M[6]  = M[2] * B[4]  + M[6] * B[5]  + M[10] * B[6];

  M[8]  = M[0] * B[8]  + M[4] * B[9]  + M[8]  * B[10];
  M[9]  = M[1] * B[8]  + M[5] * B[9]  + M[9]  * B[10];
  M[10] = M[2] * B[8]  + M[6] * B[9]  + M[10] * B[10];

  M[12] = M[0] * B[12] + M[4] * B[13] + M[8]  * B[14] + M[12];
  M[13] = M[1] * B[12] + M[5] * B[13] + M[9]  * B[14] + M[13];
  M[14] = M[2] * B[12] + M[6] * B[13] + M[10] * B[14] + M[14];

  return *this;
}

inline void mat4::apply_to(vec4& v) const
{
  v[0] = M[0] * v[0] + M[4] * v[1] + M[8]  * v[2] + M[12] * v[3];
  v[1] = M[1] * v[0] + M[5] * v[1] + M[9]  * v[2] + M[13] * v[3];
  v[2] = M[2] * v[0] + M[6] * v[1] + M[10] * v[2] + M[14] * v[3];
  v[3] = M[3] * v[0] + M[7] * v[1] + M[11] * v[2] + M[15] * v[3];
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

inline vec3 transformation::operator*(const vec3& v)
{
  vec3 res{v};
  this->apply_to(res);

  return res;
}

inline void transformation::apply_to(vec3& v)
{
  v[0] = M[0] * v[0] + M[4] * v[1] + M[8]  * v[2] + M[12];
  v[1] = M[1] * v[0] + M[5] * v[1] + M[9]  * v[2] + M[13];
  v[2] = M[2] * v[0] + M[6] * v[1] + M[10] * v[2] + M[14];
}

inline transformation rotation_matrix(const vec4& q)
{
  return transformation{
    1.0f - 2.0f * q[1] * q[1] - 2.0f * q[2] * q[2],
    2.0f * q[0] * q[1] + 2.0f * q[2] * q[3],
    2.0f * q[0] * q[2] - 2.0f * q[1] * q[3],
    2.0f * q[0] * q[1] - 2.0f * q[2] * q[3],
    1.0f - 2.0f * q[0] * q[0] - 2.0f * q[2] * q[2],
    2.0f * q[1] * q[2] + 2.0f * q[0] * q[3],
    2.0f * q[0] * q[2] + 2.0f * q[1] * q[3],
    2.0f * q[1] * q[2] - 2.0f * q[0] * q[3],
    1.0f - 2.0f * q[0] * q[0] - 2.0f * q[1] * q[1],
    0.0f, 0.0f, 0.0f
  };
}

inline transformation translation_matrix(const vec3& translation_vector)
{
  return transformation{ 0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0.0f
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