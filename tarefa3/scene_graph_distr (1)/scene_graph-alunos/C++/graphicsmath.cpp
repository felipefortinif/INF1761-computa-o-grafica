#include "graphicsmath.h"
#include "error.h"
#include <cmath>
#include <cstdlib>

Vec3 operator+ (const Vec3& a, const Vec3& b) { return { a.x + b.x, a.y + b.y, a.z + b.z }; }
Vec3 operator- (const Vec3& a, const Vec3& b) { return { a.x - b.x, a.y - b.y, a.z - b.z }; }
Vec3 operator- (const Vec3& a) { return { -a.x, -a.y, -a.z }; }
Vec3 operator* (const Vec3& a, float s) { return { a.x * s, a.y * s, a.z * s }; }
Vec3 operator* (float s, const Vec3& a) { return { a.x * s, a.y * s, a.z * s }; }

float Radians (float degrees) { return degrees * 3.14159265358979323846f / 180.0f; }

float Vec3Dot (const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

Vec3 Vec3Cross (const Vec3& a, const Vec3& b)
{
  return { a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x };
}

float Vec3Length (const Vec3& v) { return std::sqrt(Vec3Dot(v, v)); }

Vec3 Vec3Normalize (const Vec3& v)
{
  float len = Vec3Length(v);
  if (len == 0.0f) Error::Fatal("cannot normalize a zero-length vector");
  return v * (1.0f / len);
}

// Mat4 stores columns contiguously (m[col*4+row]) to match WGSL's mat4x4
// upload layout directly.
float Mat4At (const Mat4& m, int row, int col) { return m.m[col * 4 + row]; }
void Mat4Set (Mat4& m, int row, int col, float value) { m.m[col * 4 + row] = value; }

Mat4 Mat4Identity (float diagonal)
{
  Mat4 result = {};
  Mat4Set(result, 0, 0, diagonal);
  Mat4Set(result, 1, 1, diagonal);
  Mat4Set(result, 2, 2, diagonal);
  Mat4Set(result, 3, 3, diagonal);
  return result;
}

Mat4 Mat4Translation (const Vec3& offset)
{
  Mat4 result = Mat4Identity();
  Mat4Set(result, 0, 3, offset.x);
  Mat4Set(result, 1, 3, offset.y);
  Mat4Set(result, 2, 3, offset.z);
  return result;
}

Mat4 Mat4Scaling (const Vec3& factors)
{
  Mat4 result = Mat4Identity();
  Mat4Set(result, 0, 0, factors.x);
  Mat4Set(result, 1, 1, factors.y);
  Mat4Set(result, 2, 2, factors.z);
  return result;
}

Mat4 Mat4Rotation (float angleRadians, const Vec3& axis)
{
  Vec3 a = Vec3Normalize(axis);
  float c = std::cos(angleRadians);
  float s = std::sin(angleRadians);
  float t = 1.0f - c;
  Mat4 result = {};
  Mat4Set(result, 0, 0, t * a.x * a.x + c);
  Mat4Set(result, 0, 1, t * a.x * a.y - s * a.z);
  Mat4Set(result, 0, 2, t * a.x * a.z + s * a.y);
  Mat4Set(result, 1, 0, t * a.x * a.y + s * a.z);
  Mat4Set(result, 1, 1, t * a.y * a.y + c);
  Mat4Set(result, 1, 2, t * a.y * a.z - s * a.x);
  Mat4Set(result, 2, 0, t * a.x * a.z - s * a.y);
  Mat4Set(result, 2, 1, t * a.y * a.z + s * a.x);
  Mat4Set(result, 2, 2, t * a.z * a.z + c);
  Mat4Set(result, 3, 3, 1.0f);
  return result;
}

Mat4 Mat4Multiply (const Mat4& left, const Mat4& right)
{
  Mat4 result;
  for (int col = 0; col < 4; col++) {
    for (int row = 0; row < 4; row++) {
      float sum = 0.0f;
      for (int k = 0; k < 4; k++)
        sum += Mat4At(left, row, k) * Mat4At(right, k, col);
      Mat4Set(result, row, col, sum);
    }
  }
  return result;
}

Vec4 Mat4MulVec4 (const Mat4& m, const Vec4& v)
{
  float x[4] = {v.x, v.y, v.z, v.w};
  float out[4];
  for (int row = 0; row < 4; row++) {
    float sum = 0.0f;
    for (int col = 0; col < 4; col++) sum += Mat4At(m, row, col) * x[col];
    out[row] = sum;
  }
  return {out[0], out[1], out[2], out[3]};
}

Mat4 Mat4Translate (const Mat4& m, const Vec3& offset) { return Mat4Multiply(m, Mat4Translation(offset)); }
Mat4 Mat4Scale (const Mat4& m, const Vec3& factors) { return Mat4Multiply(m, Mat4Scaling(factors)); }
Mat4 Mat4Rotate (const Mat4& m, float angleRadians, const Vec3& axis) { return Mat4Multiply(m, Mat4Rotation(angleRadians, axis)); }

Mat4 Mat4Transpose (const Mat4& m)
{
  Mat4 result;
  for (int row = 0; row < 4; row++)
    for (int col = 0; col < 4; col++)
      Mat4Set(result, row, col, Mat4At(m, col, row));
  return result;
}

bool Mat4TryInverse (const Mat4& m, Mat4* out)
{
  // Gauss-Jordan elimination with partial pivoting on the augmented [m | I]
  // matrix, in double precision for numerical stability.
  double a[4][8];
  for (int row = 0; row < 4; row++) {
    for (int col = 0; col < 4; col++) a[row][col] = (double) Mat4At(m, row, col);
    for (int col = 0; col < 4; col++) a[row][4 + col] = (row == col) ? 1.0 : 0.0;
  }

  for (int pivot = 0; pivot < 4; pivot++) {
    int best = pivot;
    for (int row = pivot + 1; row < 4; row++)
      if (std::fabs(a[row][pivot]) > std::fabs(a[best][pivot])) best = row;
    if (best != pivot) for (int col = 0; col < 8; col++) std::swap(a[pivot][col], a[best][col]);

    double pivotValue = a[pivot][pivot];
    if (std::fabs(pivotValue) < 1e-12) return false;
    for (int col = 0; col < 8; col++) a[pivot][col] /= pivotValue;

    for (int row = 0; row < 4; row++) {
      if (row == pivot) continue;
      double factor = a[row][pivot];
      for (int col = 0; col < 8; col++) a[row][col] -= factor * a[pivot][col];
    }
  }

  for (int row = 0; row < 4; row++)
    for (int col = 0; col < 4; col++)
      Mat4Set(*out, row, col, (float) a[row][4 + col]);
  return true;
}

Mat4 Mat4Inverse (const Mat4& m)
{
  Mat4 result;
  if (!Mat4TryInverse(m, &result)) Error::Fatal("Mat4Inverse: matrix is singular");
  return result;
}

Mat4 Mat4LookAt (const Vec3& eye, const Vec3& center, const Vec3& up)
{
  Vec3 forward = Vec3Normalize(center - eye);
  Vec3 side = Vec3Normalize(Vec3Cross(forward, up));
  Vec3 correctedUp = Vec3Cross(side, forward);

  Mat4 result = Mat4Identity();
  Mat4Set(result, 0, 0, side.x); Mat4Set(result, 0, 1, side.y); Mat4Set(result, 0, 2, side.z);
  Mat4Set(result, 1, 0, correctedUp.x); Mat4Set(result, 1, 1, correctedUp.y); Mat4Set(result, 1, 2, correctedUp.z);
  Mat4Set(result, 2, 0, -forward.x); Mat4Set(result, 2, 1, -forward.y); Mat4Set(result, 2, 2, -forward.z);
  Mat4Set(result, 0, 3, -Vec3Dot(side, eye));
  Mat4Set(result, 1, 3, -Vec3Dot(correctedUp, eye));
  Mat4Set(result, 2, 3, Vec3Dot(forward, eye));
  return result;
}

Mat4 Mat4Perspective (float fovy, float aspect, float near, float far)
{
  if (!(fovy > 0.0f && fovy < 3.14159265358979323846f)) Error::Fatal("fovy must be between 0 and pi radians");
  if (aspect <= 0.0f) Error::Fatal("aspect must be > 0");
  if (near <= 0.0f) Error::Fatal("near must be > 0");
  if (far <= near) Error::Fatal("far must be greater than near");

  float focalLength = 1.0f / std::tan(fovy / 2.0f);
  Mat4 result = {};
  Mat4Set(result, 0, 0, focalLength / aspect);
  Mat4Set(result, 1, 1, focalLength);
  Mat4Set(result, 2, 2, far / (near - far));
  Mat4Set(result, 2, 3, far * near / (near - far));
  Mat4Set(result, 3, 2, -1.0f);
  return result;
}

Mat4 Mat4Ortho (float left, float right, float bottom, float top, float near, float far)
{
  if (left == right) Error::Fatal("left and right must differ");
  if (bottom == top) Error::Fatal("bottom and top must differ");
  if (near == far) Error::Fatal("near and far must differ");

  Mat4 result = Mat4Identity();
  Mat4Set(result, 0, 0, 2.0f / (right - left));
  Mat4Set(result, 1, 1, 2.0f / (top - bottom));
  Mat4Set(result, 2, 2, 1.0f / (near - far));
  Mat4Set(result, 0, 3, -(right + left) / (right - left));
  Mat4Set(result, 1, 3, -(top + bottom) / (top - bottom));
  Mat4Set(result, 2, 3, near / (near - far));
  return result;
}
