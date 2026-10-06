#ifndef GRAPHICSMATH_H
#define GRAPHICSMATH_H

// Small vector/matrix math for WebGPU. Mat4 is stored column-major
// (m[col*4+row]) so its bytes can be uploaded directly to a WGSL
// mat4x4<f32> with no repacking. perspective()/ortho() target WebGPU's
// [0,1] NDC z range, not OpenGL's [-1,1] - do not reuse GLM conventions.

struct Vec2 { float x, y; };
struct Vec3 { float x, y, z; };
struct Vec4 { float x, y, z, w; };
struct Mat4 { float m[16]; };

Vec3 operator+ (const Vec3& a, const Vec3& b);
Vec3 operator- (const Vec3& a, const Vec3& b);
Vec3 operator- (const Vec3& a);
Vec3 operator* (const Vec3& a, float s);
Vec3 operator* (float s, const Vec3& a);

float Radians (float degrees);
float Vec3Dot (const Vec3& a, const Vec3& b);
Vec3 Vec3Cross (const Vec3& a, const Vec3& b);
float Vec3Length (const Vec3& v);
Vec3 Vec3Normalize (const Vec3& v);

float Mat4At (const Mat4& m, int row, int col);
void Mat4Set (Mat4& m, int row, int col, float value);

Mat4 Mat4Identity (float diagonal = 1.0f);
Mat4 Mat4Translation (const Vec3& offset);
Mat4 Mat4Scaling (const Vec3& factors);
Mat4 Mat4Rotation (float angleRadians, const Vec3& axis);
Mat4 Mat4Multiply (const Mat4& left, const Mat4& right);
Vec4 Mat4MulVec4 (const Mat4& m, const Vec4& v);
Mat4 Mat4Translate (const Mat4& m, const Vec3& offset);
Mat4 Mat4Scale (const Mat4& m, const Vec3& factors);
Mat4 Mat4Rotate (const Mat4& m, float angleRadians, const Vec3& axis);
Mat4 Mat4Transpose (const Mat4& m);
Mat4 Mat4Inverse (const Mat4& m);
// Non-fatal variant: writes the inverse to *out and returns true, or
// returns false (leaving *out untouched) if m is singular.
bool Mat4TryInverse (const Mat4& m, Mat4* out);

Mat4 Mat4LookAt (const Vec3& eye, const Vec3& center, const Vec3& up);
Mat4 Mat4Perspective (float fovy, float aspect, float near, float far);
Mat4 Mat4Ortho (float left, float right, float bottom, float top, float near = -1.0f, float far = 1.0f);

#endif
