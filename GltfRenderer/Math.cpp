#include "Math.h"

#include <cmath>

Vec3::Vec3() {
	x = 0.f;
	y = 0.f;
	z = 0.f;
}

Vec3::Vec3(float x, float y, float z) {
	this->x = x;
	this->y = y;
	this->z = z;
}

Mat4::Mat4() {
	r0[0] = 0.f; r0[1] = 0.f; r0[2] = 0.f; r0[3] = 0.f;
	r1[0] = 0.f; r1[1] = 0.f; r1[2] = 0.f; r1[3] = 0.f;
	r2[0] = 0.f; r2[1] = 0.f; r2[2] = 0.f; r2[3] = 0.f;
	r3[0] = 0.f; r3[1] = 0.f; r3[2] = 0.f; r3[3] = 0.f;
}

Mat4 Mat4::Identity() {
	Mat4 res;
	res.r0[0] = 1.f;
	res.r1[1] = 1.f;
	res.r2[2] = 1.f;
	res.r3[3] = 1.f;

	return res;
}

Mat4 Mat4::Translate(float x, float y, float z) {
	Mat4 res = Mat4::Identity();
	res.r3[0] = x;
	res.r3[1] = y;
	res.r3[2] = z;

	return res;
}

Mat4 Mat4::RotateXZ(float angle) {
	Mat4 res = Mat4::Identity();
	res.r0[0] = std::cos(angle);
	res.r0[2] = std::sin(angle);
	res.r2[0] = -std::sin(angle);
	res.r2[2] = std::cos(angle);

	return res;
}

Mat4 Mat4::RotateYZ(float angle) {
	Mat4 res = Mat4::Identity();
	res.r1[1] = std::cos(angle);
	res.r1[2] = -std::sin(angle);
	res.r2[1] = std::sin(angle);
	res.r2[2] = std::cos(angle);

	return res;
 }

Mat4 Mat4::Projection(float fov, float aspect, float zNear, float zFar) {
	float horizontalFov = fov;
	float verticalFov = fov / aspect;

	float w = 1.f / std::tan(horizontalFov * 0.5f);
	float h = 1.f / std::tan(verticalFov * 0.5f);

	float zRatio = zFar / (zFar - zNear);

	Mat4 res;
	res.r0[0] = w;
  res.r1[1] = h;
	res.r2[2] = zRatio;
	res.r2[3] = 1.f;
	res.r3[2] = -zRatio * zNear;

	return res;
}

Mat4 operator*(const Mat4& M0, const Mat4& M1) {
	Mat4 res;

	res.r0[0] = M0.r0[0] * M1.r0[0] + M0.r0[1] * M1.r1[0] + M0.r0[2] * M1.r2[0] + M0.r0[3] * M1.r3[0];
	res.r0[1] = M0.r0[0] * M1.r0[1] + M0.r0[1] * M1.r1[1] + M0.r0[2] * M1.r2[1] + M0.r0[3] * M1.r3[1];
	res.r0[2] = M0.r0[0] * M1.r0[2] + M0.r0[1] * M1.r1[2] + M0.r0[2] * M1.r2[2] + M0.r0[3] * M1.r3[2];
	res.r0[3] = M0.r0[0] * M1.r0[3] + M0.r0[1] * M1.r1[3] + M0.r0[2] * M1.r2[3] + M0.r0[3] * M1.r3[3];

	res.r1[0] = M0.r1[0] * M1.r0[0] + M0.r1[1] * M1.r1[0] + M0.r1[2] * M1.r2[0] + M0.r1[3] * M1.r3[0];
	res.r1[1] = M0.r1[0] * M1.r0[1] + M0.r1[1] * M1.r1[1] + M0.r1[2] * M1.r2[1] + M0.r1[3] * M1.r3[1];
	res.r1[2] = M0.r1[0] * M1.r0[2] + M0.r1[1] * M1.r1[2] + M0.r1[2] * M1.r2[2] + M0.r1[3] * M1.r3[2];
	res.r1[3] = M0.r1[0] * M1.r0[3] + M0.r1[1] * M1.r1[3] + M0.r1[2] * M1.r2[3] + M0.r1[3] * M1.r3[3];

	res.r2[0] = M0.r2[0] * M1.r0[0] + M0.r2[1] * M1.r1[0] + M0.r2[2] * M1.r2[0] + M0.r2[3] * M1.r3[0];
	res.r2[1] = M0.r2[0] * M1.r0[1] + M0.r2[1] * M1.r1[1] + M0.r2[2] * M1.r2[1] + M0.r2[3] * M1.r3[1];
	res.r2[2] = M0.r2[0] * M1.r0[2] + M0.r2[1] * M1.r1[2] + M0.r2[2] * M1.r2[2] + M0.r2[3] * M1.r3[2];
	res.r2[3] = M0.r2[0] * M1.r0[3] + M0.r2[1] * M1.r1[3] + M0.r2[2] * M1.r2[3] + M0.r2[3] * M1.r3[3];

	res.r3[0] = M0.r3[0] * M1.r0[0] + M0.r3[1] * M1.r1[0] + M0.r3[2] * M1.r2[0] + M0.r3[3] * M1.r3[0];
	res.r3[1] = M0.r3[0] * M1.r0[1] + M0.r3[1] * M1.r1[1] + M0.r3[2] * M1.r2[1] + M0.r3[3] * M1.r3[1];
	res.r3[2] = M0.r3[0] * M1.r0[2] + M0.r3[1] * M1.r1[2] + M0.r3[2] * M1.r2[2] + M0.r3[3] * M1.r3[2];
	res.r3[3] = M0.r3[0] * M1.r0[3] + M0.r3[1] * M1.r1[3] + M0.r3[2] * M1.r2[3] + M0.r3[3] * M1.r3[3];

	return res;
}

Vec3 operator*(const Vec3& v, const Mat4& M) {
	Vec3 res;
	res.x = M.r0[0] * v.x + M.r1[0] * v.y + M.r2[0] * v.z;
	res.y = M.r0[1] * v.x + M.r1[1] * v.y + M.r2[1] * v.z;
	res.z = M.r0[2] * v.x + M.r1[2] * v.y + M.r2[2] * v.z;

	return res;
}
